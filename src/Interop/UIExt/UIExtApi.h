#pragma once

#include "Utilities/Macro.h"

#include <Windows.h>

// ============================================================================
// UIExt Interop API
//
// Exposes the generic UI framework (src/UI) to external callers (C# via
// P/Invoke, other DLLs, scripts, etc.).
//
// All handles are opaque void* pointers to UIExt::UIComponent objects. The
// caller must not dereference them directly; use the API functions below.
//
// Lifecycle:
//   1. Create controls with UIExt_Create* -> obtain handle.
//   2. Build the tree with UIExt_AddChild.
//   3. Open a root control with UIExt_Open.
//   4. Close with UIExt_Close / UIExt_CloseAll.
//
// Callbacks are C function pointers so they can be marshalled from C#.
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// Callback invoked when a button is clicked / right-clicked / dialog closed.
typedef void(__stdcall* UIExtActionCallback)(void* userData);
// Callback invoked when a checkbox toggles; receives the new checked state.
typedef void(__stdcall* UIExtToggleCallback)(int checked, void* userData);

// Anchor values (UIExt::Anchor).
#define UIExtAnchor_None 0
#define UIExtAnchor_Center 1
#define UIExtAnchor_Left 2
#define UIExtAnchor_Right 3
#define UIExtAnchor_Top 4
#define UIExtAnchor_TopLeft 5
#define UIExtAnchor_TopRight 6
#define UIExtAnchor_Bottom 7
#define UIExtAnchor_BottomLeft 8
#define UIExtAnchor_BottomRight 9

// Modal levels (UIExt::ModalLevel).
#define UIExtModal_None 0
#define UIExtModal_BlockArea 1
#define UIExtModal_BlockTactical 2
#define UIExtModal_BlockFullScreen 3

// SHP background alignment (UIExt::Panel::ShpAlign).
#define UIExtShpAlign_TopLeft 0
#define UIExtShpAlign_BottomLeft 1
#define UIExtShpAlign_TopRight 2
#define UIExtShpAlign_BottomRight 3
#define UIExtShpAlign_Center 4

// ----------------------------------------------------------------------------
// Screen / root management
// ----------------------------------------------------------------------------

/// Opens a previously created root control. The framework takes ownership.
/// modal is one of UIExtModal_*.
DEFINE_EXPORT(HRESULT, UIExt_Open, void* pRoot, int modal);

/// Closes an open root control (deferred to the next frame) or destroys a
/// detached control that has not been opened/added yet.
DEFINE_EXPORT(HRESULT, UIExt_Close, void* pControl);

/// Closes and destroys all open screens and all detached controls.
DEFINE_EXPORT(HRESULT, UIExt_CloseAll);

// ----------------------------------------------------------------------------
// Control creation
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_CreatePanel,
	int x, int y, int width, int height, void** ppControl);

DEFINE_EXPORT(HRESULT, UIExt_CreateDialog,
	int x, int y, int width, int height, const wchar_t* title, void** ppControl);

DEFINE_EXPORT(HRESULT, UIExt_CreateButton,
	int x, int y, int width, int height, const wchar_t* text, void** ppControl);

DEFINE_EXPORT(HRESULT, UIExt_CreateIconButton,
	int x, int y, int width, int height, void** ppControl);

DEFINE_EXPORT(HRESULT, UIExt_CreateCheckBox,
	int x, int y, int width, int height, const wchar_t* text, void** ppControl);

DEFINE_EXPORT(HRESULT, UIExt_CreateLabel,
	int x, int y, const wchar_t* text, void** ppControl);

DEFINE_EXPORT(HRESULT, UIExt_CreateIconStrip,
	int x, int y, int itemWidth, int itemHeight, int spacing, void** ppControl);

DEFINE_EXPORT(HRESULT, UIExt_CreateListGrid,
	int x, int y, int width, int height, void** ppControl);

DEFINE_EXPORT(HRESULT, UIExt_CreatePageView,
	int x, int y, int width, int height, void** ppControl);

// ----------------------------------------------------------------------------
// Tree management
// ----------------------------------------------------------------------------

/// Adds pChild to pParent. If the parent is already part of an open screen the
/// child is registered with the game's gadget system automatically.
DEFINE_EXPORT(HRESULT, UIExt_AddChild, void* pParent, void* pChild);

/// Removes pChild from pParent and destroys it.
DEFINE_EXPORT(HRESULT, UIExt_RemoveChild, void* pParent, void* pChild);

// ----------------------------------------------------------------------------
// Common properties
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_SetPos, void* pControl, int x, int y);
DEFINE_EXPORT(HRESULT, UIExt_SetSize, void* pControl, int width, int height);
DEFINE_EXPORT(HRESULT, UIExt_SetVisible, void* pControl, int visible);
DEFINE_EXPORT(HRESULT, UIExt_SetEnabled, void* pControl, int enabled);
DEFINE_EXPORT(HRESULT, UIExt_SetText, void* pControl, const wchar_t* text);
DEFINE_EXPORT(HRESULT, UIExt_SetChecked, void* pControl, int checked);
DEFINE_EXPORT(HRESULT, UIExt_SetAnchor,
	void* pControl, int anchor, int offsetX, int offsetY);
