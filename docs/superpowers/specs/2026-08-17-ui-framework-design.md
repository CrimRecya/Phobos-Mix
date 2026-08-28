# 通用 UI 框架设计（RA2/YR 引擎扩展）

日期：2026-08-17
状态：已与用户确认设计方向，待实现计划

## 1. 背景与目标

当前仓库中已有多个自定义游戏内 UI（底边栏选择信息、超级武器侧边栏、独特单位显示、中央消息栏），它们各自实现了 `InitClear` / `InitIO`、`GadgetClass` 子类、`DrawInfo` / `DrawShape` 等相似逻辑。每次新增一个 UI 都要重复处理控件生命周期、鼠标事件、绘制、输入阻断和状态刷新。

目标是设计一套**主要面向 C++ 功能开发者**的通用 UI 框架，让新机制可以更快地搭建“提示玩家/交互面板”，从而支撑更复杂的玩法设计。

已确认的四个目标场景：

1. 施法技能栏：底边栏扩展，显示技能图标，点击释放，支持指向型技能。
2. 突变因子选择器 + 显示器：前者是大对话框（分页多选），后者是屏幕右侧图标 + Tooltip。
3. 酒馆 UI：屏幕左侧按钮（升本、锁定等），选择单位后显示卡牌，卡牌包含描述、购买/出售按钮和快捷键。
4. 肉鸽选强化 UI：弹出页面，展示随机强化选项的描述、图标和选择按钮。

## 2. 范围

### 2.1 目标用户

- 主要使用者：本仓库的 C++ 功能开发者。
- 第一版不开放给不改代码的模组制作者做复杂 UI 配置。
- 将来可以在此基础上叠加数据驱动/配置层，但第一版不做。

### 2.2 首版组件

首版提供以下控件/能力：

- `Panel / Dialog`：背景、边框、标题栏、关闭按钮、模态选项。
- `Button / IconButton`：左/右键、悬停、禁用态、快捷键、图标/文字、点击回调。
- `CheckBox / Toggle`：多选/勾选态，用于突变因子选择等场景。
- `Label / Text`：单行/多行文字，支持 CSF 本地化。
- `Tooltip`：悬停显示标题 + 正文，由 UIRoot 统一绘制。
- `PageView / Pager`：分页网格，用翻页替代滚动条。
- `IconStrip`：屏幕右侧或任意锚点的持久图标列，用于显示器类 UI。
- `List / Grid 容器`：按行/列自动排列子控件。
- `MVVM-lite`：`Observable<T>` / `ObservableVector<T>` / `Binding` / `Command`。

### 2.3 非目标（第一版不做）

- 可输入文本框 / 可编辑文字。
- 可拖动滚动条。
- 完整 WPF 式 MVVM（反射、属性通知框架、DataTemplate）。
- 弹性布局 / 通用布局引擎。
- 动画系统 / 补间系统。
- 对现有四个自定义 UI 的强制性重构。

## 3. 总体架构

采用“在 RA2 原生 GScreen/GadgetClass 之上的声明式 C++ 组件库”，分层如下：

```
玩法逻辑（技能、突变因子、酒馆、肉鸽）
        │  创建 ViewModel / 调用命令
        ▼
ViewModel + Observable + Command
        │  数据绑定 / 事件回调
        ▼
UI 组件层（UIComponent 树）
        │  继承 GadgetClass / 注册到 GScreen
        ▼ 
UIRoot / DialogHost / GScreen 集成
        │  RA2 原生 Draw / Input / DSurface
        ▼
RA2 原生 GadgetClass / GScreenClass / DSurface / SHP-PCX
```

关键角色：

- `UIComponent`：所有控件的公共基类，继承 `GadgetClass`；提供 `Rect`、`Parent`、`Visible`、`Enabled`、`Tooltip`、事件回调、生命周期。
- `UIRoot`：面板级管理器；负责 `AddButton/RemoveButton`、模态输入策略、Tooltip、每帧 flush 绑定。
- `UIBuilder`：声明式工厂 API，例如 `UI::Panel(...)`、`UI::Button(...)`，负责创建控件树并注册。
- `Layout`：Row / Column / Grid / PageView / Anchor。
- `Mvvm`：`Observable<T>`、`ObservableVector<T>`、`Command`、`Binding`。

### 3.1 UIComponent 生命周期

- 创建：由 `UIBuilder` 创建控件并加入父容器，随后由 `UIRoot` 统一注册到 `GScreenClass`。
- 销毁：`UIRoot::Close()` 或容器销毁时，先 `RemoveButton`，再销毁子控件，最后删除自身。
- 控件销毁时自动解绑所有 `Binding`，避免悬空引用。

