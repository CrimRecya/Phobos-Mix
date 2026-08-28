#pragma once

#include "Controls/Button.h"
#include "Controls/CheckBox.h"
#include "Controls/Dialog.h"
#include "Controls/IconButton.h"
#include "Controls/IconStrip.h"
#include "Controls/Label.h"
#include "Controls/ListGrid.h"
#include "Controls/PageView.h"
#include "Controls/Panel.h"

#include <memory>
#include <string>

namespace UIExt
{
	// Declarative factory helpers. They only create controls; UIRoot::Open() registers them.
	class Builder
	{
	public:
		static std::unique_ptr<Panel> MakePanel(int x, int y, int width, int height)
		{
			return std::make_unique<UIExt::Panel>(x, y, width, height);
		}

		static std::unique_ptr<Dialog> MakeDialog(int x, int y, int width, int height, std::wstring title)
		{
			return std::make_unique<UIExt::Dialog>(x, y, width, height, std::move(title));
		}

		static std::unique_ptr<Button> MakeButton(int x, int y, int width, int height, std::wstring text)
		{
			return std::make_unique<UIExt::Button>(x, y, width, height, std::move(text));
		}

		static std::unique_ptr<IconButton> MakeIconButton(int x, int y, int width, int height)
		{
			return std::make_unique<UIExt::IconButton>(x, y, width, height);
		}

		static std::unique_ptr<CheckBox> MakeCheckBox(int x, int y, int width, int height, std::wstring text)
		{
			return std::make_unique<UIExt::CheckBox>(x, y, width, height, std::move(text));
		}

		static std::unique_ptr<Label> MakeLabel(int x, int y, std::wstring text)
		{
			return std::make_unique<UIExt::Label>(x, y, std::move(text));
		}

		static std::unique_ptr<PageView> MakePageView(int x, int y, int width, int height)
		{
			return std::make_unique<UIExt::PageView>(x, y, width, height);
		}

		static std::unique_ptr<IconStrip> MakeIconStrip(int x, int y, int itemWidth, int itemHeight, int spacing = 4)
		{
			return std::make_unique<UIExt::IconStrip>(x, y, itemWidth, itemHeight, spacing);
		}

		static std::unique_ptr<ListGrid> MakeListGrid(int x, int y, int width, int height)
		{
			return std::make_unique<UIExt::ListGrid>(x, y, width, height);
		}
	};
}