
#include "EC.h"

bool HasSyringeIH()
{
	return SyringeData::HasSyringeIH();
}
int SyringeIHVer()
{
	const auto& Data = SyringeData::GetExeData();
	return Data.VMajor * 1000000 + Data.VMinor * 10000 + Data.VRelease * 100 + Data.VBuild;
}

bool IHAvailable();
bool HasIH()
{
	return IHAvailable();
}
#ifndef SIWIC
#define WIC_LibName "SIWinterIsComing"
bool HasWIC()
{
	static bool Check { false };
	if (Check)return true;
	auto wic = Ext::GetLib(WIC_LibName);
	if (wic.Available())Check = true;
	return wic.Available();
}
int WICVer()
{
	static int Version { -1 };
	if (Version != -1)return Version;
	auto wic = Ext::GetLib(WIC_LibName);
	if (wic.Available())Version = wic.Version();
	return Version;
}
#endif

std::unordered_map<std::string, LibFuncHandle> ListenerMap;

LibFuncHandle __cdecl ListenerAccess(const char* Name)
{
	auto it = ListenerMap.find(Name);
	if (it != ListenerMap.end())return it->second;
	else return nullptr;
}
namespace ECListener
{
	void Set(const char* ListenerType, LibFuncHandle Function)
	{
		ListenerMap[ListenerType] = Function;
	}
	LibFuncHandle Get(const char* ListenerType)
	{
		return ListenerAccess(ListenerType);
	}
	void Remove(const char* ListenerType)
	{
		if (ListenerMap.find(ListenerType) != ListenerMap.end())ListenerMap.erase(ListenerType);
	}

	InitialLoad::Service<InitialLoadParam_RegisterFunction>InternalService_ListenerAccess("EC::Internal::ListenerAccess");

	std::vector<LibFuncHandle> GetAll(const char* ListenerType)
	{
		std::vector<LibFuncHandle> Result;
		InternalService_ListenerAccess.RefreshAndProcess([&](const InitialLoadParam_RegisterFunction& Request)
			{
				if (Request.Handle)
				{
					auto hd = AsType<decltype(ListenerAccess)>(Request.Handle)(ListenerType);
					if (hd)Result.push_back(hd);
				}
			});
		return Result;
	}

	void Listen_InitBeforeEverything(Listener_InitBeforeEverything Func)
	{
		Set("EC::InitBeforeEverything", Func);
	}

	void Listen_LoadBeforeTypeData(Listener_OnLoadGame Func)
	{
		Set("EC::OnReadingRules", Func);
	}
	void Listen_LoadAfterTypeData(Listener_OnLoadGame Func)
	{
		Set("EC::LoadAfterTypeData", Func);
	}

	void Listen_BeginWritingExceptIH(Listener_BeginWritingExceptIH Func)
	{
		Set("EC::BeginWritingExceptIH", Func);
	}

	void Listen_LoadCSFString(Listener_LoadCSFString Func)
	{
		Set("EC::OnLoadCSFString", Func);
	}

	void Listen_LoadScenario(Listener_LoadScenario Func)
	{
		Set("EC::OnLoadScenario", Func);
	}

	void Listen_AfterLoadGame(Listener_AfterLoadGame Func)
	{
		Set("EC::AfterLoadGame", Func);
	}

	void Listen_AfterLoadINI(Listener_AfterLoadINI Func)
	{
		Set("EC::AfterLoadINI", Func);
	}

	void Listen_ClearScenario(Listener_ClearScenario Func)
	{
		Set("EC::ClearScenario", Func);
	}
}

FuncInfo* __cdecl NullGetFunc(const char* Name, int Version)
{
	return nullptr;
}

using UTF8_SString = std::basic_string< UTF8_CharType, std::char_traits<UTF8_CharType>, std::allocator<UTF8_CharType> >;

