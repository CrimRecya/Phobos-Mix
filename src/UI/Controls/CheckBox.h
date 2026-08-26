#pragma once

#include "../UIComponent.h"

#include <Drawing.h>
#include <Surface.h>

#include <functional>
#include <string>

namespace UIExt
{
	class CheckBox : public UIComponent
	{
	public:
		CheckBox();
		CheckBox(int x, int y, int width, int height, std::wstring text = { });

		CheckBox& SetText(std::wstring text);
		CheckBox& SetChecked(bool checked);
		CheckBox& SetOnToggle(std::function<void(bool)> callback);

		bool IsChecked() const;

		void BindChecked(const Observable<bool>& observable)
		{
			this->BindValue(observable, [this](const bool& value)
			{
				this->SetChecked(value);
			});
		}

		void BindText(const Observable<std::wstring>& observable);

		UIComponentType GetType() const override { return UIComponentType::CheckBox; }

		bool Draw(bool forced) override;
		bool Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier) override;

	protected:
		std::wstring Text { };
		bool Checked { false };
		std::function<void(bool)> OnToggle_ { };
	};
}