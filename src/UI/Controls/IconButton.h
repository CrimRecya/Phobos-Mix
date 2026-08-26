#pragma once

#include "Button.h"

namespace UIExt
{
	// A button optimized for icon-based command bar / skill icons.
	class IconButton : public Button
	{
	public:
		IconButton();
		IconButton(int x, int y, int width, int height);

		UIComponentType GetType() const override { return UIComponentType::IconButton; }
	};
}