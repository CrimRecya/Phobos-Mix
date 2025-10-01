#include "IH.Ext.h"

namespace Ext
{
	CSFClass_ITable& CSFClass_GetITable()
	{
		return *Init::LibInput->FunctionTable->CSFClass_pITable;
	}

	CallInterface::CallInterface(const std::string& FuncName) :FunctionName(FuncName), Funcs({ 0, nullptr }) {}
	CallInterface::CallInterface(const char* FuncName) : FunctionName(FuncName), Funcs({ 0, nullptr }) {}
	void CallInterface::Reload(const std::string& FuncName) { FunctionName = FuncName; Funcs.N = 0; Funcs.Data = nullptr; }
	void CallInterface::Reload(const char* FuncName) { FunctionName = FuncName; Funcs.N = 0; Funcs.Data = nullptr; }
	void CallInterface::Call(JsonObject Context)
	{
		if (Funcs.N == 0)
		{
			Funcs = GetFuncByName(FunctionName.c_str());
		}
		for (size_t i = 0; i < Funcs.N; i++)
		{
			GeneralCall(*Funcs.Data[i], Context);
		}
	}
	PArray<FuncInfo*> CallInterface::Get()
	{
		if (Funcs.N == 0)
		{
			Funcs = GetFuncByName(FunctionName.c_str());
		}
		return Funcs;
	}

	ActiveContext::ActiveContext(ActiveContext&& rhs)
	{
		Idx = rhs.Idx;
		CurContext = rhs.CurContext;
		rhs.Index() = { nullptr,nullptr,GetGeneratorParam((void*)nullptr) };
		rhs.Context() = NullJsonObject;
	}
	ActiveContext::ActiveContext(noinit_t) {}
	ActiveContext::ActiveContext(const ContextIndex& _Index) :Idx(_Index) {}
	void ActiveContext::Init(const ContextIndex& _Idx)
	{
		Idx = _Idx;
	}
	ActiveContext::~ActiveContext()
	{
		DeleteContextByIdx(Idx);
	}
	std::pair<ContextIndex, JsonObject> ActiveContext::Release()
	{
		std::pair<ContextIndex, JsonObject> Ret;
		std::swap(Ret.first, Idx);
		std::swap(Ret.second, CurContext);
		return Ret;
	}
	void ActiveContext::Bind(JsonObject Context)
	{
		CurContext = DirectBindContextTo(Context, Idx);
	}
	void ActiveContext::Bind(const char* Text)
	{
		CurContext = DirectBindTextTo(Text, Idx);
	}

	ActiveRoutine::ActiveRoutine(ActiveRoutine&& rhs) :_Name(std::move(rhs._Name)), Handle(nullptr), _Routine(nullptr)
	{
		std::swap(Handle, rhs.Handle);
		std::swap(_Routine, rhs._Routine);
	}
	ActiveRoutine::ActiveRoutine(const char* Str, Routine_t Fn, const RoutineParam& Param) : _Name(Str), Handle(nullptr), _Routine(Fn)
	{
		if (RegisterRoutine(Str, Fn, Param))
			Handle = GetRoutine(Str);
	}
	ActiveRoutine::ActiveRoutine(const std::string& Str, Routine_t Fn, const RoutineParam& Param) : _Name(Str), Handle(nullptr), _Routine(Fn)
	{
		if (RegisterRoutine(Str.c_str(), Fn, Param))
			Handle = GetRoutine(Str.c_str());
	}
	ActiveRoutine::ActiveRoutine(noinit_t) : Handle(nullptr) {}
	void ActiveRoutine::Init(const char* Str, Routine_t Fn, const RoutineParam& Param)
	{
		_Routine = Fn;
		_Name = Str;
		if (RegisterRoutine(Str, Fn, Param))
			Handle = GetRoutine(Str);
		else Handle = nullptr;
	}
	void ActiveRoutine::Init(const std::string& Str, Routine_t Fn, const RoutineParam& Param)
	{
		_Routine = Fn;
		_Name = Str;
		if (RegisterRoutine(Str.c_str(), Fn, Param))
			Handle = GetRoutine(Str.c_str());
		else Handle = nullptr;
	}
	ActiveRoutine::~ActiveRoutine()
	{
		if (Handle) DeleteRoutine(_Name.c_str());
	}
	void ActiveRoutine::Destroy()
	{
		if (Handle)
		{
			DeleteRoutine(_Name.c_str());
			Handle = nullptr;
		}
	}

