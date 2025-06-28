#include "SyringeEx.h"
#include <YRPP.h>



struct _USTRING
{
	unsigned short Len;
	unsigned short MaxLen;
	wchar_t* Buf;
};

namespace SyringeData
{
	struct RemoteDataHeader
	{
		int Size;
		int NLib;
		int NAddr;
		int NHook;
		int NMem;

		int ExeDataOffset;
		int LibDataListOffset;
		int AddrDataListOffset;
		int HookDataListOffset;
		int CopyMemListOffset;
		int dwReserved[20];

		int HookOpCodeSize;
		int JmpBackCodeSize;
	};



	RemoteDataHeader* pHeader;
	ExeRemoteData* pExe;

	SyrPArray<LibRemoteData*> pLibArray;
	std::unordered_map<std::string, LibRemoteData*> LibMap;
	std::unordered_map<DWORD, LibRemoteData*> LibMap_ID;
	SyrPArray<AddrRemoteData*> pAddrArray;
	std::unordered_map<DWORD, AddrRemoteData*> AddrMap;
	SyrPArray<HookRemoteData*> pHookArray;
	std::unordered_map<DWORD, HookRemoteData*> HookMap;
	SyrPArray<MemCopyData*> pMemArray;
	std::unordered_map<std::string, MemCopyData*> MemMap;
	std::string SharedMemoryName;
	DWORD SyringeProcID;
	std::unordered_map<std::wstring, DWORD> LoadLib;
	DWORD NullOutput;

	//reference<DWORD, 0x888808> pBuffer;
	DWORD Buffer_;
	template<typename T>
	T* BufferOffset(DWORD sz)
	{
		return reinterpret_cast<T*>(Buffer_ + sz);
	}

	DWORD GetSyringeProcID()
	{
		return SyringeProcID;
	}

	DWORD BaseAddress()
	{
		return Buffer_;
	}

	DWORD GetLibBaseAddress(std::wstring Name)
	{
		for (auto& wc : Name)wc = towupper(wc);
		auto it = LoadLib.find(Name);
		if (it != LoadLib.end())return it->second;
		else return 0;
	}

	DWORD GetDatabaseSize()
	{
		if (!pHeader)return 0;
		return (DWORD)pHeader->Size;
	}

	const std::string& ExecutableDirectoryPath()
	{
		static std::string ss;
		if (!ss.empty())return ss;
		std::vector<char> full_path_exe(MAX_PATH);

		for (;;)
		{
			const DWORD result = GetModuleFileNameA(NULL,
				&full_path_exe[0],
				full_path_exe.size());

			if (result == 0)
			{
				// Report failure to caller. 
			}
			else if (full_path_exe.size() == result)
			{
				// Buffer too small: increase size. 
				full_path_exe.resize(full_path_exe.size() * 2);
			}
			else
			{
				// Success. 
				break;
			}
		}

		// Remove executable name. 
		std::string result(full_path_exe.begin(), full_path_exe.end());
		std::string::size_type i = result.find_last_of("\\/");
		if (std::string::npos != i) result.erase(i);

		ss = result;
		return ss;
	}
	DWORD SyringeHash(const char* str)
	{
		DWORD Result = 0;
		DWORD Mod = 19260817;
		for (const char* ps = str; *ps; ++ps)
		{
			Result *= Mod;
			Result += (DWORD)(*ps);
		}
		return Result + strlen(str);
	}
	DWORD SyringeHashUpper(const char* str)
	{
		DWORD Result = 0;
		DWORD Mod = 19260817;
		for (const char* ps = str; *ps; ++ps)
		{
			Result *= Mod;
			Result += (DWORD)toupper(*ps);
		}
		return Result + strlen(str);
	}
	const std::string& UniqueIDByPath()
	{
		static std::string Result{};
		if (!Result.empty())return Result;
		auto id = SyringeHashUpper(ExecutableDirectoryPath().c_str());
		Result = std::to_string(id);
		return Result;
	}

