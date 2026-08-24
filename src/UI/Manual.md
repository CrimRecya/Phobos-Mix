# UIExt 通用 UI 框架 · 使用指南

## 1. 概览

框架位于 `src/UI/`，基于引擎原生 `GadgetClass`/`GScreenClass` 体系，分三层：

| 层 | 文件 | 职责 |
|---|---|---|
| 宿主 | `UIRoot.h` | 单例，管理所有打开的 Screen、模态、每帧刷新、快捷键、Tooltip |
| 组件 | `UIComponent.h` | 控件基类：组合树、锚点布局、事件、MVVM 绑定入口 |
| 数据 | `Mvvm.h` | `Observable` / `ObservableVector` / `Command` |
| 控件 | `Controls/` | Panel、Dialog、Button、IconButton、CheckBox、Label、PageView、ListGrid、IconStrip、Tooltip |
| 辅助 | `Builder.h` / `Layout.h` | 声明式工厂 + 行/列/网格排列 |

主循环接线已经完成（`src/Misc/MessageColumn.cpp` 每帧驱动 `UpdateAndDraw`、`src/Commands/Commands.cpp` 滚轮、`src/Ext/Sidebar/Hooks.cpp` 输入拦截），**使用时不需要碰任何 hook**。

## 2. 快速上手

最小可用的模态对话框：

```cpp
#include <UI/Builder.h>
#include <UI/UIRoot.h>

void OpenSimpleDialog()
{
    auto dialog = UIExt::Builder::MakeDialog(200, 150, 400, 300, L"标题");
    dialog->SetAnchor(UIExt::Anchor::Center);          // 屏幕居中，每帧自动维持

    auto label = UIExt::Builder::MakeLabel(20, 40, L"你好，世界");
    dialog->AddChild(std::move(label));

    auto btn = UIExt::Builder::MakeButton(150, 240, 100, 30, L"关闭");
    auto* rawDialog = dialog.get();
    btn->SetOnClick([rawDialog]()
    {
        UIExt::UIRoot::Instance().Close(rawDialog);    // 延迟一帧销毁，回调里安全
    });
    dialog->AddChild(std::move(btn));

    // Open 负责注册进 GScreenClass；此后不要再用 unique_ptr 操作该树
    UIExt::UIRoot::Instance().Open(std::move(dialog), UIExt::ModalLevel::BlockTactical);
}
```

推荐结构（参照 `src/Mutation/`）：**ViewModel 持有 `Observable`/`Command`，工厂函数 `Create(ViewModel&)` 负责拼控件树，调用方 `Open`**。数据与视图分离，跟样例一样。

## 3. 核心功能

本节按主题覆盖框架的各项功能细节，可按需跳读。

### 3.1 Screen 生命周期与模态级别

```cpp
UIRoot::Instance().Open(std::unique_ptr<UIComponent> root, ModalLevel modal);
UIRoot::Instance().Close(UIComponent* root);   // 延迟到下一帧，防回调中自毁
UIRoot::Instance().CloseAll();
```

| ModalLevel | 效果 |
|---|---|
| `None` | 纯悬浮 UI，不拦截游戏输入（如右侧 MutationDisplayerStrip） |
| `BlockArea` | 只拦截自身矩形内的点击，矩形画 40% 黑色蒙版 |
| `BlockTactical` | 拦截战术区/侧边栏/命令点击，强制鼠标指针为默认；无蒙版 |
| `BlockFullScreen` | 拦截全部输入 + 禁用其它所有 Screen + 80% 全屏蒙版 |

- `Close()` 是**延迟销毁**：压入 `PendingClose_`，下一帧才 `OnDestroy` + 反注册 + 析构。回调里可以安全关掉自己的对话框；但 `Close` 之后不要再访问该指针。
- 判断是否有 UI 挡住某点：`IsBlockingAt(x, y)`；是否有全屏 UI：`IsBlockingFullScreen()`。

### 3.2 控件 API 速查

控件级 `Set*` 为流式 API（返回自身引用，可链式调用）；基类 `UIComponent` 的通用 setter 返回 `void`，不支持链式。坐标一律是**相对父控件**的（根组件相对屏幕）。