	bool ActiveRoutine::Available() const
	{
		return Handle != nullptr;
	}
	void ActiveRoutine::InstantRun() const
	{
		if (Handle) InstantRunRoutine(_Routine);
	}
	void ActiveRoutine::Pause() const
	{
		if (Handle) PauseRoutine(_Routine);
	}
	void ActiveRoutine::Resume() const
	{
		if (Handle) ResumeRoutine(_Routine);
	}
	void ActiveRoutine::Delay(int NFrames) const
	{
		if (Handle) DelayRoutine(_Routine, NFrames);
	}
	RoutineParam* ActiveRoutine::GetParam() const
	{
		if (Handle)return GetRoutineParam(Handle);
		else return nullptr;
	}

	ActiveExecutor::ActiveExecutor(noinit_t) :Running(false) {}
	ActiveExecutor::ActiveExecutor(const GeneralExecutor& Executor) : Exec(Executor), Running(false)
	{
		Idx.Type = Executor.Base.Type;
		Idx.ExecType = Executor.Base.ExecType;
	}
	void ActiveExecutor::Init(const GeneralExecutor& Executor)
	{
		Exec = Executor;
		Running = false;
		Idx.Type = Executor.Base.Type;
		Idx.ExecType = Executor.Base.ExecType;
	}
	void ActiveExecutor::Destroy()
	{
		if (IsRunning())
		{
			RemoveExecutor(Idx);
			Running = false;
		}
	}
	ActiveExecutor::~ActiveExecutor()
	{
		if (IsRunning())
		{
			RemoveExecutor(Idx);
			Running = false;
		}
	}
	void ActiveExecutor::Start(const GeneratorParam& Param, bool DirectBind)
	{
		if (IsRunning())return;
		Idx.Param = Param;
		AddExecutor(Exec, Param, DirectBind);
	}
	bool ActiveExecutor::IsRunning()
	{
		if (Running)Running = HasExecutor(Idx);
		return Running;
	}
	JsonObject ActiveExecutor::GetContext()
	{
		if (!IsRunning())return NullJsonObject;
		return GetContextByIdx(Idx);
	}

	DispatchInterface::DispatchInterface(const std::string& _Lib, const std::string& _Proc, int _Version)
		:Lib(_Lib), Proc(_Proc), Info(nullptr), Version(_Version) {}
	void DispatchInterface::Reset(const std::string& _Lib, const std::string& _Proc, int _Version)
	{
		Lib = _Lib;
		Proc = _Proc;
		Version = _Version;
		Info = nullptr;
	}
	void DispatchInterface::Reset()
	{
		Info = nullptr;
	}
	FuncInfo* DispatchInterface::GetInfo()
	{
		static FuncInfo* NotFound = (FuncInfo*)0xFFFFFFFF;
		if (Info == NotFound)return nullptr;
		if (!Info)
		{
			Info = Ext::GetFuncFromLib(Lib.c_str(), Proc.c_str(), Version);
			if (!Info)
			{
				Info = NotFound;
				return nullptr;
			}
		}
		return Info;
	}
	LibFuncHandle DispatchInterface::GetFunc()
	{
		auto i = GetInfo();
		return i ? i->Func : nullptr;
	}

