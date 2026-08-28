#pragma once

#include "../UIComponent.h"

namespace UIExt
{
	// Renders the tooltip attached to a UIComponent.
	// layoutHost is the component whose rectangle the tooltip is placed against;
	// pass null (or the component itself) for the default placement next to the
	// component's own top-right corner.
	class TooltipRenderer
	{
	public:
		static void Draw(const UIComponent& component, const UIComponent* layoutHost = nullptr);
	};
}