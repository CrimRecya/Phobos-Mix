# 通用 UI 框架实现计划

日期：2026-08-17
关联设计文档：`docs/superpowers/specs/2026-08-17-ui-framework-design.md`
状态：待执行

## 1. 目标

实现一个主要面向 C++ 功能开发者的通用 UI 框架，并完成首个真实案例“突变因子选择器 + 显示器”。第一版不重构现有 SelectedInfo / SWSidebar / UniqueButton / MessageColumn。

## 2. 里程碑

- **M0：框架地基**
  - MVVM-lite 基础（Observable / ObservableVector / Command / Binding）
  - UIComponent 基类和 UIRoot 生命周期
  - 主循环更新/绘制、输入阻断集成
- **M1：基础控件库**
  - Panel/Dialog、Button/IconButton、CheckBox、Label、Tooltip、PageView、IconStrip、List/Grid
  - Builder 声明式 API
  - 布局容器 Row/Column/Grid/Anchor
- **M2：首个案例**
  - 突变因子 ViewModel + 选择器 Dialog + 显示器 IconStrip
  - Debug 命令打开 UI
  - 手工测试清单执行

## 3. 详细任务

### M0：框架地基

1. 新建 `src/UI/` 目录。
2. 实现 `src/UI/Mvvm.h/cpp`
   - `template <typename T> class Observable`
     - 成员：`T Value`、`std::vector<BindingToken*> Bindings`、`bool Dirty`
     - `T Get() const`
     - `void Set(const T& value)`
     - `void MarkDirty()` / `bool ConsumeDirty()`
   - `template <typename T> class ObservableVector`
     - 持有 `std::vector<T> Items`
     - `SetItems / Add / Remove / Clear`
     - 变化时通知容器：`OnChanged` 回调或 `Dirty` 标记
   - `class Command`
     - `std::function<void()> Execute`
     - `std::function<bool()> CanExecute`
     - `bool CanExecuteNow() const`
     - `void NotifyCanExecuteChanged()`
   - `class Binding`
     - 绑定一个控件属性到 `Observable*` 或 lambda 读取器
     - `Apply()` 把 Observable 当前值写入控件
     - 控件销毁时自动 `Unbind()`
3. 实现 `src/UI/UIComponent.h/cpp`
   - `class UIComponent : public GadgetClass`
   - 公共字段：`UIComponent* Parent`、`bool Visible`、`bool Enabled`、`std::wstring TooltipTitle/TooltipText`
   - 生命周期：`OnCreate` / `OnDestroy` / `OnDraw` / `OnUpdate`
   - 输入回调：`OnMouseEnter` / `OnMouseLeave` / `OnAction`，内部转发给用户 lambda
   - 绑定容器：`std::vector<std::unique_ptr<Binding>> Bindings`
   - 销毁时解绑并清理子控件
4. 实现 `src/UI/UIRoot.h/cpp`
   - `static UIRoot& Instance()`
   - `void Open(UIRoot::ScreenOptions options, std::unique_ptr<UIComponent> root)`
   - `void Close()`
   - `bool IsModal() const`
   - `void UpdateAndDraw()`
     - flush 脏绑定
     - 绘制 Tooltip
     - 绘制可选模态遮罩
   - 维护当前打开的控件树，注册/注销到 `GScreenClass::Instance`
5. 主循环集成
   - 在 MainLoop 附近挂一个 hook 调用 `UIRoot::Instance().UpdateAndDraw()`
   - 如果目标地址与现有 hook 冲突，扩展现有 `MainLoop_FrameStep_NewMessageListManage` 或选择相邻空闲地址
6. 输入阻断集成
   - 修改 `src/Ext/Sidebar/Hooks.cpp` 的 `DisplayClass_ProcessClickCoords_SkipOnNewButtons`
     - 条件增加 `UIRoot::Instance().IsModalBlockingTactical()` 或具体 modal level
   - 修改 `src/Commands/Commands.cpp` 的 `CheckSkipScrollSidebar()`
     - 条件增加 UIRoot 模态判断
7. 工程文件
   - 在 `Phobos.vcxproj` 注册所有新增 `.cpp` / `.h`
   - 编译通过

### M1：基础控件库

1. 布局容器
   - `Layout.h/cpp`
     - `RowLayout` / `ColumnLayout`
     - `GridLayout`
     - `PageView`
     - `Anchor`：Center / Left / Right / Top / Bottom / BottomLeft 等
   - 提供纯 C++ 坐标计算函数，便于单测
