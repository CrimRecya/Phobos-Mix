#include "PageView.h"

#include "../Layout.h"

#include <YRMath.h>

namespace UIExt
{
	PageView::PageView()
		: UIComponent()
	{ }

	PageView::PageView(int x, int y, int width, int height)
		: UIComponent(x, y, width, height)
	{ }

	PageView& PageView::SetGrid(int columns, int rows, int itemWidth, int itemHeight, int gapX, int gapY)
	{
		this->Columns = columns;
		this->Rows = rows;
		this->ItemWidth = itemWidth;
		this->ItemHeight = itemHeight;
		this->GapX = gapX;
		this->GapY = gapY;
		this->Refresh();
		return *this;
	}

	PageView& PageView::SetPage(int page)
	{
		this->PageIndex = page;
		this->Refresh();
		return *this;
	}

	PageView& PageView::NextPage()
	{
		if (this->CanNext())
			this->SetPage(this->PageIndex + 1);

		return *this;
	}

	PageView& PageView::PrevPage()
	{
		if (this->CanPrev())
			this->SetPage(this->PageIndex - 1);

		return *this;
	}

	int PageView::GetPageCount() const
	{
		const int pageSize = this->GetPageSize();

		if (pageSize <= 0)
			return 1;

		const auto childCount = this->GetChildren().size();
		return Math::max(1, static_cast<int>((childCount + pageSize - 1) / pageSize));
	}

	int PageView::GetPageIndex() const
	{
		return this->PageIndex;
	}

	bool PageView::CanNext() const
	{
		return this->PageIndex < this->GetPageCount() - 1;
	}

	bool PageView::CanPrev() const
	{
		return this->PageIndex > 0;
	}

	void PageView::Refresh()
	{
		const int pageCount = this->GetPageCount();

		if (this->PageIndex >= pageCount)
			this->PageIndex = pageCount - 1;

		if (this->PageIndex < 0)
			this->PageIndex = 0;

		this->ApplyPageVisibility();

		const int pageSize = this->GetPageSize();
		std::vector<UIComponent*> pageItems;

		for (size_t i = this->PageIndex * pageSize; i < this->GetChildren().size() && i < static_cast<size_t>((this->PageIndex + 1) * pageSize); ++i)
		{
			if (auto* child = this->GetChildren()[i].get())
				pageItems.push_back(child);
		}

		Layout::ArrangeGrid(pageItems, this->X, this->Y, this->Columns, this->ItemWidth, this->ItemHeight, this->GapX, this->GapY);
	}

	int PageView::GetPageSize() const
	{
		return this->Columns * this->Rows;
	}

	void PageView::ApplyPageVisibility()
	{
		const int pageSize = this->GetPageSize();
		const int pageStart = this->PageIndex * pageSize;
		const int pageEnd = pageStart + pageSize;

		for (size_t i = 0; i < this->GetChildren().size(); ++i)
		{
			if (auto* child = this->GetChildren()[i].get())
			{
				const bool onPage = static_cast<int>(i) >= pageStart && static_cast<int>(i) < pageEnd;
				child->SetVisible(onPage);
				child->SetEnabled(onPage);
			}
		}
	}
}