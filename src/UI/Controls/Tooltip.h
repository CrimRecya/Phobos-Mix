#pragma once

#include "../UIComponent.h"

namespace UIExt
{
	// Renders the tooltip attached to a UIComponent.
	class TooltipRenderer
	{
	public:
		static void Draw(const UIComponent& component);
	};
}