2. 控件实现（每个都在 `src/UI/Controls/` 下）
   - `Panel`：背景绘制，支持 SHP/PCX 和纯色/透明背景
   - `Dialog`：继承 Panel；标题栏、关闭按钮、模态级别
     - `enum class ModalLevel { None, BlockArea, BlockTactical, BlockFullScreen }`
     - `Esc` 关闭
   - `Button`：左键/右键、悬停、禁用态、文字/图标、快捷键、Tooltip
   - `IconButton`：图标按钮，面向技能栏/突变图标
   - `CheckBox`：勾选态、文字、绑定 `Observable<bool>`
   - `Label`：单行/多行文字、CSF 本地化
   - `Tooltip`：不是独立控件，而是由 `UIRoot` 绘制；`UIComponent` 提供挂载点
   - `IconStrip`：锚点 + 图标列，绑定 `ObservableVector`，悬停显示 Tooltip
   - `ListGrid`：绑定 `ObservableVector<T>` 和 `ItemBuilder`
3. Builder API
   - `Builder.h`
   - 例如：
     ```cpp
     UI::Panel("突变因子")
         .SetSize(640, 480)
         .Anchor(UI::Anchor::Center)
         .AddChild(UI::Button("确认").OnClick(...));
     ```
   - 只做链式设置，不做重模板元编程
4. 每个控件完成以下通用属性：
   - `SetPos / SetSize / SetAnchor`
   - `SetVisible / SetEnabled`
   - `SetTooltip(title, text)`
   - `BindText / BindChecked / BindVisible / BindIcon`
5. 编译通过，并做基础手工验证（打开一个空 Dialog、一个按钮、一个 Tooltip）

### M2：首个案例

1. 设计互操作接口
   - 在 `src/Mutation/MutationInterop.h/cpp` 中定义 C++ 侧接口
   - C# DynamicPatcher 侧注册委托：
     - 查询可选突变列表：ID/Name/Description/Score/Icon 标识
     - 查询已激活/已选突变列表
     - 激活/取消突变
     - 提交/确认选择
   - C# 状态变化时 P/Invoke 通知 Phobos `MutationInterop`，刷新 ViewModel
   - 复用现有 `Projects/InteropUtils/Phobos/` 回调注册模式，不在 C# 侧实现 UI
2. 定义 C++ 侧突变因子数据模型
   - `MutationInfo { int ID; std::wstring Name; std::wstring Description; int Score; int IconIndex; }`
   - 第一版数据来自 `MutationInterop`；互操作不可用时可以使用临时静态数据源，便于 UI 独立联调
3. 实现 `src/Mutation/MutationViewModel.h/cpp`
   - `ObservableVector<MutationInfo> Mutations`
   - `Observable<int> PageIndex`
   - `Observable<int> MaxSelection`（可选）
   - `Observable<std::vector<int>> SelectedIDs`
   - `Observable<std::wstring> ConfirmText`
   - `Command ToggleSelect`
   - `Command PageNext / PagePrev`
   - `Command Confirm / Close`
4. 实现 `src/Mutation/MutationSelectorDialog.h/cpp`
   - 使用 `Dialog` + `PageView` + `GridLayout` + `CheckBox` + `Button`
   - 分页翻页时重建当前页
   - 已选数量变化时刷新标题栏和确认按钮
5. 实现 `src/Mutation/MutationDisplayerStrip.h/cpp`
   - 使用 `IconStrip`，绑定 `SelectedIDs`
   - 悬停显示 `MutationInfo` 的 Tooltip
6. 添加 Debug 命令
   - `src/Mutation/OpenMutationUI.h/cpp`
   - `CommandClass` 打开突变因子选择器
   - 在 `src/Commands/Commands.cpp` 注册
7. 联动
   - 选择器确认后关闭 Dialog，并通过 `MutationInterop` 把选择提交给 DynamicPatcher
   - C# 侧确认/激活变化后通知 Phobos，`MutationViewModel` 更新，显示器随之刷新
   - 显示器是常驻 UI，不随 Dialog 关闭而销毁
8. 清理 C# 侧旧 UI 副作用（后续逐步做）
   - 抽出 `Mutator.Init/Uninit` 中直接操作 `SidebarClass` / `MessageListClass` 的代码
   - 避免 C++ UI 和 C# 旧逻辑重复操作同一界面
