#pragma once

#include "UIComponent.h"

#include <vector>

namespace UIExt
{
	enum class Anchor
	{
		None = 0,
		Center,
		Left,
		Right,
		Top,
		Bottom,
		BottomLeft,
		BottomRight
	};

	// Lightweight layout helpers. They only arrange existing child components.
	class Layout
	{
	public:
		static void ArrangeRow(std::vector<UIComponent*>& items, int x, int y, int spacing);
		static void ArrangeColumn(std::vector<UIComponent*>& items, int x, int y, int spacing);
		static void ArrangeGrid(std::vector<UIComponent*>& items, int x, int y, int columns, int itemWidth, int itemHeight, int gapX, int gapY);
	};
}