### 3.2 UIRoot 职责

- 维护当前打开的 Dialog / IconStrip。
- 每帧绘制前统一 flush 脏绑定。
- 统一绘制 Tooltip 和可选模态遮罩。
- 作为全局输入阻断判断的入口。

## 4. 布局系统

第一版只支持四种布局方式：

1. **显式定位**：`X/Y/Width/Height` 直接指定，相对父容器。
2. **Row / Column**：按顺序把子控件排成一行/一列，支持间距和对齐。
3. **Grid + PageView**：Grid 按列数排多行；PageView 承载多页 Grid，用翻页代替滚动条。
4. **简单锚点**：Dialog 居中、IconStrip 贴右、技能栏贴左下等固定锚点。

布局容器在创建或绑定列表变化时计算子控件位置。

## 5. 交互与事件

- **鼠标**：左键、右键、悬停进入/离开、滚轮翻页，全部沿用 `GadgetClass` 回调。
- **Keyboard / 快捷键**：Button 可绑定 `WWKey`，Dialog 可监听 `Esc`。
- **Command**：点击控件执行绑定命令，不把玩法逻辑写在控件内部。
- **Tooltip**：控件可挂 `Tooltip`，UIRoot 统一绘制标题、正文，并做位置避让。
- **输入阻断**：Dialog 的模态行为是**可选配置**，由具体 UI 决定。

## 6. 模态策略

模态阻断作为 Dialog 的可选项，提供以下级别：

- `None`：不阻断任何输入，Dialog 只作为普通面板。
- `BlockArea`：只阻断 Dialog 覆盖区域内的底层控件。
- `BlockTactical`：阻断战术地图 / 侧边栏滚动等“游戏世界操作”，不屏蔽其它 UI 控件。
- `BlockFullScreen`：屏蔽除当前 Dialog 外的所有输入。

实现上通过一个透明的 `Blocker` 控件或扩展现有 `DisplayClass_ProcessClickCoords_SkipOnNewButtons` / 滚轮处理来完成。第一版以 `BlockTactical` 为默认推荐值，具体场景按需选择。

## 7. MVVM-lite 设计

### 7.1 基础件

- `Observable<T>`：持有值；`Set()` 时把绑定控件标脏，下一帧由 UIRoot flush。
- `ObservableVector<T>`：持有列表；变化时通知容器重建当前页。
- `Binding`：把控件 `Text` / `Checked` / `Visible` / `Icon` / `Tooltip` 等属性绑定到 Observable。
- `Command`：包装 `Execute` 与 `CanExecute`；`CanExecute` 依赖 Observable 变化时自动更新按钮禁用态。

### 7.2 刷新策略

1. 玩法代码修改 Observable。
2. 相关控件被标为 dirty，不立刻重绘。
3. UIRoot 在每帧绘制前统一 flush。
4. 列表/网格数据变化时，用 `ItemBuilder` 重建当前页子控件。

### 7.3 列表项生成

使用轻量 `ItemBuilder` 代替完整 DataTemplate 示例：

```cpp
grid.BindItems(&vm->Mutations,
    onItem = [](auto& row, auto& item)
    {
        row.Add<UI::CheckBox>(item.Name)
            .BindChecked(item.IsSelected)
            .SetTooltip(item.Description);
    });
```

### 7.4 使用边界

- 静态、永远不变的文字不需要 Observable。
- 只在 Button 内部处理的临时状态不需要 ViewModel。
- 高频每帧动画数值直接每帧更新，不为它建绑定。

## 8. 首个案例映射：突变因子选择器 / 显示器

- **选择器**：居中 Dialog；标题栏 + 关闭按钮；中间是 `PageView + Grid`，每个选项为图标 + 名称 + 描述 + CheckBox；底部为上一页/下一页、页码、当前选择数、确认按钮。
- **显示器**：`IconStrip` 固定在屏幕右侧，显示已选突变图标，悬停显示 Tooltip。
- **ViewModel**：
  - `ObservableVector<MutationInfo> Mutations`
  - `Observable<int> PageIndex`
  - `Observable<SelectionSet> SelectedMutations`
  - `Command ToggleSelect`
  - `Command PageNext / PagePrev`
  - `Command Confirm / Close`

## 8.5 与 DynamicPatcher 的对接

突变因子逻辑已经存在于外部 DynamicPatcher C# 工程中，因此按 MVVM 分层如下：

