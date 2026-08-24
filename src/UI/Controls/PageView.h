#pragma once

#include "../UIComponent.h"

#include <functional>

namespace UIExt
{
	// A simple paginated grid container.
	// It lays children out by page and hides children on other pages.
	class PageView : public UIComponent
	{
	public:
		using PageChangedCallback = std::function<void(int)>;
		using ItemBuilder = std::function<void(UIComponent&, size_t)>;

		PageView();
		PageView(int x, int y, int width, int height);

		PageView& SetGrid(int columns, int rows, int itemWidth, int itemHeight, int gapX = 4, int gapY = 4);
		PageView& SetPage(int page);
		PageView& SetPageIndex(int page);
		PageView& NextPage();
		PageView& PrevPage();
		PageView& SetOnPageChanged(PageChangedCallback callback);
		bool IsPageView() const override { return true; }

		int GetPageCount() const;
		int GetPageIndex() const;
		bool CanNext() const;
		bool CanPrev() const;

		void Refresh();
		void RebuildItems(size_t count, const ItemBuilder& builder);

	protected:
		int Columns { 1 };
		int Rows { 1 };
		int ItemWidth { 0 };
		int ItemHeight { 0 };
		int GapX { 4 };
		int GapY { 4 };
		int PageIndex { 0 };
		PageChangedCallback OnPageChanged_ { };

	private:
		int GetPageSize() const;
		void ApplyPageVisibility();
	};
}