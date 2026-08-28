#pragma once

#include "../UIComponent.h"

namespace UIExt
{
	// A simple grid container. Children are arranged in a fixed number of columns.
	class ListGrid : public UIComponent
	{
	public:
		ListGrid();
		ListGrid(int x, int y, int width, int height);

		ListGrid& SetColumns(int columns, int itemWidth = 0, int itemHeight = 0, int gapX = 4, int gapY = 4);
		void Refresh();

	protected:
		int Columns { 1 };
		int ItemWidth { 0 };
		int ItemHeight { 0 };
		int GapX { 4 };
		int GapY { 4 };
	};
}