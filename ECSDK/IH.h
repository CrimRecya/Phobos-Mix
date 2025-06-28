/*

IH V0.4 SDK
引擎作者：钢铁之锤  QQ：2482911962
SDK作者：钢铁之锤  QQ：2482911962

IH 是 钢铁之锤 开发的YR引擎扩展,目前包括IHCore IHLibList 2个组件。
这是IH的配套SDK，提供了调用IH内部功能的接口。
如果您想要调用IH的接口，请在项目当中包含此SDK。
在把IH组件（IHCore.dll IHLibList.dll等）放置于游戏目录，
并确保Syringe识别了它们后，且正确地将您的DLL通过IHLoader注册到IH组件后，
此SDK当中的接口即为可用的。
详细的接口说明请查阅IH.Loader.h IH.File.h等头文件内接口处的注释。
如有问题或反馈请联系作者。

2024年12月

*/
#pragma once

#include "IH.Loader.h"

#include "IH.Ext.h"

#include "IH.InitialService.h"

#include "IH.File.h"

#include "IH.Config.h"
