#include "UIExtApi.h"

#include <UI/UIComponent.h>
#include <UI/UIRoot.h>
#include <UI/Builder.h>
#include <UI/Layout.h>
#include <UI/Controls/Dialog.h>
#include <UI/Controls/PageView.h>
#include <UI/Controls/ListGrid.h>
#include <UI/Controls/IconStrip.h>

#include <PCX.h>
#include <FileSystem.h>

#include <algorithm>
#include <memory>
#include <vector>

namespace
{
	using UIExt::UIComponent;

	std::vector<std::unique_ptr<UIComponent>> g_detached;

	auto FindDetached(UIComponent* control) -> decltype(g_detached.begin())
	{
		return std::find_if(g_detached.begin(), g_detached.end(),
			[control](const std::unique_ptr<UIComponent>& item)
			{
				return item.get() == control;
			});
	}

	bool IsDetached(UIComponent* control)
	{
		return control && FindDetached(control) != g_detached.end();
	}

	void* RegisterDetached(std::unique_ptr<UIComponent> control)
	{
		if (!control)
			return nullptr;

		auto* raw = control.get();
		g_detached.push_back(std::move(control));
		return raw;
	}

	HRESULT TakeDetached(UIComponent* control, std::unique_ptr<UIComponent>& out)
	{
		if (!control)
			return E_POINTER;

		auto it = FindDetached(control);
		if (it == g_detached.end())
			return E_INVALIDARG;

		out = std::move(*it);
		g_detached.erase(it);
		return S_OK;
	}

	UIExt::Button* AsButton(void* pControl)
	{
		auto* control = static_cast<UIComponent*>(pControl);
		if (!control)
			return nullptr;

		const auto type = control->GetType();
		return (type == UIExt::UIComponentType::Button || type == UIExt::UIComponentType::IconButton)
			? static_cast<UIExt::Button*>(control) : nullptr;
	}

	UIExt::CheckBox* AsCheckBox(void* pControl)
	{
		auto* control = static_cast<UIComponent*>(pControl);
		return control && control->GetType() == UIExt::UIComponentType::CheckBox
			? static_cast<UIExt::CheckBox*>(control) : nullptr;
	}

	UIExt::Label* AsLabel(void* pControl)
	{
		auto* control = static_cast<UIComponent*>(pControl);
		return control && control->GetType() == UIExt::UIComponentType::Label
			? static_cast<UIExt::Label*>(control) : nullptr;
	}

	UIExt::Panel* AsPanel(void* pControl)
	{
		auto* control = static_cast<UIComponent*>(pControl);
		if (!control)
			return nullptr;

		const auto type = control->GetType();
		return (type == UIExt::UIComponentType::Panel || type == UIExt::UIComponentType::Dialog)
			? static_cast<UIExt::Panel*>(control) : nullptr;
	}

	UIExt::Dialog* AsDialog(void* pControl)
	{
		auto* control = static_cast<UIComponent*>(pControl);
		return control && control->GetType() == UIExt::UIComponentType::Dialog
			? static_cast<UIExt::Dialog*>(control) : nullptr;
	}

	UIExt::PageView* AsPageView(void* pControl)
	{
		auto* control = static_cast<UIComponent*>(pControl);
		return control && control->GetType() == UIExt::UIComponentType::PageView
			? static_cast<UIExt::PageView*>(control) : nullptr;
	}

	UIExt::ListGrid* AsListGrid(void* pControl)
	{
		auto* control = static_cast<UIComponent*>(pControl);
		return control && control->GetType() == UIExt::UIComponentType::ListGrid
			? static_cast<UIExt::ListGrid*>(control) : nullptr;
	}

	UIExt::IconStrip* AsIconStrip(void* pControl)
	{
		auto* control = static_cast<UIComponent*>(pControl);
		return control && control->GetType() == UIExt::UIComponentType::IconStrip
			? static_cast<UIExt::IconStrip*>(control) : nullptr;
	}
}