	DWORD* _PEB;
	DWORD ModuleListHeader()
	{
		__asm
		{
			push eax
			mov eax, fs: [0x30]
			mov _PEB, eax
			pop eax
		}
		return *(_PEB + 0x03) + 0x0C;
	}
	void GetModuleList(DWORD Header)
	{
		_LIST_ENTRY* p, * Head;
		p = Head = ((_LIST_ENTRY*)Header)->Flink;
		do
		{
			_USTRING* Name = (_USTRING*)(((int)p) + 0x2C);
			int Base = *((int*)(((int)p) + 0x18));
			if (!Name->Buf)break;
			std::wstring ws = Name->Buf;
			for (auto& wc : ws)wc = towupper(wc);
			LoadLib[ws] = (DWORD)Base;
			p = p->Flink;
		} while (p != Head);
	}


	bool HasInit{ false };

	bool HasSyringeIH()
	{
		return HasInit;
	}

	void InitRemoteData()
	{
		if (HasInit)return;
		//SyringeProcID = pBuffer();
		SharedMemoryName = ("SYRINGE" + UniqueIDByPath() + std::to_string((int)::GetCurrentProcessId()));
		//SharedMemoryName = ("SYRINGE" + UniqueIDByPath() + std::to_string((int)pBuffer()));
		auto hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, SharedMemoryName.c_str());
		//::MessageBoxA(NULL, ("SYRINGE" + UniqueIDByPath() + std::to_string((int)pBuffer())).c_str(), "InitRemoteData_02", MB_OK);
		if (!hMap)return;
		auto View = (SharedMemHeader*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemHeader));
		if (!View) {
			CloseHandle(hMap); return;
		}

		SyringeProcID = View->ReservedHandle;
		NullOutput = View->ReservedPtr;
		Buffer_ = View->DatabaseAddr;
		HasInit = true;
		UnmapViewOfFile(View);
		CloseHandle(hMap);

		pHeader = BufferOffset<RemoteDataHeader>(0);
		GetModuleList(ModuleListHeader());

		
		pExe = BufferOffset<ExeRemoteData>(pHeader->ExeDataOffset);
		pLibArray.N = pHeader->NLib;
		pLibArray.Data = BufferOffset<LibRemoteData*>(pHeader->LibDataListOffset);
		for (size_t i = 0; i < pLibArray.N; i++)
		{
			LibMap[pLibArray.Data[i]->LibName] = pLibArray.Data[i];
			LibMap_ID[pLibArray.Data[i]->LibID] = pLibArray.Data[i];
		}

		pAddrArray.N = pHeader->NAddr;
		pAddrArray.Data = BufferOffset<AddrRemoteData*>(pHeader->AddrDataListOffset);
		for (size_t i = 0; i < pAddrArray.N; i++)
		{
			AddrMap[pAddrArray.Data[i]->Addr] = pAddrArray.Data[i];
		}
		pHookArray.N = pHeader->NHook;
		pHookArray.Data = BufferOffset<HookRemoteData*>(pHeader->HookDataListOffset);
		for (size_t i = 0; i < pHookArray.N; i++)
		{
			HookMap[pHookArray.Data[i]->HookID] = pHookArray.Data[i];
		}
		pMemArray.N = pHeader->NMem;
		pMemArray.Data = BufferOffset<MemCopyData*>(pHeader->CopyMemListOffset);
		for (size_t i = 0; i < pMemArray.N; i++)
		{
			MemMap[pMemArray.Data[i]->Name] = pMemArray.Data[i];
		}
	}

	ExeRemoteData& GetExeData()
	{
		return *pExe;
	}
	LibRemoteData* GetLibData(const std::string& Name)
	{
		auto it = LibMap.find(Name);
		if (it != LibMap.end())return it->second;
		else return nullptr;
	}
	LibRemoteData* GetLibData(const DWORD LibID)
	{
		auto it = LibMap_ID.find(LibID);
		if (it != LibMap_ID.end())return it->second;
		else return nullptr;
	}

	DWORD GetLibID(const std::string& Name)
	{
		return SyringeHashUpper(Name.c_str());
	}
	DWORD GetHookID(const std::string& Lib, const std::string& Proc)
	{
		static const std::string AnalyzerDelim = "\\*^*\\";
		return SyringeHashUpper((Lib + AnalyzerDelim + Proc).c_str());
	}
	AddrRemoteData* GetAddrData(DWORD Addr)
	{
		auto it = AddrMap.find(Addr);
		if (it != AddrMap.end())return it->second;
		else return nullptr;
	}
	MemCopyData* GetCopyMemData(const std::string& Name)
	{
		auto it = MemMap.find(Name);
		if (it != MemMap.end())return it->second;
		else return nullptr;
	}
	HookRemoteData* GetHookData(const std::string& Lib, const std::string& Proc)
	{
		auto it = HookMap.find(GetHookID(Lib, Proc));
		if (it != HookMap.end())return it->second;
		else return nullptr;
	}
	HookRemoteData* GetHookData(const DWORD HookID)
	{
		auto it = HookMap.find(HookID);
		if (it != HookMap.end())return it->second;
		else return nullptr;
	}

	struct AddrHiddenHeader
	{
		DWORD ActiveHookCount;
		BYTE* OpCodeAddress;
		DWORD OverriddenCount;
		DWORD dwReserved;
	};

	size_t AddrRemoteData::GetOpSize() const
	{
		return sizeof(SyringeData::AddrHiddenHeader) + HookCount * pHeader->HookOpCodeSize
			+ pHeader->JmpBackCodeSize + OverriddenCount;
	}

	
	SyrPArray<BYTE> GetOpCode(DWORD Addr)
	{
		auto pAddr = GetAddrData(Addr);
		SyrPArray<BYTE> Ret;
		Ret.N = 0; Ret.Data = nullptr;
		if (!pAddr)return Ret;
		AddrHiddenHeader* pHidden = (AddrHiddenHeader*)(pAddr->HookDataAddr - sizeof(AddrHiddenHeader));
		Ret.N = pHidden->OverriddenCount, Ret.Data = pHidden->OpCodeAddress;
		return Ret;
	}

	int GetValidHookCount(DWORD Addr)
	{
		auto pAddr = GetAddrData(Addr);
		if (!pAddr)return 0;
		AddrHiddenHeader* pHidden = (AddrHiddenHeader*)(pAddr->HookDataAddr - sizeof(AddrHiddenHeader));
		return (int)pHidden->ActiveHookCount;
	}

	size_t GetActualOverriddenBytes(DWORD Addr)
	{
		auto pAddr = GetAddrData(Addr);
		if (!pAddr)return 0;
		AddrHiddenHeader* pHidden = (AddrHiddenHeader*)(pAddr->HookDataAddr - sizeof(AddrHiddenHeader));
		return pHidden->OverriddenCount;
	}

	HMODULE SyrEx{ nullptr };
	struct ExportOffsets
	{
		int CodeSize;
		int CallOfs;
		int Reserved[6];
	};