	void*& TableFunctionImpl(LibInputFnTable& Tbl, int FuncIdx)
	{
		static void* real_null = nullptr;
		auto ptr = (void**)&Tbl;
		int MaxIdx = (Tbl.Size >> 2) - 2;
		if (FuncIdx >= MaxIdx)
		{
			real_null = nullptr;
			return real_null;
		}
		return ptr[FuncIdx + 2];
	}
	void*& CustomFunctionImpl(LibInputFnTable& Tbl, int FuncIdx)
	{
		static void* real_null = nullptr;
		if (FuncIdx >= 5)
		{
			real_null = nullptr;
			return real_null;
		}
		return *((&Tbl.Custom_1) + FuncIdx);
	}
	LibData::LibData(BasicLibData* _Lib) :Lib(_Lib) {}
	bool LibData::Available()
	{
		if (!Lib)return false;
		return Lib->Available;
	}
	FuncInfo* LibData::QueryFunction(const char* Name, int Version)
	{
		//IHCore会检查参数，此处略过
		return Ext::QueryFunction(Lib, Name, Version);
	}
	int LibData::Version()
	{
		if (!Available())return 0;
		return Lib->Out->Info->Version;
	}
	const char* LibData::LibName()
	{
		if (!Available())return nullptr;
		return Lib->Out->Info->LibName;
	}
	LibInputFnTable* LibData::LibFnTable()
	{
		if (!Available())return 0;
		return Lib->In->FunctionTable;
	}
	LibFuncHandle LibData::GetTableFn(int FuncIdx)
	{
		if (!Available())return nullptr;
		return TableFunctionImpl(*Lib->In->FunctionTable, FuncIdx);
	}
	void LibData::SetTableFn(int FuncIdx, LibFuncHandle Fn)
	{
		if (!Available())return;
		TableFunctionImpl(*Lib->In->FunctionTable, FuncIdx) = Fn;
	}
	LibFuncHandle LibData::GetTableCustormFn(int FuncIdx)
	{
		if (!Available())return nullptr;
		return CustomFunctionImpl(*Lib->In->FunctionTable, FuncIdx);
	}
	void LibData::SetTableCustomFn(int FuncIdx, LibFuncHandle Fn)
	{
		if (!Available())return;
		CustomFunctionImpl(*Lib->In->FunctionTable, FuncIdx) = Fn;
	}

	CSFClass::CSFClass()
	{
		CSFClass_GetITable().Constructor(this);
	}
	CSFClass::~CSFClass()
	{
		CSFClass_GetITable().Destructor(this);
	}
	CSFClass::CSFClass(CSFClass&& rhs)
	{
		CSFClass_GetITable().MoveAssign(this, &rhs);
	}
	CSFClass::CSFClass(const CSFClass& rhs)
	{
		CSFClass_GetITable().Assign(this, &rhs);
	}
	void CSFClass::Assign(const CSFClass& rhs)
	{
		CSFClass_GetITable().Assign(this, &rhs);
	}
	bool CSFClass::LoadAsCCFile(const char* name)
	{
		return CSFClass_GetITable().LoadAsCCFile_A(this, name);
	}
	bool CSFClass::LoadAsCCFile(CCFileClass* pCC)
	{
		return CSFClass_GetITable().LoadAsCCFile_B(this, pCC);
	}
	bool CSFClass::LoadAsExternalFile(const char* name)
	{
		return CSFClass_GetITable().LoadAsExternalFile(this, name);
	}
	CSFEntry CSFClass::GetString(const char* Key)//merge操作之后请重新获取，clear操作之后全部失效
	{
		return CSFClass_GetITable().GetString(this, Key);
	}
	CSFEntry CSFClass::GetStringDefault(const char* Key)//merge操作之后请重新获取，clear操作之后全部失效
	{
		return CSFClass_GetITable().GetStringDefault(this, Key);
	}
	void CSFClass::Merge(const CSFClass* Another)
	{
		CSFClass_GetITable().Merge_A(this, Another);
	}
	void CSFClass::Merge(const char* Key, const wchar_t* value, const char* Extra)
	{
		CSFClass_GetITable().Merge_B(this, Key, value, Extra);
	}
	bool CSFClass::Available()
	{
		return CSFClass_GetITable().Available(this);
	}
	void CSFClass::Clear()
	{
		CSFClass_GetITable().Clear(this);
	}

	ECLoadStage GetLoadStage()
	{
		if (!Init::LibInput)return ECLoadStage::FailedToQuery;
		auto pStage = Init::LibInput->ECInitializeStage;
		if (!pStage)return ECLoadStage::FailedToQuery;
		switch (*pStage)
		{
		case 0:return ECLoadStage::InitialLoading;
		case 10:return ECLoadStage::InitialLoadComplete;
		case 20:
		case 50:return ECLoadStage::InitOrderChecked;
		case 100:return ECLoadStage::InitComplete;
		default:return ECLoadStage::FailedToQuery;
		}
	}

