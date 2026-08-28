#include "Layout.h"

namespace UIExt
{
	void Layout::ArrangeRow(std::vector<UIComponent*>& items, int x, int y, int spacing)
	{
		int cursor = x;

		for (auto* item : items)
		{
			if (!item)
				continue;

			item->SetPos(cursor, y);
			cursor += item->Width + spacing;
		}
	}

	void Layout::ArrangeColumn(std::vector<UIComponent*>& items, int x, int y, int spacing)
	{
		int cursor = y;

		for (auto* item : items)
		{
			if (!item)
				continue;

			item->SetPos(x, cursor);
			cursor += item->Height + spacing;
		}
	}

	void Layout::ArrangeGrid(std::vector<UIComponent*>& items, int x, int y, int columns, int itemWidth, int itemHeight, int gapX, int gapY)
	{
		if (columns <= 0)
			return;

		for (size_t i = 0; i < items.size(); ++i)
		{
			auto* item = items[i];

			if (!item)
				continue;

			const int row = static_cast<int>(i / columns);
			const int col = static_cast<int>(i % columns);
			item->SetPos(x + col * (itemWidth + gapX), y + row * (itemHeight + gapY));

			if (itemWidth > 0 && itemHeight > 0)
				item->SetSize(itemWidth, itemHeight);
		}
	}
}