#pragma once

#include "../UIComponent.h"

#include <Drawing.h>
#include <Surface.h>

#include <string>

namespace UIExt
{
	class Label : public UIComponent
	{
	public:
		Label();
		Label(int x, int y, std::wstring text = { });

		Label& SetText(std::wstring text);
		Label& SetColor(COLORREF color);
		Label& SetLineSpacing(int spacing);

		void BindText(const Observable<std::wstring>& observable);

		bool Draw(bool forced) override;

	protected:
		std::wstring Text { };
		COLORREF Color { COLOR_WHITE };
		int LineSpacing { 0 };
	};
}