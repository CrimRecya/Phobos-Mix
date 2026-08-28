#include "CheckBox.h"

namespace UIExt
{
	CheckBox::CheckBox()
		: UIComponent(0, 0, 0, 0, GadgetFlag::LeftPress, false)
	{ }

	CheckBox::CheckBox(int x, int y, int width, int height, std::wstring text)
		: UIComponent(x, y, width, height, GadgetFlag::LeftPress, false)
		, Text { std::move(text) }
	{ }

	CheckBox& CheckBox::SetText(std::wstring text)
	{
		this->Text = std::move(text);
		return *this;
	}

	CheckBox& CheckBox::SetChecked(bool checked)
	{
		this->Checked = checked;
		return *this;
	}

	CheckBox& CheckBox::SetOnToggle(std::function<void(bool)> callback)
	{
		this->OnToggle_ = std::move(callback);
		return *this;
	}

	bool CheckBox::IsChecked() const
	{
		return this->Checked;
	}

	bool CheckBox::Draw(bool forced)
	{
		if (!this->Visible)
			return false;

		constexpr int boxSize = 14;
		RectangleStruct box { this->X, this->Y + (this->Height - boxSize) / 2, boxSize, boxSize };
		DSurface::Composite->FillRect(&box, this->Disabled ? 0x222222 : 0x3A3A3A);

		if (this->Checked)
		{
			RectangleStruct mark { box.X + 3, box.Y + 3, box.Width - 6, box.Height - 6 };
			DSurface::Composite->FillRect(&mark, 0x67EC);
		}

		if (!this->Text.empty())
		{
			Point2D position { this->X + boxSize + 6, this->Y + this->Height / 2 };
			RectangleStruct rect { 0, 0, this->X + this->Width, this->Y + this->Height };
			const auto printType = TextPrintType::Center | TextPrintType::Point8;
			const auto color = this->Disabled ? 0x666666 : COLOR_WHITE;
			DSurface::Composite->DrawTextA(this->Text.c_str(), &rect, &position, color, 0, printType);
		}

		return false;
	}

	bool CheckBox::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
	{
		if (!this->Disabled && (flags & GadgetFlag::LeftPress))
		{
			this->SetChecked(!this->Checked);

			if (this->OnToggle_)
				this->OnToggle_(this->Checked);
		}

		return this->UIComponent::Action(flags, pKey, modifier);
	}
}