std::string Temp_LibraryName;
int Temp_Version;
UTF8_SString Temp_Description;
std::function<void()> Temp_OnFirstInit;
std::function<void()> Temp_OnOrderedInit;
std::vector<InitDependency> Temp_Dependencies;
LibVersionInfo Temp_VersionInfo;
GetFunc_t Temp_GetFunc;


std::unordered_map<std::string, FuncInfo> Temp_FuncList;
FuncInfo* __cdecl GetFuncFromList(const char* Name, int Version)
{
	if (Version != DoNotCheckVersion && Temp_Version < Version)return nullptr;
	auto it = Temp_FuncList.find(Name);
	if (it == Temp_FuncList.end())return nullptr;
	else return &it->second;
}

void StoreInitConfig(
	const char* LibraryName, //库名
	int Version,
	int LowestSupportedVersion,
	UTF8_CString Description,
	const std::function<void()>& OnFirstInit,
	const std::function<void()>& OnOrderedInit,
	std::initializer_list<InitDependency> Dependencies
)
{
	Temp_LibraryName = LibraryName ? LibraryName : "";
	Temp_Description = Description ? UTF8_SString(Description) : UTF8_SString();
	Temp_OnFirstInit = OnFirstInit;
	Temp_OnOrderedInit = OnOrderedInit;
	Temp_Dependencies.assign(Dependencies);
	Temp_Version = Version;
	
	Temp_VersionInfo.LibName = Temp_LibraryName.c_str();
	Temp_VersionInfo.Version = Version;
	Temp_VersionInfo.LowestSupportedVersion = LowestSupportedVersion;
	Temp_VersionInfo.Description = Temp_Description.c_str();
}

bool ECInitLibrary(
	const char* LibraryName, //库名
	int Version,
	int LowestSupportedVersion,
	UTF8_CString Description,
	const std::function<void()>& OnFirstInit,
	const std::function<void()>& OnOrderedInit,
	std::initializer_list<InitDependency> Dependencies
)
{
	StoreInitConfig(LibraryName, Version, LowestSupportedVersion, Description, OnFirstInit, OnOrderedInit, Dependencies);
	Temp_GetFunc = NullGetFunc;
	return Init::Initialize();
}

bool ECInitLibrary(
	const char* LibraryName, //库名
	int Version,
	int LowestSupportedVersion,
	UTF8_CString Description,
	const std::function<void()>& OnFirstInit,
	const std::function<void()>& OnOrderedInit,
	GetFunc_t GetFunc,
	std::initializer_list<InitDependency> Dependencies
)
{
	StoreInitConfig(LibraryName, Version, LowestSupportedVersion, Description, OnFirstInit, OnOrderedInit, Dependencies);
	Temp_GetFunc = GetFunc;
	return Init::Initialize();
}

bool ECInitLibrary(
	const char* LibraryName, //库名
	int Version,
	int LowestSupportedVersion,
	UTF8_CString Description,
	const std::function<void()>& OnFirstInit,
	const std::function<void()>& OnOrderedInit,
	const std::unordered_map<std::string, LibFuncHandle>& ExportFuncs,
	std::initializer_list<InitDependency> Dependencies
)
{
	StoreInitConfig(LibraryName, Version, LowestSupportedVersion, Description, OnFirstInit, OnOrderedInit, Dependencies);
	Temp_GetFunc = GetFuncFromList;
	for (const auto& p : ExportFuncs)
		Temp_FuncList.insert({ p.first, FuncInfo(p.second , FuncType::Default) });
	return Init::Initialize();
}

void __cdecl MyOrderedInit()
{
	if (Temp_OnOrderedInit)Temp_OnOrderedInit();
}

void MyInit(InitResult& Result)
{
	Result.Info = &Temp_VersionInfo;
	Result.GetFunc = Temp_GetFunc;
	Result.Dependencies = PArray<InitDependency>(Temp_Dependencies);
	Result.OrderedInit = MyOrderedInit;

	if (Temp_OnFirstInit)
	{
		Temp_OnFirstInit();
	}
}
