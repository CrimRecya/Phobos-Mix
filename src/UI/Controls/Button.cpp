#include "Button.h"

namespace UIExt
{
	Button::Button()
		: UIComponent(0, 0, 0, 0, GadgetFlag::LeftPress | GadgetFlag::RightPress, false)
	{ }

	Button::Button(int x, int y, int width, int height, std::wstring text)
		: UIComponent(x, y, width, height, GadgetFlag::LeftPress | GadgetFlag::RightPress, false)
		, Text { std::move(text) }
	{ }

	Button& Button::SetText(std::wstring text)
	{
		this->Text = std::move(text);
		return *this;
	}

	Button& Button::SetIcon(BSurface* surface)
	{
		this->IconSurface = surface;
		return *this;
	}

	Button& Button::SetOnClick(std::function<void()> callback)
	{
		this->OnClick_ = std::move(callback);
		return *this;
	}

	Button& Button::SetOnRightClick(std::function<void()> callback)
	{
		this->OnRightClick_ = std::move(callback);
		return *this;
	}

	Button& Button::OnClick(std::function<void()> callback)
	{
		return this->SetOnClick(std::move(callback));
	}

	bool Button::Draw(bool forced)
	{
		if (!this->Visible)
			return false;

		RectangleStruct rect { this->X, this->Y, this->Width, this->Height };
		const auto bgColor = this->Disabled ? this->ColorDisabled : (this->Hovering ? this->ColorHover : this->ColorNormal);
		DSurface::Composite->FillRect(&rect, bgColor);

		if (this->IconSurface)
		{
			RectangleStruct iconRect { this->X, this->Y, this->Width, this->Height };
			PCX::Instance.BlitToSurface(&iconRect, DSurface::Composite, this->IconSurface);
		}

		if (!this->Text.empty())
		{
			Point2D position { this->X + this->Width / 2, this->Y + this->Height / 2 };
			RectangleStruct surfaceRect { 0, 0, this->X + this->Width, this->Y + this->Height };
			const auto printType = TextPrintType::Center | TextPrintType::Point8;
			DSurface::Composite->DrawTextA(this->Text.c_str(), &surfaceRect, &position, this->ColorText, 0, printType);
		}

		return false;
	}

	bool Button::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
	{
		if (!this->Disabled)
		{
			if ((flags & GadgetFlag::LeftPress) && this->OnClick_)
				this->OnClick_();

			if ((flags & GadgetFlag::RightPress) && this->OnRightClick_)
				this->OnRightClick_();
		}

		return this->UIComponent::Action(flags, pKey, modifier);
	}
}