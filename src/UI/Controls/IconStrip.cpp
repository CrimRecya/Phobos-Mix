#include "IconStrip.h"

#include "../Layout.h"

#include <YRMath.h>

namespace UIExt
{
	IconStrip::IconStrip()
		: UIComponent()
	{ }

	IconStrip::IconStrip(int x, int y, int itemWidth, int itemHeight, int spacing)
		: UIComponent(x, y, 0, 0)
		, ItemWidth { itemWidth }
		, ItemHeight { itemHeight }
		, Spacing { spacing }
	{ }

	IconStrip& IconStrip::SetItemSize(int width, int height)
	{
		this->ItemWidth = width;
		this->ItemHeight = height;
		return *this;
	}

	IconStrip& IconStrip::SetSpacing(int spacing)
	{
		this->Spacing = spacing;
		return *this;
	}

	void IconStrip::RebuildItems(size_t count, const ItemBuilder& builder)
	{
		this->GetChildren().clear();

		for (size_t i = 0; i < count; ++i)
		{
			if (builder)
				builder(*this, i);
		}

		this->Refresh();
	}

	void IconStrip::Refresh()
	{
		std::vector<UIComponent*> items;

		for (auto& child : this->GetChildren())
		{
			if (child)
			{
				child->SetSize(this->ItemWidth, this->ItemHeight);
				items.push_back(child.get());
			}
		}

		this->SetSize(this->ItemWidth, this->ItemHeight * static_cast<int>(items.size()) + this->Spacing * Math::max(0, static_cast<int>(items.size()) - 1));
		Layout::ArrangeColumn(items, this->X, this->Y, this->Spacing);
		this->UpdateTreePositions();
	}
}