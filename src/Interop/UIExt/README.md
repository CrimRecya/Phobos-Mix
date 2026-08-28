# UIExt Interop API

`src/Interop/UIExt` 导出通用 UI 框架（`src/UI`）的 C 接口，供外部调用方（例如 DynamicPatcher C#、其他 DLL、脚本）创建和操作游戏内 UI 元素。

所有接口使用 `__stdcall` 调用约定，通过 `Phobos.dll` 导出，命名前缀为 `UIExt_`。

## 基本生命周期

1. 使用 `UIExt_Create*` 创建控件，得到一个不透明句柄（`void*` / `IntPtr`）。
2. 使用 `UIExt_AddChild` 把子控件挂到父控件上。
3. 使用 `UIExt_Open` 打开一个根控件（Panel / Dialog / IconStrip 等），框架接管所有权。
4. 使用 `UIExt_Close`（对已打开的根延迟到下一帧销毁）/ `UIExt_CloseAll` 关闭和销毁。

> 注意：句柄指向框架内部对象。不要自行释放，不要跨关闭操作后继续使用。

## 常量

### Anchor

| 常量 | 值 |
|---|---|
| `UIExtAnchor_None` | 0 |
| `UIExtAnchor_Center` | 1 |
| `UIExtAnchor_Left` | 2 |
| `UIExtAnchor_Right` | 3 |
| `UIExtAnchor_Top` | 4 |
| `UIExtAnchor_TopLeft` | 5 |
| `UIExtAnchor_TopRight` | 6 |
| `UIExtAnchor_Bottom` | 7 |
| `UIExtAnchor_BottomLeft` | 8 |
| `UIExtAnchor_BottomRight` | 9 |

### Modal

| 常量 | 值 | 说明 |
|---|---|---|
| `UIExtModal_None` | 0 | 不阻断输入 |
| `UIExtModal_BlockArea` | 1 | 阻断根控件覆盖区域 |
| `UIExtModal_BlockTactical` | 2 | 阻断战术地图/侧边栏等游戏世界操作 |
| `UIExtModal_BlockFullScreen` | 3 | 阻断除当前全屏 UI 外的所有输入 |

### ShpAlign

| 常量 | 值 | 说明 |
|---|---|---|
| `UIExtShpAlign_TopLeft` | 0 | 左上角 |
| `UIExtShpAlign_BottomLeft` | 1 | 左下角 |
| `UIExtShpAlign_TopRight` | 2 | 右上角 |
| `UIExtShpAlign_BottomRight` | 3 | 右下角 |
| `UIExtShpAlign_Center` | 4 | 居中 |

## 屏幕 / 根管理

| 函数 | 说明 |
|---|---|
| `HRESULT UIExt_Open(void* pRoot, int modal)` | 打开根控件，modal 取 `UIExtModal_*` |
| `HRESULT UIExt_Close(void* pControl)` | 关闭已打开的根（下一帧销毁），或销毁创建后尚未挂载（detached）的控件 |
| `HRESULT UIExt_CloseAll()` | 关闭所有屏幕并清空所有未挂载控件 |

## 控件创建

| 函数 | 创建类型 |
|---|---|
| `UIExt_CreatePanel(x, y, w, h, out ppControl)` | Panel |
| `UIExt_CreateDialog(x, y, w, h, title, out ppControl)` | Dialog |
| `UIExt_CreateButton(x, y, w, h, text, out ppControl)` | Button |
| `UIExt_CreateIconButton(x, y, w, h, out ppControl)` | IconButton |
| `UIExt_CreateCheckBox(x, y, w, h, text, out ppControl)` | CheckBox |
| `UIExt_CreateLabel(x, y, text, out ppControl)` | Label |
| `UIExt_CreateIconStrip(x, y, itemW, itemH, spacing, out ppControl)` | IconStrip |
| `UIExt_CreateListGrid(x, y, w, h, out ppControl)` | ListGrid |
| `UIExt_CreatePageView(x, y, w, h, out ppControl)` | PageView |

IconButton 创建后可用 `UIExt_Button_SetIconFromFile` 设置 PCX 图标。

## 控件树

| 函数 | 说明 |
|---|---|
| `UIExt_AddChild(parent, child)` | 把 `child` 挂到 `parent` 下；若 `parent` 已打开，会自动注册到游戏 Gadget 系统 |
| `UIExt_RemoveChild(parent, child)` | 从 `parent` 移除并销毁 `child` |

## 通用属性