**通用（`UIComponent`）**：`SetPos / SetSize / SetVisible / SetEnabled / SetTooltip(title, text) / SetTooltipPadding / SetTooltipLineSpacing / SetAnchor / SetAnchorOffset / SetRelativePosition / AddChild`，事件 `SetOnMouseEnter / SetOnMouseLeave / SetOnAction(flags, pKey, modifier)`。

| 控件 | 关键 API |
|---|---|
| `Panel` | `SetBackColor(color, opacity)`、`SetBorder(bool, color)`、`SetCustomDraw(fn)`（自绘 SHP/PCX 底图，见样例） |
| `Dialog` | 继承 Panel；`SetTitle`、`SetCloseAction(fn)`（右上角内置关闭按钮） |
| `Button` | `SetText / SetIcon / SetOnClick / OnClick(流式别名) / SetOnRightClick / SetShortcut(VK_*) / SetTextOffset / SetTextAnchor / BindIcon` |
| `IconButton` | 继承 Button；`SetIconSurface(BSurface*)`，无图标时可用 `SetText` 显示文字（如首字，截取由调用方完成） |
| `CheckBox` | `SetChecked / SetOnToggle(fn(bool))` |
| `Label` | `SetText / SetColor / SetLineSpacing`，支持 `\n` 多行 |
| `PageView` | `SetGrid(列,行,项宽,项高,gapX,gapY)` 分页网格；`NextPage/PrevPage/SetPage/SetOnPageChanged(fn)`；状态查询 `GetPageCount/GetPageIndex/CanNext/CanPrev`；滚轮悬停自动翻页 |
| `ListGrid` | `SetColumns(n, 项宽, 项高)` 固定列网格 |
| `IconStrip` | `SetItemSize / SetSpacing`；高度随条目数自动增长 |

Tooltip 只要 `SetTooltip(标题, 正文)` 即可，支持 `\n`，悬停自动显示，无需额外代码。

### 3.3 布局

- **相对坐标**：`AddChild` 自动记录 `RelativeX/Y`，父组件移动后每帧由 `UpdateTreePositions` 级联同步，子控件跟随。
- **锚点**：`SetAnchor(UIExt::Anchor::Center)`（九宫格 + 偏移 `SetAnchor(a, dx, dy)`）。无父时相对屏幕 `ViewBounds`——**分辨率/窗口变化时自动维持位置**，常驻 UI（如右侧 Strip）必备。
- **手排**：`Layout::ArrangeRow / ArrangeColumn / ArrangeGrid`（静态方法，排已有指针数组）。
- **容器自排**：PageView/ListGrid/IconStrip 调 `Refresh()` 重排；增删子节点后需调用。

### 3.4 MVVM 数据绑定

**`Observable<T>`（拉模型）**：`Set()` 只递增版本号，**下一帧** `UIRoot::FlushBindings` 才把新值应用进控件——回调链中不会重入：

```cpp
struct MyViewModel {
    UIExt::Observable<std::wstring> ConfirmText { L"确认" };
    UIExt::Observable<bool> Ready { false };
    UIExt::Command CommitCommand;   // 构造时给 Execute/CanExecute
};
```

绑定方式（任何控件）：

```cpp
label->BindText(viewModel.ConfirmText);          // 控件专属
panel->BindVisible(viewModel.Ready);             // 通用：Visible/Enabled/TooltipTitle/TooltipText
checkBox->BindChecked(viewModel.SomeFlag);
button->BindCommand(viewModel.CommitCommand);    // 见下

// 万能形式：任意 Observable<T> -> 任意 setter
component->BindValue(viewModel.SelectedIDs, [checkBox, id](const std::vector<int>& ids)
{
    checkBox->SetChecked(std::find(ids.begin(), ids.end(), id) != ids.end());
});
```

**`Command` + `BindCommand`**：按钮点击执行 `Execute`，`CanExecute` 为 false 时自动禁用；改完条件后调 `command.NotifyCanExecuteChanged()` 让按钮刷新状态。运行期可 `SetExecute / SetCanExecute` 替换逻辑（`SetCanExecute` 内部会自动触发一次通知）。注意：**`BindCommand` 会覆盖 `SetOnClick` 并接管 `Enabled`，二者别混用**。