9. 工程文件
   - 注册 `src/Mutation/` 下新增 `.cpp` / `.h`
   - 编译通过
10. 手工测试清单执行
    - 见下方“测试清单”

## 4. 文件清单（新增/修改）

新增：

```text
src/UI/
  UIComponent.h/cpp
  UIRoot.h/cpp
  Builder.h
  Layout.h/cpp
  Mvvm.h/cpp
  Controls/
    Panel.h/cpp
    Dialog.h/cpp
    Button.h/cpp
    CheckBox.h/cpp
    Label.h/cpp
    Tooltip.h/cpp
    PageView.h/cpp
    IconStrip.h/cpp
    ListGrid.h/cpp

src/Mutation/
  MutationInterop.h/cpp
  MutationViewModel.h/cpp
  MutationSelectorDialog.h/cpp
  MutationDisplayerStrip.h/cpp
  OpenMutationUI.h/cpp
```

修改：

```text
Phobos.vcxproj
src/Ext/Sidebar/Hooks.cpp        // 输入阻断增加 UIRoot
src/Commands/Commands.cpp        // 滚轮/输入阻断增加 UIRoot；注册 Debug 命令
src/Commands/Commands.h          // 如果需要命令宏/头文件引用
主循环 hook 所在文件             // UIRoot::Instance().UpdateAndDraw()

DynamicPatcher（外部 C# 工程，单独提交）
Projects/Extension/Mutators/Mutator.cs           // 逐步移除 Sidebar/MessageList 直接副作用
Projects/Extension/Mutators/MutatorRandomizer.cs // 补充公开查询接口（如可选列表）
Projects/InteropUtils/Phobos/*.cs                // 新增委托注册 + P/Invoke 通知
```

## 5. 依赖关系

- M0 是 M1 的前置依赖。
- M1 的 Dialog/PageView/CheckBox/IconStrip 是 M2 的前置依赖。
- M2 可以并行做数据模型和 Debug 命令，但 UI 组装依赖 M1。

## 6. 风险与对策

- **GadgetClass 生命周期复杂**
  - 对策：所有创建/删除集中在 `UIRoot`，控件不允许自行 AddButton/RemoveButton。
- **模态阻断影响现有 UI**
  - 对策：默认 `ModalLevel::BlockTactical`，只阻断战术地图/侧边栏滚动，不屏蔽其它 UI。
- **快捷键冲突**
  - 对策：Debug 命令先不绑默认键，后续快捷键统一登记并查重日志。
- **每帧 flush 与绘制时序**
  - 对策：先标脏、下一帧绘制前统一应用，避免在绘制过程中改控件。
- **素材缺失**
  - 对策：所有 SHP/PCX 加载都走降级路径：空控件或占位色块 + 日志。
- **C++ / C# 互操作**
  - 对策：互操作只传纯数据（ID/字符串/int），不在边界传递 C++ 对象或 C# 对象；回调必须用静态字段持有，防止 GC 回收。

## 7. 测试清单

1. 打开/关闭 Dialog，`Esc` 关闭生效。
2. 模态级别：
   - `None`：对话框外仍可操作战术地图。
   - `BlockTactical`：不能点地图/滚侧边栏，但 IconStrip 仍可交互。
   - `BlockFullScreen`：只能操作当前 Dialog。
3. 分页：
   - 第一页时“上一页”禁用，最后一页时“下一页”禁用。
   - 页码文字正确。
4. 多选：
   - CheckBox 勾选/取消正确。
   - 标题栏已选数量正确。
   - 确认按钮在无选择时禁用（如果限制为至少 1 个）。
5. IconStrip：
   - 确认后右侧图标出现。
   - 悬停 Tooltip 显示名称/描述。
6. 分辨率：
   - 分别测试 4:3 和 16:9，Dialog 居中、IconStrip 靠右、不越界。
7. 素材缺失：
   - 删除某个图标/背景素材后不崩溃，显示占位。
8. 与旧 UI 共存：
   - 打开突变因子 Dialog 时，SelectedInfo/SWSidebar 仍不崩溃；模态只按配置阻断地图输入。

## 8. 完成标准

- M0/M1/M2 全部编译通过。
- 突变因子选择器和显示器可被 Debug 命令打开，交互符合测试清单。
- 设计文档中的非目标没有被突破（不做文本框、滚动条、完整 MVVM、弹性布局）。