#define UPDFN(x) \
	static decltype(& x) fn;\
	if (!fn)fn = reinterpret_cast<decltype(& x)>(GetProcAddress(SyrEx, #x));\
	if (!fn)return
#define UPDCHK\
	if (!SyrEx)SyrEx = LoadLibraryA("SyringeEx.dll");\
	if (!SyrEx)return


	void __stdcall UpdateJson(const char* Name, const char* Json)
	{
		UPDCHK;
		UPDFN(UpdateJson);
		fn(Name, Json);
	}

	JsonObject __stdcall GetJson(const char* Name)
	{
		UPDCHK NullJsonObject;
		UPDFN(GetJson) NullJsonObject;
		return fn(Name);
	}

	ExportOffsets* Ofses{ nullptr };

	ExportOffsets* __stdcall GetExportData()
	{
		if (Ofses)return Ofses;
		UPDCHK nullptr;
		UPDFN(GetExportData) nullptr;
		return Ofses = fn();
	}

	void __stdcall SetBackUp(DWORD Addr, int Size)
	{
		UPDCHK;
		UPDFN(SetBackUp);
		fn(Addr, Size);
	}

	void __stdcall RestoreBackUp(DWORD Addr)
	{
		UPDCHK;
		UPDFN(RestoreBackUp);
		fn(Addr);
	}

	const char* GetSettingText(const LibRemoteData& Lib)
	{
		return Lib.SettingText;
	}
	/*
	JsonObject GetSettingJson(const LibRemoteData& Lib)
	{
		UpdateJson(Lib.LibName, Lib.SettingText);
		return GetJson(Lib.LibName);
	}*/

	void ReplaceCallFunctionAt(DWORD Address, DWORD Func)
	{
		RestoreBackUp(Address);
		SetBackUp(Address, 4);
		*((DWORD*)Address) = Func - (Address - 1) - 5;
	}

	void RestoreCallFunctionAt(DWORD Address)
	{
		RestoreBackUp(Address);
	}

	bool IsHookValid(DWORD Address, const std::string& Lib, const std::string& Proc)
	{
		auto it = AddrMap.find(Address);
		if (it == AddrMap.end())return false;
		if (!GetHookData(Lib, Proc))return false;
		for (int i = 0; i < it->second->HookCount; i++)
		{
			if (GetHookID(Lib, Proc) == it->second->HookIdx(i))
			{
				auto pLib = GetLibData(Lib);
				if (!pLib)return false;
				HMODULE hLib = LoadLibraryA(pLib->AbsPath);
				if (!hLib)return false;
				auto pProc = GetProcAddress(hLib, Proc.c_str());
				return pProc ? true : false;
			}
		}
		return false;
	}

	void ReplaceHookFunction(DWORD Address, const std::string& Lib, const std::string& Proc, DWORD pFunc)
	{
		auto it = AddrMap.find(Address);
		if (it == AddrMap.end())return;
		for (int i = 0; i < it->second->HookCount; i++)
		{
			if (GetHookID(Lib, Proc) == it->second->HookIdx(i))
			{
				auto pOfs = GetExportData();
				if (!pOfs)continue;
				AddrHiddenHeader* pHidden = (AddrHiddenHeader*)(it->second->HookDataAddr - sizeof(AddrHiddenHeader));
				auto pBegin = pHidden->OpCodeAddress + pOfs->CodeSize * i;
				DWORD OldProtect;
				VirtualProtect(pBegin, pOfs->CodeSize, PAGE_EXECUTE_READWRITE, &OldProtect);
				if (*pBegin != 0x90)//NOP
				{
					ReplaceCallFunctionAt((DWORD)pBegin + pOfs->CallOfs, pFunc);
				}
				VirtualProtect(pBegin, pOfs->CodeSize, OldProtect, &OldProtect);
			}
		}
	}

	void RestoreHookFunction(DWORD Address, const std::string& Lib, const std::string& Proc)
	{
		auto it = AddrMap.find(Address);
		if (it == AddrMap.end())return;
		for (int i = 0; i < it->second->HookCount; i++)
		{
			if (GetHookID(Lib, Proc) == it->second->HookIdx(i))
			{
				auto pOfs = GetExportData();
				if (!pOfs)continue;
				AddrHiddenHeader* pHidden = (AddrHiddenHeader*)(it->second->HookDataAddr - sizeof(AddrHiddenHeader));
				auto pBegin = pHidden->OpCodeAddress + pOfs->CodeSize * i;
				DWORD OldProtect;
				VirtualProtect(pBegin, pOfs->CodeSize, PAGE_EXECUTE_READWRITE, &OldProtect);
				if (*pBegin != 0x90)//NOP
				{
					RestoreCallFunctionAt((DWORD)pBegin + pOfs->CallOfs);
				}
				VirtualProtect(pBegin, pOfs->CodeSize, OldProtect, &OldProtect);
			}
		}
	}

	void DisableOpCode(DWORD Addr)
	{
		auto pa = GetOpCode(Addr);
		SetBackUp((DWORD)pa.Data, pa.N);
		DWORD OldProtect;
		VirtualProtect((LPVOID)pa.Data, pa.N, PAGE_EXECUTE_READWRITE, &OldProtect);
		memset((LPVOID)pa.Data, 0x90, pa.N);
		VirtualProtect((LPVOID)pa.Data, pa.N, OldProtect, &OldProtect);
	}

	void EnableOpCode(DWORD Addr)
	{
		auto pa = GetOpCode(Addr);
		RestoreBackUp(Addr);
	}

	void EnableHook(DWORD Address, const std::string& Lib, const std::string& Proc)
	{
		RestoreHookFunction(Address, Lib, Proc);
	}

	void DisableHook(DWORD Address, const std::string& Lib, const std::string& Proc)
	{
		ReplaceHookFunction(Address, Lib, Proc, NullOutput);
	}

	void EnableAllHooksAt(DWORD Address)
	{
		auto it = AddrMap.find(Address);
		if (it == AddrMap.end())return;
		auto pOfs = GetExportData();
		if (!pOfs)return;
		DWORD OldProtect;
		AddrHiddenHeader* pHidden = (AddrHiddenHeader*)(it->second->HookDataAddr - sizeof(AddrHiddenHeader));
		for (int i = 0; i < it->second->HookCount; i++)
		{
			auto pBegin = pHidden->OpCodeAddress + pOfs->CodeSize * i;
			VirtualProtect(pBegin, pOfs->CodeSize, PAGE_EXECUTE_READWRITE, &OldProtect);
			if (*pBegin != 0x90)//NOP
				RestoreCallFunctionAt((DWORD)pBegin + pOfs->CallOfs);
			VirtualProtect(pBegin, pOfs->CodeSize, OldProtect, &OldProtect);
		}
	}

	void DisableAllHooksAt(DWORD Address)
	{
		auto it = AddrMap.find(Address);
		if (it == AddrMap.end())return;
		auto pOfs = GetExportData();
		if (!pOfs)return;
		DWORD OldProtect;
		AddrHiddenHeader* pHidden = (AddrHiddenHeader*)(it->second->HookDataAddr - sizeof(AddrHiddenHeader));
		for (int i = 0; i < it->second->HookCount; i++)
		{
			auto pBegin = pHidden->OpCodeAddress + pOfs->CodeSize * i;
			VirtualProtect(pBegin, pOfs->CodeSize, PAGE_EXECUTE_READWRITE, &OldProtect);
			if (*pBegin != 0x90)//NOP
				ReplaceCallFunctionAt((DWORD)pBegin + pOfs->CallOfs, NullOutput);
			VirtualProtect(pBegin, pOfs->CodeSize, OldProtect, &OldProtect);
		}
	}

	struct PlainRegisters
	{
		DWORD origin;
		DWORD flags;
		DWORD EDI;
		DWORD ESI;
		DWORD EBP;
		DWORD ESP;
		DWORD EBX;
		DWORD EDX;
		DWORD ECX;
		DWORD EAX;
	};

	
	HookContext::HookContext(HookContext&& rhs) :pReg(nullptr), Stack()
	{
		std::swap(pReg, rhs.pReg);
		std::swap(Stack, rhs.Stack);
	}
	HookContext::HookContext(REGISTERS* Reg) :pReg(Reg), Stack()
	{

	}
	HookContext::~HookContext()
	{
		if (pReg)delete pReg;
		if (Stack.N && Stack.Data)delete[]Stack.Data;
	}
	HookContext::HookContext(size_t StackSize, size_t Top)
	{
		Stack.N = StackSize;
		Stack.Data = new BYTE[StackSize];
		auto pPlain = new PlainRegisters;
		memset(pPlain, 0, sizeof(pPlain));
		pPlain->EBP = Stack.N + (DWORD)Stack.Data;
		pPlain->ESP = pPlain->EBP - Top;
		pReg = (REGISTERS*)pPlain;
	}
	DWORD HookContext::CallHook(HookType Proc) const
	{
		if (!Proc)return 0;
		return Proc(pReg);
	}
	DWORD HookContext::CallHook(DWORD Address, const std::string& Lib, const std::string& Proc) const
	{
		auto pLib = GetLibData(Lib);
		if (!pLib)return UINT_MAX;
		HMODULE hLib = LoadLibraryA(pLib->AbsPath);
		if (!hLib)return UINT_MAX;
		HookType pProc = (HookType)GetProcAddress(hLib, Proc.c_str());
		if (!pProc)return UINT_MAX;
		return pProc(pReg);
	}

	HookType GetHookProc(DWORD Address, const std::string& Lib, const std::string& Proc)
	{
		auto pLib = GetLibData(Lib);
		if (!pLib)return nullptr;
		HMODULE hLib = LoadLibraryA(pLib->AbsPath);
		if (!hLib)return nullptr;
		return (HookType)GetProcAddress(hLib, Proc.c_str());
	}

	//BYTE const hook_jmp[5] = { 0xE9, _INIT, _INIT, _INIT, _INIT };
	void ReplaceFuncBody(DWORD Addr,DWORD Proc)
	{
		RestoreBackUp(Addr);//这个是覆盖式的
		SetBackUp(Addr, 5);
		DWORD OldProtect;
		VirtualProtect((LPVOID)Addr, 5, PAGE_EXECUTE_READWRITE, &OldProtect);
		*((LPBYTE)Addr) = 0xE9;//JMP
		*((LPDWORD)(Addr+1)) = Proc;
		VirtualProtect((LPVOID)Addr, 5, OldProtect, &OldProtect);
	}

	void RestoreFuncBody(DWORD Addr)
	{
		RestoreBackUp(Addr);
	}
}	
