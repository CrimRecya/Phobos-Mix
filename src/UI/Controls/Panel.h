#pragma once

#include "../UIComponent.h"

#include <Drawing.h>
#include <Surface.h>

namespace UIExt
{
	// Simple background panel. Subclasses / game-specific code can draw SHP/PCX art later.
	class Panel : public UIComponent
	{
	public:
		Panel();
		Panel(int x, int y, int width, int height);

		Panel& SetBackColor(ColorStruct color, int opacity = 40);
		Panel& SetBorder(bool enabled, COLORREF color = COLOR_WHITE);

		bool Draw(bool forced) override;

	protected:
		ColorStruct FillColor { 0, 0, 0 };
		int FillOpacity { 0 };
		bool DrawBorder { false };
		COLORREF BorderColor { COLOR_WHITE };
	};
}