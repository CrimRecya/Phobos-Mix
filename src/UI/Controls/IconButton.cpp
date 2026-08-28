#include "IconButton.h"

namespace UIExt
{
	IconButton::IconButton()
		: Button()
	{ }

	IconButton::IconButton(int x, int y, int width, int height, BSurface* icon)
		: Button(x, y, width, height)
	{
		this->SetIcon(icon);
	}

	IconButton& IconButton::SetIconSurface(BSurface* surface)
	{
		this->SetIcon(surface);
		return *this;
	}
}