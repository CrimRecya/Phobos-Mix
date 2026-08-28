#include "Tooltip.h"

#include <Drawing.h>
#include <Surface.h>
#include <YRMath.h>

namespace UIExt
{
	void TooltipRenderer::Draw(const UIComponent& component)
	{
		if (component.TooltipTitle.empty() && component.TooltipText.empty())
			return;

		Point2D location { component.X + component.Width + 8, component.Y - 3 };
		RectangleStruct titleRect = Drawing::GetTextDimensions(component.TooltipTitle.c_str(), location, 0, 3, 2);

		int width = titleRect.Width;
		int height = titleRect.Height;

		if (!component.TooltipText.empty())
		{
			RectangleStruct textRect = Drawing::GetTextDimensions(component.TooltipText.c_str(), location, 0, 3, 2);
			width = Math::max(width, textRect.Width);
			height += textRect.Height + 2;
		}

		width += 8;
		height += 4;
		location += Point2D { 4, 1 };

		RectangleStruct drawRect { location.X, location.Y, width, height };
		ColorStruct bgColor { 0, 0, 0 };
		DSurface::Composite->FillRectTrans(&drawRect, &bgColor, 40);

		if (component.TooltipText.empty())
			drawRect.Height = titleRect.Height + 4;

		DSurface::Composite->DrawRect(&drawRect, COLOR_WHITE);

		Point2D textPos = location + Point2D { 0, 0 };
		DSurface::Composite->DrawText(component.TooltipTitle.c_str(), &textPos, COLOR_WHITE);

		if (!component.TooltipText.empty())
		{
			textPos.Y += titleRect.Height + 2;
			DSurface::Composite->DrawText(component.TooltipText.c_str(), &textPos, COLOR_WHITE);
		}
	}
}