**`ObservableVector<T>`**：列表数据。修改接口 `SetItems / Add / RemoveAt / Clear`，任一变更都会递增版本并推送通知（`SetOnChanged(fn)`），配合重建逻辑（见 §3.5）。

### 3.5 动态增删子控件

**已打开的 Screen** 必须走这对 API（内部做 `GScreenClass` 注册/反注册）：

```cpp
UIRoot::Instance().AddRootChild(root, std::move(child));    // 自动 OnCreate + 注册
UIRoot::Instance().RemoveRootChild(root, child);            // 自动 OnDestroy + 反注册 + 移除
```

`MutationDisplayerStrip::Refresh` 就是标准范式：循环 `RemoveRootChild` 清空 → 按数据重建 → `strip->Refresh()`。

⚠️ **`RebuildItems` 只能在 Open 之前用**：它直接 `clear()` 子节点而不做反注册，对已打开的 Screen 调用会在 `GScreenClass` 留下悬空指针导致崩溃。构建期（`Create` 工厂内、`Open` 前）随意用，运行期换 `Add/RemoveRootChild`。

### 3.6 快捷键

```cpp
button->SetShortcut(VK_RETURN);    // 或 WWKey 枚举
```

- 由 `UIRoot::HandleShortcuts` 每帧边沿检测（按下瞬间触发，已按住不连发）
- 顶层 Screen 优先，同帧只响应一个键
- 触发等价于 `button->Click()`（走 `OnClick`/绑定的 Command），与鼠标点击同一路径

## 4. 自定义控件

继承 `UIComponent`（或现有控件），按需重写：

```cpp
class MyGauge : public UIExt::UIComponent {
public:
    using UIComponent::UIComponent;
    void OnDraw() override { /* DSurface::Composite 上画 */ }
    void OnUpdate() override { /* 每帧逻辑 */ }
    void OnCreate() override { }   // Open 时调用（先父后子）
    void OnDestroy() override { }  // Close 时调用（先子后父）
};
```

- 绘制目标固定 `DSurface::Composite`，裁剪自己传 `RectangleStruct`
- 命中/悬停/输入分发由 `GadgetClass` 体系自动处理，只需关注 `OnDraw/OnUpdate`
- 需要被 `UIRoot` 特殊识别时重写 `IsDialog/IsPageView/IsButton`

## 5. 注意事项（踩坑清单）

1. **`Open` 之后 relinquish 所有权**：树归 `UIRoot` 管，只保留裸指针用于 `Close`/动态操作。
2. **`Close` 延迟一帧**：回调里安全自杀，但 `Close` 后别再解引用。
3. **`RebuildItems` 的注册坑**：见 §3.5，运行期禁用。
4. **`Observable` 生命周期**必须 ≥ 绑定它的控件（ViewModel 要比 Dialog 活得久，常驻或按需重建都行）。
5. **绑定是下一帧生效**：`Set` 之后立刻读控件状态会拿到旧值；同帧多处 `Set` 也会合并且只应用一次。
6. **`BindCommand` 与 `SetOnClick` 互斥**，且它接管 `Enabled`。
7. **PageView 非当前页的子控件**被移到 `(-10000,-10000)` 并禁用（防抢鼠标输入），依赖子节点顺序分页——插删节点后要 `Refresh()`。
8. **输入锁**：引擎 `UserInputLocked` 时所有控件不可交互（`CanInteract` 内置判断）。

## 6. 参考样例

| 样例 | 演示内容 |
|---|---|
| `src/Mutation/MutationSelectorDialog.cpp` | 完整 MVVM：Dialog + PageView 分页 + Command 按钮联动 + 双向同步 + 快捷键 + `SetCustomDraw` 画 SHP |
| `src/Mutation/MutationDisplayerStrip.cpp` | 常驻 IconStrip：Anchor::Right + `Observable` 驱动的运行期动态增删 |