| 函数 | 说明 |
|---|---|
| `UIExt_SetPos(control, x, y)` | 设置位置 |
| `UIExt_SetSize(control, w, h)` | 设置尺寸 |
| `UIExt_SetVisible(control, visible)` | 0/1 控制可见性 |
| `UIExt_SetEnabled(control, enabled)` | 0/1 控制可用性 |
| `UIExt_SetText(control, text)` | 设置 Button / Label / CheckBox 文本，或 Dialog 标题 |
| `UIExt_SetChecked(control, checked)` | 设置 CheckBox 勾选状态 |
| `UIExt_SetAnchor(control, anchor, offsetX, offsetY)` | 设置锚点 |
| `UIExt_SetTooltip(control, title, text)` | 设置标题/正文 Tooltip |
| `UIExt_SetTooltipDelegated(control, delegated)` | 上交 tooltip 布局权给父控件（可多级）；由最终管理控件优先在左右边外侧绘制，避免遮挡兄弟控件 |
| `UIExt_SetTooltipMaxWidth(control, width)` | tooltip 文本自动换行的最大宽度（像素，0 = 关闭），`\n` 仍为强制换行；各行自动平衡，避免末行吊脚 |
| `UIExt_SetBackColor(control, r, g, b, opacity)` | 设置 Panel/Dialog 背景色（0-255）和透明度 |
| `UIExt_SetBorder(control, enabled, color)` | 设置 Panel/Dialog 边框，color 为 `COLORREF` |

## Panel / Dialog SHP 背景

| 函数 | 说明 |
|---|---|
| `UIExt_Panel_SetShpBackground(panel, shpFile, paletteFile, frame, offsetX, offsetY, align)` | 给 Panel/Dialog 设置一个 SHP 背景装饰素材 |

参数说明：

- `shpFile`：游戏目录下的 SHP 文件名，例如 `"SIDEBAR.SHP"`。
- `paletteFile`：PAL 文件名；传 `nullptr` 或空串时使用默认 `ANIM_PAL`。
- `frame`：要绘制的帧号，越界时会自动归 0。
- `offsetX` / `offsetY`：在对齐基础上追加的偏移。
- `align`：`UIExtShpAlign_*`，控制素材贴在左上/左下/右上/右下/居中。

## Button

| 函数 | 说明 |
|---|---|
| `UIExt_Button_SetOnClick(button, callback, userData)` | 左键点击回调 |
| `UIExt_Button_SetOnRightClick(button, callback, userData)` | 右键点击回调 |
| `UIExt_Button_SetShortcut(button, key)` | 设置快捷键（虚拟键码） |
| `UIExt_Button_Click(button)` | 程序化触发点击 |
| `UIExt_Button_SetIconFromFile(button, filename)` | 从游戏目录加载 PCX 文件并设置为按钮图标 |
| `UIExt_Button_SetColor(button, normal, hover, disabled, text)` | 设置四态颜色，均为 `COLORREF`（0x00BBGGRR）；默认 0x303030 / 0x4A4A4A / 0x222222 / 白色 |
| `UIExt_Button_SetFillOpacity(button, opacity)` | 背景填充透明度 0-100：**0 不绘制背景**，1-99 半透明，≥100 不透明（默认） |
| `UIExt_Button_SetDrawHoverBorder(button, draw)` | 是否绘制悬停白色描边（0/1，默认绘制） |

回调类型：

```cpp
typedef void(__stdcall* UIExtActionCallback)(void* userData);
```

### PCX 素材注意事项

> ⚠️ 使用 PCX 素材时请注意（例如通过 `UIExt_Button_SetIconFromFile` 设置的按钮图标，以及框架内其它 PCX 绘制）：
>
> 1. **布局**：如果 PCX 的画布触及战术面板顶部，游戏会立即崩溃。这是游戏引擎本身的限制，请避免把 PCX 画到屏幕/战术区顶部边缘。
> 2. **尺寸**：如果 PCX 的长宽像素数为奇数，则会导致绘制错误，在左侧和下侧画布边缘产生一条异常的线。这也是游戏引擎本身的限制。

## CheckBox

| 函数 | 说明 |
|---|---|
| `UIExt_CheckBox_SetOnToggle(checkBox, callback, userData)` | 勾选状态变化回调 |

回调类型：

```cpp
typedef void(__stdcall* UIExtToggleCallback)(int checked, void* userData);
```

## Dialog

| 函数 | 说明 |
|---|---|
| `UIExt_Dialog_SetCloseAction(dialog, callback, userData)` | 绑定 Dialog 自带关闭按钮 |

## PageView

| 函数 | 说明 |
|---|---|
| `UIExt_PageView_SetGrid(pageView, columns, rows, itemW, itemH, gapX, gapY)` | 设置分页网格 |
| `UIExt_PageView_SetPage(pageView, page)` | 跳转到指定页 |
| `UIExt_PageView_NextPage(pageView)` | 下一页 |
| `UIExt_PageView_PrevPage(pageView)` | 上一页 |
| `UIExt_PageView_GetPageCount(pageView, out count)` | 获取页数 |
| `UIExt_PageView_GetPageIndex(pageView, out index)` | 获取当前页索引 |

## ListGrid

