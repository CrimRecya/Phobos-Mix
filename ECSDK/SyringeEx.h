#pragma once
#include <Syringe.h>
#include <vector>
#include <unordered_map>
#include <string>
#include "ExtJson.h"

#if SYR_VER == 2

#pragma pack(push, 16)
#pragma warning(push)
#pragma warning( disable : 4324)
__declspec(align(32)) struct hookaltdecl {
	unsigned int hookAddr;
	unsigned int hookSize;
	const char* hookName;
	int Priority;
	const char* SubPriorityPtr;
};

static_assert(sizeof(hookaltdecl) == 32);
#pragma warning(pop)
#pragma pack(pop)

#pragma section(".hphks00", read, write)

namespace SyringeData {
	namespace HookAlt {

	};
};

#define declhookex(hook, funcname, size, priority, sub_priority) \
namespace SyringeData { \
	namespace HookAlt { __declspec(allocate(".hphks00")) hookaltdecl _hk__ ## hook ## funcname = { ## hook, ## size, #funcname, ## priority, ## sub_priority }; }; \
};

#endif

#ifndef declhookex
#define declhookex(hook, funcname, size, priority, sub_priority)
#endif // declhookex

//#ifdef DEFINE_HOOK 

#define DEFINE_HOOKEX(hook, funcname, size, priority, sub_priority) \
declhookex(hook, funcname, size, priority, sub_priority) \
EXPORT_FUNC(funcname)

#define DEFINE_HOOKEX_AGAIN(hook, funcname, size, priority, sub_priority) \
declhookex(hook, funcname, size, priority, sub_priority)

using HookType = DWORD(__cdecl*)(REGISTERS*);

template<typename T>
struct SyrPArray
{
	size_t N;
	const T* Data;

	SyrPArray() :N(0), Data(nullptr) {}
	SyrPArray(const std::vector<T>& p) :N(p.size()), Data(p.data()) {}

};


//在DllMain当中Init::Initialize()后即可使用
namespace SyringeData
{
	struct ExeRemoteData
	{
		char SyringeVersionStr[256];
		BYTE VMajor;
		BYTE VMinor;
		BYTE VRelease;
		BYTE VBuild;

		char FileName[260];
		char AbsPath[260];
		DWORD BaseAddress;
		DWORD EntryPoint;
		
		int dwReserved[3];
	};

	struct LibRemoteData
	{
		const char* LibName;
		const char* AbsPath;
		DWORD LibID;
		const char* SettingText;

		int dwReserved[4];
	};

	struct AddrRemoteData
	{
		DWORD Addr;
		DWORD HookDataAddr;
		int OverriddenCount;
		int HookCount;
		int dwReserved[6];
		DWORD FirstHookIndex;//VLA Header

		inline DWORD HookIdx(int i)
		{
			return *((&FirstHookIndex) + i);
		}

		inline BYTE* HookData()
		{
			return (BYTE*)HookDataAddr;
		}

		size_t GetOpSize() const;
	};
	struct HookRemoteData
	{
		const char* ProcName;
		DWORD HookID;
		DWORD LibID;
		DWORD HookAddress;
		size_t OverrideLength;
		int dwReserved[3];
	};

	struct MemCopyData
	{
		char* Name;
		void* Addr;
	};

	struct SharedMemHeader
	{
		int TotalSize;
		int WritingComplete;
		int RecordCount;
		int RecordSize;

		DWORD DatabaseAddr;
		DWORD DllRecordAddr;
		int DllRecordCount;
		int ReservedPtr;
		int ReservedHandle;
		int Reserved[7];
	};

	struct HookContext
	{
		REGISTERS* pReg;
		SyrPArray<BYTE> Stack;

		HookContext() = delete;
		HookContext(const HookContext&) = delete;
		HookContext(HookContext&&);
		HookContext(REGISTERS* pReg);
		HookContext(size_t StackSize, size_t TopOffset);
		~HookContext();
		DWORD CallHook(HookType Proc) const;
		DWORD CallHook(DWORD Address, const std::string& Lib, const std::string& Proc) const;
	};
#define ThisHookContext HookContext{ R };

	extern std::unordered_map<std::string, LibRemoteData*> LibMap;
	extern std::unordered_map<DWORD, AddrRemoteData*> AddrMap;
	extern std::unordered_map<DWORD, HookRemoteData*> HookMap;
	extern std::unordered_map<std::string, MemCopyData*> MemMap;
	extern std::unordered_map<std::wstring, DWORD> LoadLib;

	bool HasSyringeIH();
	DWORD SyringeHash(const char* str);
	DWORD SyringeHashUpper(const char* str);
	DWORD GetSyringeProcID();
	const std::string& ExecutableDirectoryPath();
	const std::string& UniqueIDByPath();//只考虑地址
	DWORD BaseAddress();
	void InitRemoteData();
	ExeRemoteData& GetExeData();
	DWORD GetDatabaseSize();
	LibRemoteData* GetLibData(const std::string& Name);
	LibRemoteData* GetLibData(const DWORD LibID);
	DWORD GetLibID(const std::string& Name);
	DWORD GetHookID(const std::string& Lib, const std::string& Proc);
	AddrRemoteData* GetAddrData(DWORD Addr);
	MemCopyData* GetCopyMemData(const std::string& Name);
	HookRemoteData* GetHookData(const std::string& Lib, const std::string& Proc);
	HookRemoteData* GetHookData(const DWORD HookID);
	DWORD GetLibBaseAddress(std::wstring Name);

	SyrPArray<BYTE> GetOpCode(DWORD Addr);
	int GetValidHookCount(DWORD Addr);
	const char* GetSettingText(const LibRemoteData& Lib);
	void ReplaceHookFunction(DWORD Address, const std::string& Lib, const std::string& Proc, DWORD pFunc);
	void RestoreHookFunction(DWORD Address, const std::string& Lib, const std::string& Proc);
	void DisableOpCode(DWORD Addr);
	void EnableOpCode(DWORD Addr);
	void EnableHook(DWORD Address, const std::string& Lib, const std::string& Proc);
	void DisableHook(DWORD Address, const std::string& Lib, const std::string& Proc);
	void EnableAllHooksAt(DWORD Address);
	void DisableAllHooksAt(DWORD Address);
	bool IsHookValid(DWORD Address, const std::string& Lib, const std::string& Proc);
	HookType GetHookProc(DWORD Address, const std::string& Lib, const std::string& Proc);
}
/*

    GetOpCode获取/修改被覆盖的机器码
	DisableOpCode取消执行原始机器码
	EnableOpCode恢复执行原始机器码
	GetActualOverriddenBytes获取实际覆盖的字节数
    GetValidHookCount获取地址上有效的钩子数量
	EnableHook运行时启用指定钩子
	DisableHook运行时禁用指定钩子
	EnableAllHooksAt启用某个地址的全部钩子
	DisableAllHooksAt禁用某个地址的全部钩子
	ReplaceHookFunction换掉某个钩子执行的函数
	RestoreHookFunction恢复某个钩子执行的函数
	ReplaceFunctionBody替换函数体
	RestoreFunctionBody恢复函数体
	GetSettingJSON获取设置
	GetSettingText获取设置，但是文本
	GetLoadedConfigJSON获取全部设置
	GetLoadedConfigText获取全部设置，但是文本
	HookContext:调用钩子

*/