#pragma once

#include "../UIComponent.h"

#include <Drawing.h>
#include <PCX.h>
#include <Surface.h>

#include <Utilities/Constructs.h>

#include <cstddef>
#include <functional>
#include <string>

namespace UIExt
{
	enum class TextAlign
	{
		Left,
		Center,
		Right
	};

	enum class TextVAlign
	{
		Top,
		Middle,
		Bottom
	};

	// A simple clickable button with optional text and icon.
	// Actual SHP/PCX artwork can be layered on later.
	class Button : public UIComponent
	{
	public:
		Button();
		Button(int x, int y, int width, int height, std::wstring text = { });
		~Button() override;

		Button& SetText(std::wstring text);
		Button& SetIconFile(const char* filename);
		Button& SetOnClick(std::function<void()> callback);
		Button& SetOnRightClick(std::function<void()> callback);
		Button& SetShortcut(DWORD key);
		Button& SetShortcut(WWKey key);
		Button& SetTextOffset(int x, int y);
		Button& SetTextAnchor(TextAlign align, TextVAlign valign);
		Button& SetColor(COLORREF normal, COLORREF hover, COLORREF disabled, COLORREF text);
		Button& SetFillOpacity(int opacity);
		Button& SetDrawHoverBorder(bool draw);

		// Fluent alias used by Builder.
		Button& OnClick(std::function<void()> callback);

		// Programmatically trigger the button's primary click action.
		void Click();
		DWORD GetShortcutKey() const { return this->ShortcutKey; }

		// MVVM helpers.
		void BindText(const Observable<std::wstring>& observable);
		void BindCommand(Command& command);

		bool Draw(bool forced) override;
		bool Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier) override;
		UIComponentType GetType() const override { return UIComponentType::Button; }

	protected:
		std::wstring Text { };
		PhobosPCXFile IconFile { };
		std::function<void()> OnClick_ { };
		std::function<void()> OnRightClick_ { };
		DWORD ShortcutKey { 0 };
		Command* BoundCommand_ { nullptr };
		size_t CommandToken_ { static_cast<size_t>(-1) };
		COLORREF ColorNormal { 0x303030 };
		COLORREF ColorHover { 0x4A4A4A };
		COLORREF ColorDisabled { 0x222222 };
		COLORREF ColorText { COLOR_WHITE };
		TextAlign TextAlign_ { TextAlign::Center };
		TextVAlign TextVAlign_ { TextVAlign::Middle };
		int TextOffsetX { 0 };
		int TextOffsetY { 1 };
		int FillOpacity { 100 };
		bool DrawHoverBorder { true };
	};
}