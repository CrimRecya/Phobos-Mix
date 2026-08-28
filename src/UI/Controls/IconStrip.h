#pragma once

#include "../UIComponent.h"

namespace UIExt
{
	// A vertical strip of icon buttons, useful for persistent indicator columns.
	class IconStrip : public UIComponent
	{
	public:
		IconStrip();
		IconStrip(int x, int y, int itemWidth, int itemHeight, int spacing = 4);

		IconStrip& SetItemSize(int width, int height);
		IconStrip& SetSpacing(int spacing);

		void Refresh();

	protected:
		int ItemWidth { 60 };
		int ItemHeight { 48 };
		int Spacing { 4 };
	};
}