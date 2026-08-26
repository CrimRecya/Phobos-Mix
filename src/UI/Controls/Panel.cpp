#include "Panel.h"

#include <FileSystem.h>

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

	Panel& Panel::SetShpBackground(
		SHPStruct* shp,
		ConvertClass* palette,
		int frame,
		int offsetX,
		int offsetY,
		ShpAlign align)
	{
		this->BackgroundShp = shp;
		this->BackgroundPalette = palette;
		this->BackgroundFrame = frame;
		this->BackgroundOffsetX = offsetX;
		this->BackgroundOffsetY = offsetY;
		this->BackgroundAlign = align;
		return *this;
	}

	Panel& Panel::ClearShpBackground()
	{
		this->BackgroundShp = nullptr;
		this->BackgroundPalette = nullptr;
		this->BackgroundFrame = 0;
		this->BackgroundOffsetX = 0;
		this->BackgroundOffsetY = 0;
		this->BackgroundAlign = ShpAlign::TopLeft;
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

		if (this->BackgroundShp && this->BackgroundShp->Frames > 0)
		{
			int x = this->X + this->BackgroundOffsetX;
			int y = this->Y + this->BackgroundOffsetY;

			const int shpWidth = this->BackgroundShp->Width;
			const int shpHeight = this->BackgroundShp->Height;

			switch (this->BackgroundAlign)
			{
			case ShpAlign::TopLeft:
				break;
			case ShpAlign::BottomLeft:
				y = this->Y + this->Height - shpHeight + this->BackgroundOffsetY;
				break;
			case ShpAlign::TopRight:
				x = this->X + this->Width - shpWidth + this->BackgroundOffsetX;
				break;
			case ShpAlign::BottomRight:
				x = this->X + this->Width - shpWidth + this->BackgroundOffsetX;
				y = this->Y + this->Height - shpHeight + this->BackgroundOffsetY;
				break;
			case ShpAlign::Center:
				x = this->X + (this->Width - shpWidth) / 2 + this->BackgroundOffsetX;
				y = this->Y + (this->Height - shpHeight) / 2 + this->BackgroundOffsetY;
				break;
			}

			Point2D position { x, y };
			RectangleStruct surfaceRect { 0, 0, this->X + this->Width, this->Y + this->Height };
			auto* palette = this->BackgroundPalette ? this->BackgroundPalette : FileSystem::ANIM_PAL;

			DSurface::Composite->DrawSHP(palette, this->BackgroundShp, this->BackgroundFrame,
				&position, &surfaceRect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}

		if (this->CustomDraw_)
			this->CustomDraw_(*this);

		return false;
	}
}