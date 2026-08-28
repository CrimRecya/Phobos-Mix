#pragma once

#include <GadgetClass.h>
#include <MouseClass.h>

#include "Mvvm.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace UIExt
{
	// Base class for all UI framework controls.
	// Every control is a GadgetClass so it can be registered with GScreenClass.
	class UIComponent : public GadgetClass
	{
	public:
		UIComponent()
			: GadgetClass(noinit_t())
		{ }

		UIComponent(int x, int y, int width, int height, GadgetFlag flags = static_cast<GadgetFlag>(0), bool sticky = false)
			: GadgetClass(x, y, width, height, flags, sticky)
		{ }

		~UIComponent() override = default;

		UIComponent(const UIComponent&) = delete;
		UIComponent& operator=(const UIComponent&) = delete;

		// Framework lifecycle / drawing hooks.
		virtual void OnDraw() { }
		virtual void OnEnter() { }
		virtual void OnLeave() { }

		// Public state.
		UIComponent* Parent { nullptr };
		bool Visible { true };
		bool Enabled { true };
		bool Hovering { false };
		std::wstring TooltipTitle { };
		std::wstring TooltipText { };

		// Child management.
		UIComponent* AddChild(std::unique_ptr<UIComponent> child)
		{
			if (!child)
				return nullptr;

			child->Parent = this;
			auto* raw = child.get();
			this->Children_.push_back(std::move(child));
			return raw;
		}

		const std::vector<std::unique_ptr<UIComponent>>& GetChildren() const
		{
			return this->Children_;
		}

		std::vector<std::unique_ptr<UIComponent>>& GetChildren()
		{
			return this->Children_;
		}

		// Common property helpers.
		void SetPos(int x, int y)
		{
			this->SetPosition(x, y);
		}

		void SetSize(int width, int height)
		{
			this->SetDimension(width, height);
		}

		void SetVisible(bool visible)
		{
			this->Visible = visible;
		}

		void SetEnabled(bool enabled)
		{
			this->Enabled = enabled;
			this->Disabled = !enabled;
		}

		void SetTooltip(std::wstring title, std::wstring text)
		{
			this->TooltipTitle = std::move(title);
			this->TooltipText = std::move(text);
		}

		// MVVM binding helpers.
		template <typename T, typename F>
		void BindValue(const Observable<T>& observable, F&& setter)
		{
			this->Bindings_.emplace_back(std::make_unique<ValueBinding<T>>(&observable, std::function<void(const T&)>(std::forward<F>(setter))));
		}

		void FlushBindings()
		{
			for (auto& binding : this->Bindings_)
			{
				if (binding)
					binding->ApplyIfNeeded();
			}

			for (auto& child : this->Children_)
			{
				if (child)
					child->FlushBindings();
			}
		}

		// Event callback registration.
		void SetOnMouseEnter(std::function<void()> callback)
		{
			this->OnMouseEnter_ = std::move(callback);
		}

		void SetOnMouseLeave(std::function<void()> callback)
		{
			this->OnMouseLeave_ = std::move(callback);
		}

		void SetOnAction(std::function<void(GadgetFlag, DWORD*, KeyModifier)> callback)
		{
			this->OnAction_ = std::move(callback);
		}

		// GadgetClass overrides.
		bool Draw(bool forced) override
		{
			if (!this->Visible)
				return false;

			this->OnDraw();
			return false;
		}

		void OnMouseEnter() override
		{
			this->Hovering = true;
			MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);

			if (this->OnMouseEnter_)
				this->OnMouseEnter_();

			this->OnEnter();
		}

		void OnMouseLeave() override
		{
			this->Hovering = false;
			MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);

			if (this->OnMouseLeave_)
				this->OnMouseLeave_();

			this->OnLeave();
		}

		bool Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier) override
		{
			if (this->OnAction_)
				this->OnAction_(flags, pKey, modifier);

			return this->GadgetClass::Action(flags, pKey, modifier);
		}

	private:
		std::vector<std::unique_ptr<UIComponent>> Children_ { };
		std::vector<std::unique_ptr<BindingBase>> Bindings_ { };
		std::function<void()> OnMouseEnter_ { };
		std::function<void()> OnMouseLeave_ { };
		std::function<void(GadgetFlag, DWORD*, KeyModifier)> OnAction_ { };
	};
}