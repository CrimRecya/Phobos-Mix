#include "ListGrid.h"

#include "../Layout.h"

namespace UIExt
{
	ListGrid::ListGrid()
		: UIComponent()
	{ }

	ListGrid::ListGrid(int x, int y, int width, int height)
		: UIComponent(x, y, width, height)
	{ }

	ListGrid& ListGrid::SetColumns(int columns, int itemWidth, int itemHeight, int gapX, int gapY)
	{
		this->Columns = columns;
		this->ItemWidth = itemWidth;
		this->ItemHeight = itemHeight;
		this->GapX = gapX;
		this->GapY = gapY;
		this->Refresh();
		return *this;
	}

	void ListGrid::Refresh()
	{
		std::vector<UIComponent*> items;

		for (auto& child : this->GetChildren())
		{
			if (child)
				items.push_back(child.get());
		}

		Layout::ArrangeGrid(items, this->X, this->Y, this->Columns, this->ItemWidth, this->ItemHeight, this->GapX, this->GapY);
	}
}