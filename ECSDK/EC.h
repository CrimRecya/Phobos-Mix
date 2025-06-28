#pragma once

/*
万物互通 SDK

2024.12

这套SDK整合了有关IHCore SyringeIH SIWIC三个来源的接口
同时包含了一个公开库cJson作为数据交换格式
*/

/*
cJSON是Dave Gamble开发的一个Json解析库。
ExtJson.h对cJson做了C++的简单包装。
https://github.com/DaveGamble/cJSON
*/
#include "ExtJson.h"

/*
SyringeIH 是 钢铁之锤 对Syringe的升级。
SyringeEx.h里面包含了其支持的接口。
https://github.com/IronHammer-Std/Syringe-IH
（最新源码最好看一下万物互通QQ群里面最新开发包当中的）
此项目由Ares-Developers 的 Syringe0.7.2.0 升级而来
https://github.com/Ares-Developers/Syringe
*/
#include "SyringeEx.h"

/*
IH 是 钢铁之锤 开发的YR引擎扩展。
*/
#include "IH.h"

/*
EC的一些综合接口。
*/
#include "EC.Misc.h"
#include "EC.Listener.h"

/*
一组检测某些特定的组件是否在工作的API。
如果返回true，表明相关的功能能够正常工作。

cJSON.h和ExtJson.h无依赖。
SyringeEx.h的接口依赖SyringeIH
IH.xxx.h的接口依赖IH组件
WIC.xxx.h的接口依赖WIC组件

在DllMain的Init::Initialize()后 HasSyringeIH 和 HasIH 可以工作
在EC初始化后 HasWIC 可以工作
*/
bool HasSyringeIH();
int SyringeIHVer();
bool HasIH();
#ifndef SIWIC
bool HasWIC();
int WICVer();
#endif


/*
获取EC的载入状态
函数始终可用
当返回值为Ext::ECLoadStage::InitComplete时
EC载入正式完成
*/
inline Ext::ECLoadStage GetECLoadStage()
{
    return Ext::GetLoadStage();
}


/*

初始化函数 在DllMain的DLL_PROCESS_ATTACH阶段调用

*/
bool ECInitLibrary(
    const char* LibraryName, //库名
    int Version,
    int LowestSupportedVersion,
    UTF8_CString Description,
    const std::function<void()>& OnFirstInit,
    const std::function<void()>& OnOrderedInit,
    std::initializer_list<InitDependency> Dependencies = {}
);
bool ECInitLibrary(
    const char* LibraryName, //库名
    int Version,
    int LowestSupportedVersion,
    UTF8_CString Description,
    const std::function<void()>& OnFirstInit,
    const std::function<void()>& OnOrderedInit,
    GetFunc_t GetFunc,
	std::initializer_list<InitDependency> Dependencies = {}
);
bool ECInitLibrary(
    const char* LibraryName, //库名
    int Version,
    int LowestSupportedVersion,
    UTF8_CString Description,
    const std::function<void()>& OnFirstInit,
    const std::function<void()>& OnOrderedInit,
    const std::unordered_map<std::string, LibFuncHandle>& ExportFuncs,
    std::initializer_list<InitDependency> Dependencies = {}
);

/*
结构：
现有的头文件
cJSON.h->ExtJson.h->SyringeEx.h->
                IH.Initial.h->IH.File.h  ->IH.Ext.h
                              IH.Loader.h->IH.InitialService.h                        ->IH.h  
                                              WIC.Define.h->WIC.Interface.h              ->WIC.h
                                            WIC.Template.h->WIC.Buff.h->WIC.ExampleBuff.h
*/