// ----------------------------------------------------------------------------
// Screen / root management
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_Open, void* pRoot, int modal)
{
	if (!pRoot)
		return E_POINTER;

	if (modal < UIExtModal_None || modal > UIExtModal_BlockFullScreen)
		return E_INVALIDARG;

	auto* root = static_cast<UIComponent*>(pRoot);
	std::unique_ptr<UIComponent> rootPtr;
	auto hr = TakeDetached(root, rootPtr);
	if (FAILED(hr))
		return hr;

	UIExt::UIRoot::Instance().Open(std::move(rootPtr), static_cast<UIExt::ModalLevel>(modal));
	return S_OK;
}

DEFINE_EXPORT(HRESULT, UIExt_Close, void* pControl)
{
	if (!pControl)
		return E_POINTER;

	auto* control = static_cast<UIComponent*>(pControl);

	if (IsDetached(control))
	{
		auto it = FindDetached(control);
		g_detached.erase(it);
		return S_OK;
	}

	UIExt::UIRoot::Instance().Close(control);
	return S_OK;
}

DEFINE_EXPORT(HRESULT, UIExt_CloseAll)
{
	UIExt::UIRoot::Instance().CloseAll();
	g_detached.clear();
	return S_OK;
}

// ----------------------------------------------------------------------------
// Control creation
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_CreatePanel,
	int x, int y, int width, int height, void** ppControl)
{
	if (!ppControl)
		return E_POINTER;

	*ppControl = RegisterDetached(UIExt::Builder::MakePanel(x, y, width, height));
	return *ppControl ? S_OK : E_OUTOFMEMORY;
}

DEFINE_EXPORT(HRESULT, UIExt_CreateDialog,
	int x, int y, int width, int height, const wchar_t* title, void** ppControl)
{
	if (!ppControl)
		return E_POINTER;

	std::unique_ptr<UIExt::Dialog> dialog;
	if (title)
		dialog = UIExt::Builder::MakeDialog(x, y, width, height, title);
	else
		dialog = UIExt::Builder::MakeDialog(x, y, width, height, L"");

	*ppControl = RegisterDetached(std::move(dialog));
	return *ppControl ? S_OK : E_OUTOFMEMORY;
}

DEFINE_EXPORT(HRESULT, UIExt_CreateButton,
	int x, int y, int width, int height, const wchar_t* text, void** ppControl)
{
	if (!ppControl)
		return E_POINTER;

	auto button = text
		? UIExt::Builder::MakeButton(x, y, width, height, text)
		: UIExt::Builder::MakeButton(x, y, width, height, L"");

	*ppControl = RegisterDetached(std::move(button));
	return *ppControl ? S_OK : E_OUTOFMEMORY;
}

DEFINE_EXPORT(HRESULT, UIExt_CreateIconButton,
	int x, int y, int width, int height, void** ppControl)
{
	if (!ppControl)
		return E_POINTER;

	auto button = UIExt::Builder::MakeIconButton(x, y, width, height);
	*ppControl = RegisterDetached(std::move(button));
	return *ppControl ? S_OK : E_OUTOFMEMORY;
}

DEFINE_EXPORT(HRESULT, UIExt_CreateCheckBox,
	int x, int y, int width, int height, const wchar_t* text, void** ppControl)
{
	if (!ppControl)
		return E_POINTER;

	auto checkBox = text
		? UIExt::Builder::MakeCheckBox(x, y, width, height, text)
		: UIExt::Builder::MakeCheckBox(x, y, width, height, L"");

	*ppControl = RegisterDetached(std::move(checkBox));
	return *ppControl ? S_OK : E_OUTOFMEMORY;
}