| 函数 | 说明 |
|---|---|
| `UIExt_ListGrid_SetColumns(grid, columns, itemW, itemH, gapX, gapY)` | 设置列数/格子尺寸 |
| `UIExt_ListGrid_Refresh(grid)` | 重新布局子控件 |

## IconStrip

| 函数 | 说明 |
|---|---|
| `UIExt_IconStrip_SetItemSize(strip, w, h)` | 设置图标尺寸 |
| `UIExt_IconStrip_SetSpacing(strip, spacing)` | 设置间距 |
| `UIExt_IconStrip_Refresh(strip)` | 重新布局并调整高度 |

## Layout 工具

```cpp
HRESULT UIExt_Layout_ArrangeRow(void* const* ppItems, int count, int x, int y, int spacing);
HRESULT UIExt_Layout_ArrangeColumn(void* const* ppItems, int count, int x, int y, int spacing);
HRESULT UIExt_Layout_ArrangeGrid(
    void* const* ppItems, int count, int x, int y, int columns,
    int itemWidth, int itemHeight, int gapX, int gapY);
```

`ppItems` 是控件句柄数组。

## 返回值

- `S_OK`：成功。
- `S_FALSE`：操作无效果（当前 API 较少使用）。
- `E_POINTER`：必要指针参数为空。
- `E_INVALIDARG`：参数无效，或句柄类型与函数不匹配。
- `E_OUTOFMEMORY`：创建控件失败。

## C# 使用示例

```csharp
using System;
using System.Runtime.InteropServices;

public static class UIExt
{
    public const int Modal_BlockTactical = 2;

    public const int Anchor_Center = 1;

    public const int ShpAlign_BottomLeft = 1;

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void ActionCallback(IntPtr userData);

    [UnmanagedFunctionPointer(CallingConvention.StdCall)]
    public delegate void ToggleCallback(int checkedState, IntPtr userData);

    [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall)]
    public static extern int UIExt_CreateDialog(
        int x, int y, int width, int height,
        [MarshalAs(UnmanagedType.LPWStr)] string title,
        out IntPtr dialog);

    [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall)]
    public static extern int UIExt_CreateLabel(
        int x, int y,
        [MarshalAs(UnmanagedType.LPWStr)] string text,
        out IntPtr label);

    [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall)]
    public static extern int UIExt_CreateButton(
        int x, int y, int width, int height,
        [MarshalAs(UnmanagedType.LPWStr)] string text,
        out IntPtr button);

    [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall)]
    public static extern int UIExt_AddChild(IntPtr parent, IntPtr child);

    [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall)]
    public static extern int UIExt_Open(IntPtr root, int modal);

    [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall)]
    public static extern int UIExt_SetPos(IntPtr control, int x, int y);

    [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall)]
    public static extern int UIExt_SetSize(IntPtr control, int width, int height);

    [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall)]
    public static extern int UIExt_Button_SetOnClick(
        IntPtr button, ActionCallback callback, IntPtr userData);

    [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall)]
    public static extern int UIExt_Button_SetIconFromFile(
        IntPtr button,
        [MarshalAs(UnmanagedType.LPStr)] string filename);

    [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall)]
    public static extern int UIExt_Panel_SetShpBackground(
        IntPtr panel,
        [MarshalAs(UnmanagedType.LPStr)] string shpFile,
        [MarshalAs(UnmanagedType.LPStr)] string paletteFile,
        int frame, int offsetX, int offsetY, int align);

    [DllImport("Phobos.dll", CallingConvention = CallingConvention.StdCall)]
    public static extern int UIExt_Close(IntPtr control);

    // 委托必须保存为字段，防止被 GC 回收后 C++ 侧回调悬空。
    private static ActionCallback CloseCallback;
    private static IntPtr OpenDialog;

    public static void OpenExampleDialog()
    {
        IntPtr dialog;
        int hr = UIExt_CreateDialog(200, 100, 400, 300, "Example", out dialog);
        if (hr != 0)
            return;

        UIExt_Panel_SetShpBackground(dialog, "sidedecor.shp", null, 0, 0, 0, ShpAlign_BottomLeft);

        IntPtr label;
        UIExt_CreateLabel(20, 20, "Hello from C#", out label);
        UIExt_AddChild(dialog, label);

        IntPtr button;
        UIExt_CreateButton(280, 240, 100, 30, "OK", out button);

        OpenDialog = dialog;
        CloseCallback = _ => UIExt_Close(OpenDialog);
        UIExt_Button_SetOnClick(button, CloseCallback, IntPtr.Zero);

        UIExt_AddChild(dialog, button);

        UIExt_Open(dialog, Modal_BlockTactical);
    }
}
```

> C# 委托必须保存为字段（本例为 `CloseCallback`），防止被 GC 回收后 C++ 侧回调悬空；并建议同时保存要关闭的对话框句柄。