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

		int contentWidth = titleRect.Width;
		int contentHeight = titleRect.Height;

		if (!component.TooltipText.empty())
		{
			RectangleStruct textRect = Drawing::GetTextDimensions(component.TooltipText.c_str(), location, 0, 3, 2);
			contentWidth = Math::max(contentWidth, textRect.Width);
			contentHeight += textRect.Height + component.TooltipLineSpacing;
		}

		const int width = contentWidth + component.TooltipPadding * 2;
		const int height = contentHeight + component.TooltipPadding * 2;

		// Prefer showing the tooltip on the right side, but flip to the left
		// when it would cross the right edge (e.g. right-side icon strips).
		if (location.X + width > DSurface::ViewBounds.X + DSurface::ViewBounds.Width)
			location.X = component.X - width - 8;

		// Prefer below, but flip above when it would cross the bottom edge.
		if (location.Y + height > DSurface::ViewBounds.Y + DSurface::ViewBounds.Height)
			location.Y = component.Y - height - 8;

		// Keep the tooltip inside the visible screen area.
		if (location.X < DSurface::ViewBounds.X)
			location.X = DSurface::ViewBounds.X;
		if (location.Y < DSurface::ViewBounds.Y)
			location.Y = DSurface::ViewBounds.Y;

		RectangleStruct drawRect { location.X, location.Y, width, height };
		ColorStruct bgColor { 0, 0, 0 };
		DSurface::Composite->FillRectTrans(&drawRect, &bgColor, 40);
		DSurface::Composite->DrawRect(&drawRect, COLOR_WHITE);

		Point2D textPos = location + Point2D { component.TooltipPadding, component.TooltipPadding };
		DSurface::Composite->DrawText(component.TooltipTitle.c_str(), &textPos, COLOR_WHITE);

		if (!component.TooltipText.empty())
		{
			textPos.Y += titleRect.Height + component.TooltipLineSpacing;
			DSurface::Composite->DrawText(component.TooltipText.c_str(), &textPos, COLOR_WHITE);
		}
	}
}
