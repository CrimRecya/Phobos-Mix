#include "Dialog.h"

namespace UIExt
{
	Dialog::Dialog()
		: Panel()
	{ }

	Dialog::Dialog(int x, int y, int width, int height, std::wstring title)
		: Panel(x, y, width, height)
		, Title { std::move(title) }
	{ }

	Dialog& Dialog::SetTitle(std::wstring title)
	{
		this->Title = std::move(title);
		return *this;
	}

	bool Dialog::Draw(bool forced)
	{
		if (!this->Visible)
			return false;

		Panel::Draw(forced);

		if (!this->Title.empty())
		{
			Point2D position { this->X + this->Width / 2, this->Y + 3 };
			RectangleStruct rect { 0, 0, this->X + this->Width, this->Y + this->Height };
			const auto printType = TextPrintType::Center | TextPrintType::Point8;
			const auto color = Drawing::RGB_To_Int(Drawing::TooltipColor);
			DSurface::Composite->DrawTextA(this->Title.c_str(), &rect, &position, color, 0, printType);
		}

		return false;
	}
}