```text
DynamicPatcher C#         = Model / 领域服务层
    MutatorCacheManager / MutatorRandomizer / Mutator
             ↑ 薄互操作层（查询/命令回调）
Phobos C++                = ViewModel + View
    src/UI（通用框架）
    src/Mutation（突变因子专属 ViewModel / View / 互操作适配）
```

原则：

- **Model 在 DynamicPatcher C#**：`MutatorCacheManager`、`MutatorRandomizer`、`Mutator` 继续作为数据源和玩法逻辑。
- **ViewModel / View 在 Phobos C++**：突变因子选择器、显示器、Mutable ViewModel 都使用 `src/UI` 框架。
- **Phobos 核心 UI 框架不感知突变因子**：`src/UI` 只提供通用工具；突变因子专属代码放 `src/Mutation`。
- **互操作沿用现有模式**：
  - C# 把查询/激活/取消等委托注册给 Phobos，Phobos C++ 通过函数指针调用。
  - C# 侧状态变化时 P/Invoke 通知 Phobos，`MutationViewModel` 刷新 Observable。
- **清理 C# 侧 UI 副作用**：`Mutator.Init/Uninit` 中直接操作 `SidebarClass` / `MessageListClass` 的逻辑逐步抽出，避免 C++ UI 和 C# 旧逻辑重复操作同一界面。

## 9. 工程结构

新增独立模块 `src/UI/`（通用框架）与 `src/Mutation/`（突变因子专属 UI），不与现有 `src/Ext/Sidebar/` 混在一起：

```
src/UI/
  UIComponent.h/cpp      // 控件基类 + 事件 + 生命周期
  UIRoot.h/cpp           // Dialog/面板管理器、flush、Tooltip、模态
  Builder.h              // 声明式创建 API
  Layout.h/cpp           // Row/Column/Grid/PageView/Anchor
  Mvvm.h/cpp             // Observable/ObservableVector/Command/Binding
  Controls/
    Panel.h/cpp
    Button.h/cpp
    CheckBox.h/cpp
    Label.h/cpp
    Tooltip.h/cpp
    PageView.h/cpp
    IconStrip.h/cpp
    ListGrid.h/cpp

src/Mutation/
  MutationInterop.h/cpp      // Phobos 与 DynamicPatcher 之间的薄适配层
  MutationViewModel.h/cpp    // MutationInfo / Observable / Command
  MutationSelectorDialog.h/cpp
  MutationDisplayerStrip.h/cpp
  OpenMutationUI.h/cpp       // Debug 命令，也归入突变因子模块
```

集成点：

- `UIRoot` 为全局单例，按需 `Open/Close`，不强制进入 Sidebar 初始化。
- 每帧在 MainLoop 附近 flush 脏绑定、绘制 Tooltip 和模态遮罩。
- 扩展现有 `DisplayClass_ProcessClickCoords_SkipOnNewButtons` 和滚轮处理，加入 `UIRoot` 判断。
- 第一版不改写现有 SelectedInfo/SWSidebar/UniqueButton/MessageColumn。

## 10. 错误处理

- 素材（SHP/PCX/Palette）缺失时不崩溃：降级为占位图形或隐藏对应控件，并写日志。
- 控件销毁时自动解绑 Binding，避免悬空引用。
- 同一时间允许存在多个 UI，但模态阻断级别由每个 Dialog 自行声明。
- 快捷键冲突：框架不覆盖原版命令；新 UI 在绑定快捷键时记录日志，必要时由开发者调整。
- 尊重 `ScenarioClass::UserInputLocked`：锁输入时不响应 UI 交互。
- UI 是纯客户端临时对象，不序列化、不进同步逻辑。

## 11. 测试策略

- 提供 Debug 命令直接打开突变因子选择器/显示器，便于手工测试。
- 手工测试清单：
  - 打开/关闭 Dialog、Esc 关闭、模态阻断级别生效。
  - 分页翻页、页码、上一页/下一页禁用态。
  - CheckBox 多选、已选数量、确认按钮可用性。
  - IconStrip 显示/隐藏、Tooltip 内容与位置。
  - 不同分辨率（4:3、16:9）和素材缺失场景。
  - 快捷键冲突、与旧 UI 同时打开时的行为。
- 逻辑层（布局计算、MVVM 绑定、Command）尽量保持纯计算，便于后续做单元测试。

## 12. 兼容性与风险

- 纯客户端 UI，不会造成 desync。
- 新 UI 只叠加不重写旧 UI，降低回归风险。
- 主要风险是控件树生命周期管理、模态输入阻断与旧滚动/点击逻辑的交互；通过在 UIRoot 收敛这些逻辑来控制。

