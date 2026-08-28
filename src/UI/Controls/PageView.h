#pragma once

#include "../UIComponent.h"

namespace UIExt
{
	// A simple paginated grid container.
	// It lays children out by page and hides children on other pages.
	class PageView : public UIComponent
	{
	public:
		PageView();
		PageView(int x, int y, int width, int height);

		PageView& SetGrid(int columns, int rows, int itemWidth, int itemHeight, int gapX = 4, int gapY = 4);
		PageView& SetPage(int page);
		PageView& NextPage();
		PageView& PrevPage();

		int GetPageCount() const;
		int GetPageIndex() const;
		bool CanNext() const;
		bool CanPrev() const;

		void Refresh();

	protected:
		int Columns { 1 };
		int Rows { 1 };
		int ItemWidth { 0 };
		int ItemHeight { 0 };
		int GapX { 4 };
		int GapY { 4 };
		int PageIndex { 0 };

	private:
		int GetPageSize() const;
		void ApplyPageVisibility();
	};
}