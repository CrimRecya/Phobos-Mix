#pragma once
#include "IH.Loader.h"

namespace InitialLoad
{
	template<typename ParamType, typename... TArgs>
	IHInitialLoadService CreateRequest(const char* Name, TArgs&&... args)//创建后永不销毁
	{
		static_assert(std::is_base_of<InitialLoadParam, ParamType>::value, "ParamType 必须派生自 InitialLoadParam ！");
		return IHInitialLoadService{ Name,new ParamType(std::forward<TArgs>(args)...) };
	}
	void ServiceRequest(IHInitialLoadService IService);
	template<typename ParamType, typename... TArgs>
	void CreateRequestAndSubmit(const char* Name, TArgs&&... args)//创建后永不销毁
	{
		static_assert(std::is_base_of<InitialLoadParam, ParamType>::value, "ParamType 必须派生自 InitialLoadParam ！");
		ServiceRequest(IHInitialLoadService{ Name,new ParamType(std::forward<TArgs>(args)...) });
	}
	template<typename ParamType>
	class Service
	{
		static_assert(std::is_base_of<InitialLoadParam, ParamType>::value, "ParamType 必须派生自 InitialLoadParam ！");
		std::string ServiceName;
		PArray<IHInitialLoadService> Requests;
		bool Requested{ false };
	public:
		Service() = delete;
		Service(const Service&) = delete;
		Service(Service&&) = delete;

		Service(const std::string& Name) : ServiceName(Name), Requests() {}
		Service(const char* Name) : ServiceName(Name), Requests() {}
		void ProcessOnce(const std::function<void(const ParamType&)>& Func)//如果列表为空则填补并处理
		{
			if (!Requested)
			{
				Refresh();
				for (size_t i = 0; i < Requests.N; i++)
					if (Requests.Data[i].Param)
						Func(*static_cast<const ParamType* const>(Requests.Data[i].Param));
				Requested = true;
			}
		}
		void Process(const std::function<void(const ParamType&)>& Func)//如果列表为空仍会尝试refresh
		{
			if (!Requests.N)Refresh();
			for (size_t i = 0; i < Requests.N; i++)
				if (Requests.Data[i].Param)
					Func(*static_cast<const ParamType* const>(Requests.Data[i].Param));
		}
		void Refresh()
		{
			Requests = Init::QueryServiceRequest(ServiceName.c_str());
		}
		void RefreshAndProcess(const std::function<void(const ParamType&)>& Func)
		{
			Refresh();
			Process(Func);
		}
	};
}