DEFINE_EXPORT(HRESULT, UIExt_CreateLabel,
	int x, int y, const wchar_t* text, void** ppControl)
{
	if (!ppControl)
		return E_POINTER;

	auto label = text
		? UIExt::Builder::MakeLabel(x, y, text)
		: UIExt::Builder::MakeLabel(x, y, L"");

	*ppControl = RegisterDetached(std::move(label));
	return *ppControl ? S_OK : E_OUTOFMEMORY;
}

DEFINE_EXPORT(HRESULT, UIExt_CreateIconStrip,
	int x, int y, int itemWidth, int itemHeight, int spacing, void** ppControl)
{
	if (!ppControl)
		return E_POINTER;

	*ppControl = RegisterDetached(UIExt::Builder::MakeIconStrip(x, y, itemWidth, itemHeight, spacing));
	return *ppControl ? S_OK : E_OUTOFMEMORY;
}

DEFINE_EXPORT(HRESULT, UIExt_CreateListGrid,
	int x, int y, int width, int height, void** ppControl)
{
	if (!ppControl)
		return E_POINTER;

	*ppControl = RegisterDetached(UIExt::Builder::MakeListGrid(x, y, width, height));
	return *ppControl ? S_OK : E_OUTOFMEMORY;
}

DEFINE_EXPORT(HRESULT, UIExt_CreatePageView,
	int x, int y, int width, int height, void** ppControl)
{
	if (!ppControl)
		return E_POINTER;

	*ppControl = RegisterDetached(UIExt::Builder::MakePageView(x, y, width, height));
	return *ppControl ? S_OK : E_OUTOFMEMORY;
}

// ----------------------------------------------------------------------------
// Tree management
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_AddChild, void* pParent, void* pChild)
{
	if (!pParent || !pChild)
		return E_POINTER;

	auto* parent = static_cast<UIComponent*>(pParent);
	auto* child = static_cast<UIComponent*>(pChild);

	std::unique_ptr<UIComponent> childPtr;
	auto hr = TakeDetached(child, childPtr);
	if (FAILED(hr))
		return hr;

	if (UIExt::UIRoot::Instance().IsComponentOpen(parent))
	{
		UIExt::UIRoot::Instance().AddRootChild(parent, std::move(childPtr));
	}
	else
	{
		parent->AddChild(std::move(childPtr));
	}

	return S_OK;
}

DEFINE_EXPORT(HRESULT, UIExt_RemoveChild, void* pParent, void* pChild)
{
	if (!pParent || !pChild)
		return E_POINTER;

	auto* parent = static_cast<UIComponent*>(pParent);
	auto* child = static_cast<UIComponent*>(pChild);

	if (UIExt::UIRoot::Instance().IsComponentOpen(parent))
	{
		UIExt::UIRoot::Instance().RemoveRootChild(parent, child);
		return S_OK;
	}

	auto& children = parent->GetChildren();
	children.erase(std::remove_if(children.begin(), children.end(),
		[child](const std::unique_ptr<UIComponent>& item)
		{
			return item.get() == child;
		}), children.end());

	return S_OK;
}

// ----------------------------------------------------------------------------
// Common properties
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_SetPos, void* pControl, int x, int y)
{
	if (!pControl)
		return E_POINTER;

	static_cast<UIComponent*>(pControl)->SetPos(x, y);
	return S_OK;
}

DEFINE_EXPORT(HRESULT, UIExt_SetSize, void* pControl, int width, int height)
{
	if (!pControl)
		return E_POINTER;

	static_cast<UIComponent*>(pControl)->SetSize(width, height);
	return S_OK;
}

DEFINE_EXPORT(HRESULT, UIExt_SetVisible, void* pControl, int visible)
{
	if (!pControl)
		return E_POINTER;

	static_cast<UIComponent*>(pControl)->SetVisible(visible != 0);
	return S_OK;
}

DEFINE_EXPORT(HRESULT, UIExt_SetEnabled, void* pControl, int enabled)
{
	if (!pControl)
		return E_POINTER;

	static_cast<UIComponent*>(pControl)->SetEnabled(enabled != 0);
	return S_OK;
}

