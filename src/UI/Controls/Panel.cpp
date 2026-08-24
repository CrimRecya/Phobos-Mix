#include "Panel.h"

namespace UIExt
{
	Panel::Panel()
		: UIComponent()
	{ }

	Panel::Panel(int x, int y, int width, int height)
		: UIComponent(x, y, width, height)
	{ }

	Panel& Panel::SetBackColor(ColorStruct color, int opacity)
	{
		this->FillColor = color;
		this->FillOpacity = opacity;
		return *this;
	}

	Panel& Panel::SetBorder(bool enabled, COLORREF color)
	{
		this->DrawBorder = enabled;
		this->BorderColor = color;
		return *this;
	}

	Panel& Panel::SetCustomDraw(DrawCallback callback)
	{
		this->CustomDraw_ = std::move(callback);
		return *this;
	}

	bool Panel::Draw(bool forced)
	{
		if (!this->IsVisibleInTree())
			return false;

		this->UIComponent::Draw(forced);

		RectangleStruct rect { this->X, this->Y, this->Width, this->Height };

		if (this->FillOpacity > 0)
			DSurface::Composite->FillRectTrans(&rect, &this->FillColor, this->FillOpacity);

		if (this->DrawBorder)
			DSurface::Composite->DrawRect(&rect, this->BorderColor);

		if (this->CustomDraw_)
			this->CustomDraw_(*this);

		return false;
	}
}