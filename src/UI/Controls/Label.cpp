#include "Label.h"

namespace UIExt
{
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

	void Label::BindText(const Observable<std::wstring>& observable)
	{
		this->BindValue(observable, [this](const std::wstring& value)
		{
			this->SetText(value);
		});
	}

	bool Label::Draw(bool forced)
	{
		if (!this->Visible || this->Text.empty())
			return false;

		Point2D position { this->X, this->Y };
		RectangleStruct rect { 0, 0, this->X + this->Width, this->Y + this->Height };
		DSurface::Composite->DrawTextA(this->Text.c_str(), &rect, &position, this->Color, 0, TextPrintType::Point8);
		return false;
	}
}