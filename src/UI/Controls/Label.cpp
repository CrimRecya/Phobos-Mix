#include "Label.h"

#include <vector>

namespace UIExt
{
	namespace
	{
		std::vector<std::wstring> WrapLabelText(const std::wstring& text, int maxWidth)
		{
			std::vector<std::wstring> lines;

			if (maxWidth <= 0)
			{
				lines.push_back(text);
				return lines;
			}

			std::wstring current;
			Point2D dummy { 0, 0 };

			for (const wchar_t ch : text)
			{
				if (ch == L'\n')
				{
					lines.push_back(current);
					current.clear();
					continue;
				}

				const std::wstring candidate = current + ch;
				const auto rect = Drawing::GetTextDimensions(candidate.c_str(), dummy, static_cast<WORD>(TextPrintType::Point8));

				if (rect.Width > maxWidth && !current.empty())
				{
					lines.push_back(current);
					current = ch;
				}
				else
				{
					current = candidate;
				}
			}

			if (!current.empty())
				lines.push_back(current);

			if (lines.empty())
				lines.push_back(text);

			return lines;
		}
	}

	Label::Label()
		: UIComponent()
	{ }

	Label::Label(int x, int y, std::wstring text)
		: UIComponent(x, y, 0, 0)
		, Text { std::move(text) }
	{ }

	Label& Label::SetText(std::wstring text)
	{
		this->Text = std::move(text);
		return *this;
	}

	Label& Label::SetColor(COLORREF color)
	{
		this->Color = color;
		return *this;
	}

	Label& Label::SetLineSpacing(int spacing)
	{
		this->LineSpacing = spacing;
		return *this;
	}

	void Label::BindText(const Observable<std::wstring>& observable)
	{
		this->BindValue(observable, [this](const std::wstring& value)
		{
			this->SetText(value);
		});
	}

	bool Label::Draw(bool forced)
	{
		if (!this->IsVisibleInTree() || this->Text.empty())
			return false;

		this->UIComponent::Draw(forced);

		const auto lines = WrapLabelText(this->Text, this->Width);
		Point2D dummy { 0, 0 };
		const auto lineHeight = Drawing::GetTextDimensions(L"A", dummy, static_cast<WORD>(TextPrintType::Point8)).Height;
		const int lineStep = this->LineSpacing > 0 ? this->LineSpacing : (lineHeight > 1 ? lineHeight - 1 : 1);
		int y = this->Y;

		for (const auto& line : lines)
		{
			Point2D position { this->X, y };
			RectangleStruct rect { 0, 0, this->X + this->Width, this->Y + this->Height };
			DSurface::Composite->DrawTextA(line.c_str(), &rect, &position, this->Color, 0, TextPrintType::Point8);
			y += lineStep;
		}

		return false;
	}
}
