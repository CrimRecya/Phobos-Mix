#pragma once

#include "../UIComponent.h"

#include <Drawing.h>
#include <PCX.h>
#include <Surface.h>

#include <functional>
#include <string>

namespace UIExt
{
	// A simple clickable button with optional text and icon.
	// Actual SHP/PCX artwork can be layered on later.
	class Button : public UIComponent
	{
	public:
		Button();
		Button(int x, int y, int width, int height, std::wstring text = { });

		Button& SetText(std::wstring text);
		Button& SetIcon(BSurface* surface);
		Button& SetOnClick(std::function<void()> callback);
		Button& SetOnRightClick(std::function<void()> callback);

		// Fluent alias used by Builder.
		Button& OnClick(std::function<void()> callback);

		bool Draw(bool forced) override;
		bool Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier) override;

	protected:
		std::wstring Text { };
		BSurface* IconSurface { nullptr };
		std::function<void()> OnClick_ { };
		std::function<void()> OnRightClick_ { };
		COLORREF ColorNormal { 0x303030 };
		COLORREF ColorHover { 0x4A4A4A };
		COLORREF ColorDisabled { 0x222222 };
		COLORREF ColorText { COLOR_WHITE };
	};
}