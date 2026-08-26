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

	Button::~Button()
	{
		if (this->BoundCommand_ && this->CommandToken_ != static_cast<size_t>(-1))
			this->BoundCommand_->RemoveCanExecuteChanged(this->CommandToken_);
	}

	Button& Button::SetText(std::wstring text)
	{
		this->Text = std::move(text);
		return *this;
	}

	Button& Button::SetIconFile(const char* filename)
	{
		this->IconFile = filename ? filename : "";
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

	Button& Button::SetShortcut(DWORD key)
	{
		this->ShortcutKey = key;
		return *this;
	}

	Button& Button::SetShortcut(WWKey key)
	{
		this->ShortcutKey = static_cast<DWORD>(key);
		return *this;
	}

	Button& Button::SetTextOffset(int x, int y)
	{
		this->TextOffsetX = x;
		this->TextOffsetY = y;
		return *this;
	}

	Button& Button::SetTextAnchor(TextAlign align, TextVAlign valign)
	{
		this->TextAlign_ = align;
		this->TextVAlign_ = valign;
		return *this;
	}

	Button& Button::OnClick(std::function<void()> callback)
	{
		return this->SetOnClick(std::move(callback));
	}

	void Button::Click()
	{
		if (this->CanInteract() && this->OnClick_)
			this->OnClick_();
	}

	void Button::BindText(const Observable<std::wstring>& observable)
	{
		this->BindValue(observable, [this](const std::wstring& value)
		{
			this->SetText(value);
		});
	}

	void Button::BindCommand(Command& command)
	{
		if (this->BoundCommand_ && this->CommandToken_ != static_cast<size_t>(-1))
			this->BoundCommand_->RemoveCanExecuteChanged(this->CommandToken_);

		this->BoundCommand_ = &command;
		this->CommandToken_ = command.AddCanExecuteChanged([this]
		{
			this->SetEnabled(this->BoundCommand_ && this->BoundCommand_->CanExecute());
		});

		this->SetOnClick([&command]
		{
			if (command.CanExecute())
				command.Execute();
		});

		this->SetEnabled(command.CanExecute());
	}

	bool Button::Draw(bool forced)
	{
		if (!this->IsVisibleInTree())
			return false;

		this->UIComponent::Draw(forced);

		RectangleStruct rect { this->X, this->Y, this->Width, this->Height };
		const auto bgColor = this->Disabled ? this->ColorDisabled : (this->Hovering ? this->ColorHover : this->ColorNormal);
		DSurface::Composite->FillRect(&rect, bgColor);

		if (auto* surface = this->IconFile.GetSurface())
		{
			RectangleStruct iconRect { this->X, this->Y, this->Width, this->Height };
			PCX::Instance.BlitToSurface(&iconRect, DSurface::Composite, surface);
		}

		if (!this->Text.empty())
		{
			Point2D dummy { 0, 0 };
			const auto textRect = Drawing::GetTextDimensions(this->Text.c_str(), dummy, static_cast<WORD>(TextPrintType::Point8));

			int textX = this->X;
			int textY = this->Y;

			switch (this->TextAlign_)
			{
			case TextAlign::Left:
				textX = this->X;
				break;
			case TextAlign::Center:
				textX = this->X + (this->Width - textRect.Width) / 2;
				break;
			case TextAlign::Right:
				textX = this->X + this->Width - textRect.Width;
				break;
			}

			switch (this->TextVAlign_)
			{
			case TextVAlign::Top:
				textY = this->Y;
				break;
			case TextVAlign::Middle:
				textY = this->Y + (this->Height - textRect.Height) / 2;
				break;
			case TextVAlign::Bottom:
				textY = this->Y + this->Height - textRect.Height;
				break;
			}

			Point2D position { textX + this->TextOffsetX, textY + this->TextOffsetY };
			RectangleStruct surfaceRect { 0, 0, this->X + this->Width, this->Y + this->Height };
			DSurface::Composite->DrawTextA(this->Text.c_str(), &surfaceRect, &position, this->ColorText, 0, TextPrintType::Point8);
		}

		if (this->Hovering && !this->Disabled)
			DSurface::Composite->DrawRect(&rect, COLOR_WHITE);

		return false;
	}

	bool Button::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
	{
		if (!this->CanInteract())
			return false;

		if ((flags & GadgetFlag::LeftPress) && this->OnClick_)
			this->OnClick_();

		if ((flags & GadgetFlag::RightPress) && this->OnRightClick_)
			this->OnRightClick_();

		return this->UIComponent::Action(flags, pKey, modifier);
	}
}
