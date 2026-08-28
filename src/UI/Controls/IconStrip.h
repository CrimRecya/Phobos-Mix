#pragma once

#include "../UIComponent.h"

#include <functional>

namespace UIExt
{
	// A vertical strip of icon buttons, useful for persistent indicator columns.
	class IconStrip : public UIComponent
	{
	public:
		using ItemBuilder = std::function<void(UIComponent&, size_t)>;

		IconStrip();
		IconStrip(int x, int y, int itemWidth, int itemHeight, int spacing = 4);

		IconStrip& SetItemSize(int width, int height);
		IconStrip& SetSpacing(int spacing);

		void Refresh();
		void RebuildItems(size_t count, const ItemBuilder& builder);

		UIComponentType GetType() const override { return UIComponentType::IconStrip; }

	protected:
		int ItemWidth { 60 };
		int ItemHeight { 48 };
		int Spacing { 4 };
	};
}