DEFINE_EXPORT(HRESULT, UIExt_SetText, void* pControl, const wchar_t* text)
{
	if (!pControl)
		return E_POINTER;

	const std::wstring value = text ? text : L"";

	if (auto* button = AsButton(pControl))
	{
		button->SetText(value);
		return S_OK;
	}

	if (auto* checkBox = AsCheckBox(pControl))
	{
		checkBox->SetText(value);
		return S_OK;
	}

	if (auto* label = AsLabel(pControl))
	{
		label->SetText(value);
		return S_OK;
	}

	if (auto* dialog = AsDialog(pControl))
	{
		dialog->SetTitle(value);
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_SetChecked, void* pControl, int checked)
{
	if (!pControl)
		return E_POINTER;

	if (auto* checkBox = AsCheckBox(pControl))
	{
		checkBox->SetChecked(checked != 0);
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_SetAnchor,
	void* pControl, int anchor, int offsetX, int offsetY)
{
	if (!pControl)
		return E_POINTER;

	auto* control = static_cast<UIComponent*>(pControl);

	if (anchor < UIExtAnchor_None || anchor > UIExtAnchor_BottomRight)
		return E_INVALIDARG;

	control->SetAnchor(static_cast<UIExt::Anchor>(anchor), offsetX, offsetY);
	return S_OK;
}

DEFINE_EXPORT(HRESULT, UIExt_SetTooltip,
	void* pControl, const wchar_t* title, const wchar_t* text)
{
	if (!pControl)
		return E_POINTER;

	static_cast<UIComponent*>(pControl)->SetTooltip(title ? title : L"", text ? text : L"");
	return S_OK;
}

DEFINE_EXPORT(HRESULT, UIExt_SetBackColor,
	void* pControl, int r, int g, int b, int opacity)
{
	if (!pControl)
		return E_POINTER;

	if (auto* panel = AsPanel(pControl))
	{
		ColorStruct color
		{
			static_cast<BYTE>(r),
			static_cast<BYTE>(g),
			static_cast<BYTE>(b)
		};
		panel->SetBackColor(color, opacity);
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_SetBorder, void* pControl, int enabled, int color)
{
	if (!pControl)
		return E_POINTER;

	if (auto* panel = AsPanel(pControl))
	{
		panel->SetBorder(enabled != 0, static_cast<COLORREF>(color));
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_Panel_SetShpBackground,
	void* pPanel, const char* shpFile, const char* paletteFile,
	int frame, int offsetX, int offsetY, int align)
{
	if (!pPanel || !shpFile)
		return E_POINTER;

	if (align < UIExtShpAlign_TopLeft || align > UIExtShpAlign_Center)
		return E_INVALIDARG;

	auto* panel = AsPanel(pPanel);
	if (!panel)
		return E_INVALIDARG;

	auto* shp = FileSystem::LoadSHPFile(shpFile);
	if (!shp)
		return S_FALSE;

	if (frame < 0 || (shp->Frames > 0 && frame >= shp->Frames))
		frame = 0;

	ConvertClass* palette = nullptr;

	if (paletteFile && *paletteFile)
	{
		// FileSystem::LoadPALFile does not null-check its internal load,
		// so verify the palette file is readable before calling it.
		if (FileSystem::LoadFile(paletteFile, false) != nullptr)
			palette = FileSystem::LoadPALFile(paletteFile, DSurface::Composite);
	}

	panel->SetShpBackground(shp, palette, frame, offsetX, offsetY,
		static_cast<UIExt::Panel::ShpAlign>(align));

	return S_OK;
}

// ----------------------------------------------------------------------------
// Button
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_Button_SetOnClick,
	void* pButton, UIExtActionCallback callback, void* userData)
{
	if (!pButton)
		return E_POINTER;

	if (auto* button = AsButton(pButton))
	{
		if (callback)
		{
			button->SetOnClick([callback, userData]
			{
				callback(userData);
			});
		}
		else
		{
			button->SetOnClick(nullptr);
		}

		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_Button_SetOnRightClick,
	void* pButton, UIExtActionCallback callback, void* userData)
{
	if (!pButton)
		return E_POINTER;

	if (auto* button = AsButton(pButton))
	{
		if (callback)
		{
			button->SetOnRightClick([callback, userData]
			{
				callback(userData);
			});
		}
		else
		{
			button->SetOnRightClick(nullptr);
		}

		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_Button_SetShortcut, void* pButton, int key)
{
	if (!pButton)
		return E_POINTER;

	if (auto* button = AsButton(pButton))
	{
		button->SetShortcut(static_cast<DWORD>(key));
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_Button_Click, void* pButton)
{
	if (!pButton)
		return E_POINTER;

	if (auto* button = AsButton(pButton))
	{
		button->Click();
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_Button_SetIconFromFile, void* pButton, const char* filename)
{
	if (!pButton || !filename)
		return E_POINTER;

	if (auto* button = AsButton(pButton))
	{
		// Let SetIconFile do the actual load/cache lookup. Calling
		// PCX::Instance.LoadFile() first here caused crashes with some PCX
		// files when the button was later drawn.
		button->SetIconFile(filename);
		return PCX::Instance.GetSurface(filename) ? S_OK : S_FALSE;
	}

	return E_INVALIDARG;
}

// ----------------------------------------------------------------------------
// CheckBox
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_CheckBox_SetOnToggle,
	void* pCheckBox, UIExtToggleCallback callback, void* userData)
{
	if (!pCheckBox)
		return E_POINTER;

	if (auto* checkBox = AsCheckBox(pCheckBox))
	{
		if (callback)
		{
			checkBox->SetOnToggle([callback, userData](bool checked)
			{
				callback(checked ? 1 : 0, userData);
			});
		}
		else
		{
			checkBox->SetOnToggle(nullptr);
		}

		return S_OK;
	}

	return E_INVALIDARG;
}

// ----------------------------------------------------------------------------
// Dialog
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_Dialog_SetCloseAction,
	void* pDialog, UIExtActionCallback callback, void* userData)
{
	if (!pDialog)
		return E_POINTER;

	if (auto* dialog = AsDialog(pDialog))
	{
		if (callback)
		{
			dialog->SetCloseAction([callback, userData]
			{
				callback(userData);
			});
		}
		else
		{
			dialog->SetCloseAction(nullptr);
		}

		return S_OK;
	}

	return E_INVALIDARG;
}

// ----------------------------------------------------------------------------
// PageView
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_PageView_SetGrid,
	void* pPageView, int columns, int rows, int itemWidth, int itemHeight, int gapX, int gapY)
{
	if (!pPageView)
		return E_POINTER;

	if (auto* pageView = AsPageView(pPageView))
	{
		pageView->SetGrid(columns, rows, itemWidth, itemHeight, gapX, gapY);
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_PageView_SetPage, void* pPageView, int page)
{
	if (!pPageView)
		return E_POINTER;

	if (auto* pageView = AsPageView(pPageView))
	{
		pageView->SetPage(page);
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_PageView_NextPage, void* pPageView)
{
	if (!pPageView)
		return E_POINTER;

	if (auto* pageView = AsPageView(pPageView))
	{
		pageView->NextPage();
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_PageView_PrevPage, void* pPageView)
{
	if (!pPageView)
		return E_POINTER;

	if (auto* pageView = AsPageView(pPageView))
	{
		pageView->PrevPage();
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_PageView_GetPageCount, void* pPageView, int* pCount)
{
	if (!pPageView || !pCount)
		return E_POINTER;

	if (auto* pageView = AsPageView(pPageView))
	{
		*pCount = pageView->GetPageCount();
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_PageView_GetPageIndex, void* pPageView, int* pIndex)
{
	if (!pPageView || !pIndex)
		return E_POINTER;

	if (auto* pageView = AsPageView(pPageView))
	{
		*pIndex = pageView->GetPageIndex();
		return S_OK;
	}

	return E_INVALIDARG;
}

// ----------------------------------------------------------------------------
// ListGrid
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_ListGrid_SetColumns,
	void* pListGrid, int columns, int itemWidth, int itemHeight, int gapX, int gapY)
{
	if (!pListGrid)
		return E_POINTER;

	if (auto* listGrid = AsListGrid(pListGrid))
	{
		listGrid->SetColumns(columns, itemWidth, itemHeight, gapX, gapY);
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_ListGrid_Refresh, void* pListGrid)
{
	if (!pListGrid)
		return E_POINTER;

	if (auto* listGrid = AsListGrid(pListGrid))
	{
		listGrid->Refresh();
		return S_OK;
	}

	return E_INVALIDARG;
}

// ----------------------------------------------------------------------------
// IconStrip
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_IconStrip_SetItemSize,
	void* pIconStrip, int width, int height)
{
	if (!pIconStrip)
		return E_POINTER;

	if (auto* iconStrip = AsIconStrip(pIconStrip))
	{
		iconStrip->SetItemSize(width, height);
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_IconStrip_SetSpacing, void* pIconStrip, int spacing)
{
	if (!pIconStrip)
		return E_POINTER;

	if (auto* iconStrip = AsIconStrip(pIconStrip))
	{
		iconStrip->SetSpacing(spacing);
		return S_OK;
	}

	return E_INVALIDARG;
}

DEFINE_EXPORT(HRESULT, UIExt_IconStrip_Refresh, void* pIconStrip)
{
	if (!pIconStrip)
		return E_POINTER;

	if (auto* iconStrip = AsIconStrip(pIconStrip))
	{
		iconStrip->Refresh();
		return S_OK;
	}

	return E_INVALIDARG;
}

// ----------------------------------------------------------------------------
// Layout helpers
// ----------------------------------------------------------------------------

DEFINE_EXPORT(HRESULT, UIExt_Layout_ArrangeRow,
	void* const* ppItems, int count, int x, int y, int spacing)
{
	if (!ppItems && count > 0)
		return E_POINTER;

	std::vector<UIComponent*> items;
	items.reserve(count > 0 ? count : 0);

	for (int i = 0; i < count; ++i)
	{
		if (ppItems[i])
			items.push_back(static_cast<UIComponent*>(ppItems[i]));
	}

	UIExt::Layout::ArrangeRow(items, x, y, spacing);
	return S_OK;
}

DEFINE_EXPORT(HRESULT, UIExt_Layout_ArrangeColumn,
	void* const* ppItems, int count, int x, int y, int spacing)
{
	if (!ppItems && count > 0)
		return E_POINTER;

	std::vector<UIComponent*> items;
	items.reserve(count > 0 ? count : 0);

	for (int i = 0; i < count; ++i)
	{
		if (ppItems[i])
			items.push_back(static_cast<UIComponent*>(ppItems[i]));
	}

	UIExt::Layout::ArrangeColumn(items, x, y, spacing);
	return S_OK;
}

DEFINE_EXPORT(HRESULT, UIExt_Layout_ArrangeGrid,
	void* const* ppItems, int count, int x, int y, int columns,
	int itemWidth, int itemHeight, int gapX, int gapY)
{
	if (!ppItems && count > 0)
		return E_POINTER;

	std::vector<UIComponent*> items;
	items.reserve(count > 0 ? count : 0);

	for (int i = 0; i < count; ++i)
	{
		if (ppItems[i])
			items.push_back(static_cast<UIComponent*>(ppItems[i]));
	}

	UIExt::Layout::ArrangeGrid(items, x, y, columns, itemWidth, itemHeight, gapX, gapY);
	return S_OK;
}