DEFINE_EXPORT(HRESULT, UIExt_SetTooltip,
	void* pControl, const wchar_t* title, const wchar_t* text);

// Panel / Dialog background.
DEFINE_EXPORT(HRESULT, UIExt_SetBackColor,
	void* pControl, int r, int g, int b, int opacity);
DEFINE_EXPORT(HRESULT, UIExt_SetBorder, void* pControl, int enabled, int color);

/// Loads a SHP frame and draws it as a background decoration on a Panel/Dialog.
/// paletteFile can be null/empty to use the default ANIM_PAL.
/// align is one of UIExtShpAlign_*.
DEFINE_EXPORT(HRESULT, UIExt_Panel_SetShpBackground,
	void* pPanel, const char* shpFile, const char* paletteFile,
	int frame, int offsetX, int offsetY, int align);

// ----------------------------------------------------------------------------
// Button
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_Button_SetOnClick,
	void* pButton, UIExtActionCallback callback, void* userData);
DEFINE_EXPORT(HRESULT, UIExt_Button_SetOnRightClick,
	void* pButton, UIExtActionCallback callback, void* userData);
DEFINE_EXPORT(HRESULT, UIExt_Button_SetShortcut, void* pButton, int key);
DEFINE_EXPORT(HRESULT, UIExt_Button_Click, void* pButton);
DEFINE_EXPORT(HRESULT, UIExt_Button_SetIconFromFile, void* pButton, const char* filename);

/// Sets the button's four colors as COLORREF values (0x00BBGGRR).
/// Defaults: normal 0x303030, hover 0x4A4A4A, disabled 0x222222, text COLOR_WHITE.
DEFINE_EXPORT(HRESULT, UIExt_Button_SetColor,
	void* pButton, int normalColor, int hoverColor, int disabledColor, int textColor);

/// Sets the background fill opacity (0-100). 0 disables the background fill
/// entirely; 1-99 draws a translucent fill; >= 100 draws an opaque fill.
DEFINE_EXPORT(HRESULT, UIExt_Button_SetFillOpacity, void* pButton, int opacity);

/// Controls whether the white outline is drawn while the button is hovered.
DEFINE_EXPORT(HRESULT, UIExt_Button_SetDrawHoverBorder, void* pButton, int draw);

// ----------------------------------------------------------------------------
// CheckBox
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_CheckBox_SetOnToggle,
	void* pCheckBox, UIExtToggleCallback callback, void* userData);

// ----------------------------------------------------------------------------
// Dialog
// ----------------------------------------------------------------------------

/// Wires the dialog's built-in close button to a callback.
DEFINE_EXPORT(HRESULT, UIExt_Dialog_SetCloseAction,
	void* pDialog, UIExtActionCallback callback, void* userData);

// ----------------------------------------------------------------------------
// PageView
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_PageView_SetGrid,
	void* pPageView, int columns, int rows, int itemWidth, int itemHeight, int gapX, int gapY);
DEFINE_EXPORT(HRESULT, UIExt_PageView_SetPage, void* pPageView, int page);
DEFINE_EXPORT(HRESULT, UIExt_PageView_NextPage, void* pPageView);
DEFINE_EXPORT(HRESULT, UIExt_PageView_PrevPage, void* pPageView);
DEFINE_EXPORT(HRESULT, UIExt_PageView_GetPageCount, void* pPageView, int* pCount);
DEFINE_EXPORT(HRESULT, UIExt_PageView_GetPageIndex, void* pPageView, int* pIndex);

// ----------------------------------------------------------------------------
// ListGrid
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_ListGrid_SetColumns,
	void* pListGrid, int columns, int itemWidth, int itemHeight, int gapX, int gapY);
DEFINE_EXPORT(HRESULT, UIExt_ListGrid_Refresh, void* pListGrid);

// ----------------------------------------------------------------------------
// IconStrip
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_IconStrip_SetItemSize,
	void* pIconStrip, int width, int height);
DEFINE_EXPORT(HRESULT, UIExt_IconStrip_SetSpacing, void* pIconStrip, int spacing);
DEFINE_EXPORT(HRESULT, UIExt_IconStrip_Refresh, void* pIconStrip);

// ----------------------------------------------------------------------------
// Layout helpers
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_Layout_ArrangeRow,
	void* const* ppItems, int count, int x, int y, int spacing);
DEFINE_EXPORT(HRESULT, UIExt_Layout_ArrangeColumn,
	void* const* ppItems, int count, int x, int y, int spacing);
DEFINE_EXPORT(HRESULT, UIExt_Layout_ArrangeGrid,
	void* const* ppItems, int count, int x, int y, int columns,
	int itemWidth, int itemHeight, int gapX, int gapY);

#ifdef __cplusplus
}
#endif