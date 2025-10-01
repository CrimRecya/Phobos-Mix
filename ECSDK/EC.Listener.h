#pragma once
#include "IH.Loader.h"


struct ExceptionAnalyzer
{
	//行为受SyringeIH是否存在影响，可以通过 HasSyringeIH() 确定是否存在。
	void (CALLBACK* AnalyzeAddr)(DWORD /*In*/ Addr, size_t /*In*/ BaseDescBufSize, DWORD& /*Out*/ RelAddr, char* /*Out GBK*/ BaseDesc);
	void (CALLBACK* GetAccessStr)(LPVOID /*In*/ Addr, size_t /*In*/ BaseDescBufSize, char* /*Out GBK*/ AccessStr);
	bool (CALLBACK* IsAddrExecutable)(LPVOID /*In*/ Addr);
	void (CALLBACK* WriteToExceptIH)(const char* /*In GBK*/ Format, ...);

	int Reserved[4];
};

/*
前排：
EC监听器的实现还是初始服务接口
本质是一个叫做EC::Internal::ListenerAccess的服务
以后一些乱七八糟的功能也会用服务接口给
（或许这样初始服务看起来会不那么废物？）
*/

namespace ECListener
{
	void Set(const char* ListenerType, LibFuncHandle Function);

	LibFuncHandle Get(const char* ListenerType);

	void Remove(const char* ListenerType);

	std::vector<LibFuncHandle> GetAll(const char* ListenerType);

	//!!一个库每件事只能有一个监听器!!

	//在EXE的最开始，全局对象初始化完成，内存可分配，Winmain顶头位置
	//完成一些必要的初始化工作
	typedef void (CALLBACK* Listener_InitBeforeEverything)();
	void Listen_InitBeforeEverything(Listener_InitBeforeEverything Func);

	//在读入rules（如rulesmd.ini，地图文件）等时触发
	//Listen_LoadBeforeTypeData在原版的类型读取前触发
	//Listen_LoadAfterTypeData在原版的类型读取后触发
	//载入或更新设置
	typedef void (CALLBACK* Listener_OnLoadGame)(const CCINIClass* pIni);
	void Listen_LoadBeforeTypeData(Listener_OnLoadGame Func);
	void Listen_LoadAfterTypeData(Listener_OnLoadGame Func);

	//在触发FE时调用，入参为一组在异常状态下可以安全分析内存的函数
	//记录必要的错误信息，转存入except_ih.txt
	typedef void (CALLBACK* Listener_BeginWritingExceptIH)(const ExceptionAnalyzer& Anal);
	void Listen_BeginWritingExceptIH(Listener_BeginWritingExceptIH Func);

	//在载入CSF条目时调用，可以截留CSF的解析
	//如果想要修改解析的结果，可以返回非零值作为新的解析结果，要求返回的const wchar_t*是非临时值，生命期一直保持
	//监听函数返回nullptr，则返回原有查找策略
	typedef const wchar_t* (CALLBACK* Listener_LoadCSFString)(const char* pLabel);
	void Listen_LoadCSFString(Listener_LoadCSFString Func);

	//对局从INI载入时，载入过程当中触发
	typedef void (CALLBACK* Listener_LoadScenario)();
	void Listen_LoadScenario(Listener_LoadScenario Func);

	//从存档载入对局时，载入完成后将会调用
	// HasWIC() == true 时可以工作
	// 什么，不要问我为什么这个必样的函数执行的时候Swizzle怎么还没生效
	typedef void (CALLBACK* Listener_AfterLoadGame)();
	void Listen_AfterLoadGame(Listener_AfterLoadGame Func);

	//从INI、地图载入对局时，载入完成后将会调用
	// HasWIC() == true 时可以工作
	typedef void (CALLBACK* Listener_AfterLoadINI)();
	void Listen_AfterLoadINI(Listener_AfterLoadINI Func);

	//对局结束，清理数据时将会调用
	// HasWIC() == true 时可以工作
	typedef void (CALLBACK* Listener_ClearScenario)();
	void Listen_ClearScenario(Listener_ClearScenario Func);
}