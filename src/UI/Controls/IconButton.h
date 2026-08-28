#pragma once

#include "Button.h"

namespace UIExt
{
	// A button optimized for icon-based command bar / skill icons.
	class IconButton : public Button
	{
	public:
		IconButton();
		IconButton(int x, int y, int width, int height, BSurface* icon = nullptr);

		IconButton& SetIconSurface(BSurface* surface);
	};
}