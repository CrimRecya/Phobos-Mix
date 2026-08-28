#include "Dialog.h"

namespace UIExt
{
	Dialog::Dialog()
		: Panel()
	{
		this->X = 0;
		this->Y = 0;
		this->Width = 0;
		this->Height = 0;

		auto closeButton = std::make_unique<Button>(this->X + this->Width - 24, this->Y + 2, 22, 18, L"X");
		this->CloseButton_ = closeButton.get();
		this->AddChild(std::move(closeButton));
	}

	Dialog::Dialog(int x, int y, int width, int height, std::wstring title)
		: Panel(x, y, width, height)
		, Title { std::move(title) }
	{
		auto closeButton = std::make_unique<Button>(this->X + this->Width - 24, this->Y + 2, 22, 18, L"X");
		this->CloseButton_ = closeButton.get();
		this->AddChild(std::move(closeButton));
	}

	Dialog& Dialog::SetTitle(std::wstring title)
	{
		this->Title = std::move(title);
		return *this;
	}

	Dialog& Dialog::SetCloseAction(std::function<void()> action)
	{
		if (this->CloseButton_)
			this->CloseButton_->SetOnClick(std::move(action));

		return *this;
	}

	bool Dialog::Draw(bool forced)
	{
		if (!this->IsVisibleInTree())
			return false;

		Panel::Draw(forced);

		if (this->CloseButton_)
		{
			this->CloseButton_->SetPos(this->X + this->Width - 24, this->Y + 2);
			this->CloseButton_->SetSize(22, 18);
		}

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