	FuncInfo* GetFuncFromLib(const char* pLib, const char* pFunc, int Version)
	{
		return Init::LibInput->FunctionTable->GetFuncFromLib(pLib, pFunc, Version);
	}
	PArray<FuncInfo*> GetFuncByName(const char* pFunc)
	{
		return Init::LibInput->FunctionTable->GetFuncByName(pFunc);
	}
	void RegisterContextProcessor(const char* Type, ContextFunc_t pProcessor)
	{
		Init::LibInput->FunctionTable->RegisterContextProcessor(Type, pProcessor);
	}
	void RegisterDirectBinder(const char* Type, const char* BindType, DirectBinder_t pDirectBinder)
	{
		Init::LibInput->FunctionTable->RegisterDirectBinder(Type, BindType, pDirectBinder);
	}
	bool RegisterRoutine(const char* Name, Routine_t Routine, RoutineParam Param)
	{
		return Init::LibInput->FunctionTable->RegisterRoutine(Name, Routine, Param);
	}
	GenCallRetType GeneralCall(const FuncInfo& Fn, JsonObject Context)
	{
		return Init::LibInput->FunctionTable->GeneralCall(Fn, Context);
	}
	JsonObject DirectBindContextTo(JsonObject Context, const ContextIndex& Idx)
	{
		return Init::LibInput->FunctionTable->DirectBindContextTo(Context, Idx);
	}
	JsonObject DirectBindTextTo(const char* Text, const ContextIndex& Idx)
	{
		return Init::LibInput->FunctionTable->DirectBindTextTo(Text, Idx);
	}
	JsonObject GetContextByIdx(const ContextIndex& Idx)
	{
		return Init::LibInput->FunctionTable->GetContextByIdx(Idx);
	}
	void DeleteContextByIdx(const ContextIndex& Idx)
	{
		Init::LibInput->FunctionTable->DeleteContextByIdx(Idx);
	}
	RoutineHandle GetRoutine(const char* Name)
	{
		return Init::LibInput->FunctionTable->GetRoutine(Name);
	}
	void InstantRunRoutine(RoutineHandle Routine)
	{
		Init::LibInput->FunctionTable->InstantRunRoutine(Routine);
	}
	void PauseRoutine(RoutineHandle Routine)
	{
		Init::LibInput->FunctionTable->PauseRoutine(Routine);
	}
	void ResumeRoutine(RoutineHandle Routine)
	{
		Init::LibInput->FunctionTable->ResumeRoutine(Routine);
	}
	void DelayRoutine(RoutineHandle Routine, int Delay)
	{
		Init::LibInput->FunctionTable->DelayRoutine(Routine, Delay);
	}
	void DeleteRoutine(const char* Name)
	{
		Init::LibInput->FunctionTable->DeleteRoutine(Name);
	}
	bool MakeExecutor_BaseText(ExecutorBase& Base, FuncInfo* Action, int Delay, const char* Text, const char* Type, const char* ExecType, SwizzleExecutor_t Swizzle, FuncInfo* Destructor)
	{
		return Init::LibInput->FunctionTable->MakeExecutor_BaseText(Base, Action, Delay, Text, Type, ExecType, Swizzle, Destructor);
	}
	bool MakeExecutor_Base(ExecutorBase& Base, FuncInfo* Action, int Delay, const char* Type, const char* ExecType, JsonObject Context, FuncInfo* Destructor)
	{
		return Init::LibInput->FunctionTable->MakeExecutor_Base(Base, Action, Delay, Type, ExecType, Context, Destructor);
	}
	bool MakeExecutor_Trigger(GeneralExecutor& Target, FuncInfo* Condition, int Interval)
	{
		return Init::LibInput->FunctionTable->MakeExecutor_Trigger(Target, Condition, Interval);
	}
	bool MakeExecutor_InfiniteTrigger(GeneralExecutor& Target, FuncInfo* Condition, int Interval)
	{
		return Init::LibInput->FunctionTable->MakeExecutor_InfiniteTrigger(Target, Condition, Interval);
	}
	bool MakeExecutor_Once(GeneralExecutor& Target)
	{
		return Init::LibInput->FunctionTable->MakeExecutor_Once(Target);
	}
	bool MakeExecutor_InfiniteLoop(GeneralExecutor& Target, int Interval)
	{
		return Init::LibInput->FunctionTable->MakeExecutor_InfiniteLoop(Target, Interval);
	}
	bool MakeExecutor_WhileLoop(GeneralExecutor& Target, FuncInfo* Condition, int Interval)
	{
		return Init::LibInput->FunctionTable->MakeExecutor_WhileLoop(Target, Condition, Interval);
	}
	bool MakeExecutor_NLoop(GeneralExecutor& Target, int N, int Interval)
	{
		return Init::LibInput->FunctionTable->MakeExecutor_NLoop(Target, N, Interval);
	}
	bool MakeExecutorEx_Trigger(GeneralExecutor& Target, const ExecutorBase& Base, FuncInfo* Condition, int Interval)
	{
		return Init::LibInput->FunctionTable->MakeExecutorEx_InfiniteTrigger(Target, Base, Condition, Interval);
	}
	bool MakeExecutorEx_InfiniteTrigger(GeneralExecutor& Target, const ExecutorBase& Base, FuncInfo* Condition, int Interval)
	{
		return Init::LibInput->FunctionTable->MakeExecutorEx_InfiniteTrigger(Target, Base, Condition, Interval);
	}
	bool MakeExecutorEx_Once(GeneralExecutor& Target, const ExecutorBase& Base)
	{
		return Init::LibInput->FunctionTable->MakeExecutorEx_Once(Target, Base);
	}
	bool MakeExecutorEx_InfiniteLoop(GeneralExecutor& Target, const ExecutorBase& Base, int Interval)
	{
		return Init::LibInput->FunctionTable->MakeExecutorEx_InfiniteLoop(Target, Base, Interval);
	}
	bool MakeExecutorEx_WhileLoop(GeneralExecutor& Target, const ExecutorBase& Base, FuncInfo* Condition, int Interval)
	{
		return Init::LibInput->FunctionTable->MakeExecutorEx_WhileLoop(Target, Base, Condition, Interval);
	}
	bool MakeExecutorEx_NLoop(GeneralExecutor& Target, const ExecutorBase& Base, int N, int Interval)
	{
		return Init::LibInput->FunctionTable->MakeExecutorEx_NLoop(Target, Base, N, Interval);
	}
	void AddExecutor(const GeneralExecutor& GenExec, const GeneratorParam& Param, bool DirectBind)
	{
		Init::LibInput->FunctionTable->AddExecutor(GenExec, Param, DirectBind);
	}
	bool HasExecutor(const ContextIndex& Idx)
	{
		return Init::LibInput->FunctionTable->HasExecutor(Idx);
	}
	void RemoveExecutor(const ContextIndex& Idx)
	{
		Init::LibInput->FunctionTable->RemoveExecutor(Idx);
	}
	RoutineParam* GetRoutineParam(RoutineHandle Routine)
	{
		return Init::LibInput->FunctionTable->GetRoutineParam(Routine);
	}
	void ResetGetFunctionBuffer()
	{
		Init::LibInput->FunctionTable->ResetGetFunctionBuffer();
	}

	bool RegisterRoutineSet(const char* Name, const RoutineSet& Routine, RoutineParam Param, bool Paused, int InitialDelay, bool ResetParam, CreateMode Mode)
	{
		return Init::LibInput->FunctionTable->RegisterRoutineSet(Name, Routine, Param, Paused, InitialDelay, ResetParam, Mode);
	}
	LibData GetLib(const char* Name)
	{
		return LibData(Init::LibInput->FunctionTable->GetLib(Name));
	}
	LibData GetAvailableLib(const char* Name)
	{
		return LibData(Init::LibInput->FunctionTable->GetAvailableLib(Name));
	}
	FuncInfo* QueryFunction(BasicLibData* Lib, const char* Name, int Version)
	{
		return Init::LibInput->FunctionTable->QueryFunction(Lib, Name, Version);
	}
	void* CustomFunction(int FuncIdx)
	{
		return CustomFunctionImpl(*Init::LibInput->FunctionTable, FuncIdx);
	}
}