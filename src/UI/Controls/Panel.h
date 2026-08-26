#pragma once

#include "../UIComponent.h"

#include <Drawing.h>
#include <Surface.h>

#include <functional>

namespace UIExt
{
	// Simple background panel. Subclasses / game-specific code can draw SHP/PCX art later.
	class Panel : public UIComponent
	{
	public:
		using DrawCallback = std::function<void(Panel&)>;

		enum class ShpAlign
		{
			TopLeft = 0,
			BottomLeft = 1,
			TopRight = 2,
			BottomRight = 3,
			Center = 4
		};

		Panel();
		Panel(int x, int y, int width, int height);

		Panel& SetBackColor(ColorStruct color, int opacity = 40);
		Panel& SetBorder(bool enabled, COLORREF color = COLOR_WHITE);
		Panel& SetCustomDraw(DrawCallback callback);

		// Draws a SHP frame as a background decoration on this panel.
		// Palette can be null; in that case FileSystem::ANIM_PAL is used.
		Panel& SetShpBackground(
			SHPStruct* shp,
			ConvertClass* palette,
			int frame,
			int offsetX,
			int offsetY,
			ShpAlign align = ShpAlign::TopLeft);

		Panel& ClearShpBackground();

		UIComponentType GetType() const override { return UIComponentType::Panel; }

		bool Draw(bool forced) override;

	protected:
		ColorStruct FillColor { 0, 0, 0 };
		int FillOpacity { 0 };
		bool DrawBorder { false };
		COLORREF BorderColor { COLOR_WHITE };
		DrawCallback CustomDraw_ { };

		SHPStruct* BackgroundShp { nullptr };
		ConvertClass* BackgroundPalette { nullptr };
		int BackgroundFrame { 0 };
		int BackgroundOffsetX { 0 };
		int BackgroundOffsetY { 0 };
		ShpAlign BackgroundAlign { ShpAlign::TopLeft };
	};
}