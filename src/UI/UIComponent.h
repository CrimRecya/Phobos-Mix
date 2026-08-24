#pragma once

#include <GadgetClass.h>
#include <MouseClass.h>
#include <ScenarioClass.h>
#include <Surface.h>

#include "Mvvm.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace UIExt
{
	// Simple anchor points. Anchors are resolved against the parent component,
	// or against the screen bounds when the component has no parent.
	enum class Anchor
	{
		None = 0,
		Center,
		Left,
		Right,
		Top,
		TopLeft,
		TopRight,
		Bottom,
		BottomLeft,
		BottomRight
	};

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
		virtual void OnCreate() { }
		virtual void OnDestroy() { }
		virtual void OnUpdate() { }
		virtual void OnDraw() { }
		virtual void OnEnter() { }
		virtual void OnLeave() { }
		virtual bool IsDialog() const { return false; }
		virtual bool IsPageView() const { return false; }
		virtual bool IsButton() const { return false; }

		// Public state.
		UIComponent* Parent { nullptr };
		bool Visible { true };
		bool Enabled { true };
		bool Hovering { false };
		std::wstring TooltipTitle { };
		std::wstring TooltipText { };
		int TooltipPadding { 5 };
		int TooltipLineSpacing { 1 };
		Anchor AnchorPoint { Anchor::None };
		int AnchorOffsetX { 0 };
		int AnchorOffsetY { 0 };

		// Child management.
		UIComponent* AddChild(std::unique_ptr<UIComponent> child)
		{
			if (!child)
				return nullptr;

			child->Parent = this;
			child->UpdateRelativePosition();
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
			this->X = x;
			this->Y = y;
			this->UpdateRelativePosition();
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

		void SetTooltipPadding(int padding)
		{
			this->TooltipPadding = padding;
		}

		void SetTooltipLineSpacing(int spacing)
		{
			this->TooltipLineSpacing = spacing;
		}

		void SetAnchor(Anchor anchor)
		{
			this->AnchorPoint = anchor;
		}

		void SetAnchor(Anchor anchor, int offsetX, int offsetY)
		{
			this->AnchorPoint = anchor;
			this->AnchorOffsetX = offsetX;
			this->AnchorOffsetY = offsetY;
		}

		void SetAnchorOffset(int offsetX, int offsetY)
		{
			this->AnchorOffsetX = offsetX;
			this->AnchorOffsetY = offsetY;
		}

		void SetRelativePosition(int x, int y)
		{
			this->RelativeX = x;
			this->RelativeY = y;

			if (this->Parent)
			{
				this->X = this->Parent->X + x;
				this->Y = this->Parent->Y + y;
			}
			else
			{
				this->X = x;
				this->Y = y;
			}
		}

		// Effective visibility/enabled state walks up the parent chain.
		bool IsVisibleInTree() const
		{
			for (const auto* node = this; node; node = node->Parent)
			{
				if (!node->Visible)
					return false;
			}

			return true;
		}

		bool IsEnabledInTree() const
		{
			for (const auto* node = this; node; node = node->Parent)
			{
				if (!node->Enabled)
					return false;
			}

			return true;
		}

		bool CanInteract() const
		{
			return this->IsEnabledInTree()
				&& this->IsVisibleInTree()
				&& !ScenarioClass::Instance->UserInputLocked;
		}

		// MVVM binding helpers.
		template <typename T, typename F>
		void BindValue(const Observable<T>& observable, F&& setter)
		{
			this->Bindings_.emplace_back(std::make_unique<ValueBinding<T>>(&observable, std::function<void(const T&)>(std::forward<F>(setter))));
		}

		void BindVisible(const Observable<bool>& observable)
		{
			this->BindValue(observable, [this](const bool& value)
			{
				this->SetVisible(value);
			});
		}

		void BindEnabled(const Observable<bool>& observable)
		{
			this->BindValue(observable, [this](const bool& value)
			{
				this->SetEnabled(value);
			});
		}

		void BindTooltipTitle(const Observable<std::wstring>& observable)
		{
			this->BindValue(observable, [this](const std::wstring& value)
			{
				this->TooltipTitle = value;
			});
		}

		void BindTooltipText(const Observable<std::wstring>& observable)
		{
			this->BindValue(observable, [this](const std::wstring& value)
			{
				this->TooltipText = value;
			});
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

		// Recursive helpers used by UIRoot.
		void UpdateTreePositions()
		{
			if (this->Parent)
			{
				this->X = this->Parent->X + this->RelativeX;
				this->Y = this->Parent->Y + this->RelativeY;
			}

			for (auto& child : this->Children_)
			{
				if (child)
					child->UpdateTreePositions();
			}
		}

		void ApplyAnchors()
		{
			if (this->AnchorPoint != Anchor::None)
				this->ApplyAnchor();

			for (auto& child : this->Children_)
			{
				if (child)
					child->ApplyAnchors();
			}
		}

		void UpdateTree()
		{
			this->OnUpdate();

			for (auto& child : this->Children_)
			{
				if (child)
					child->UpdateTree();
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
			if (!this->IsVisibleInTree())
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
			if (!this->CanInteract())
				return false;

			if (this->OnAction_)
				this->OnAction_(flags, pKey, modifier);

			return this->GadgetClass::Action(flags, pKey, modifier);
		}

	protected:
		void ApplyAnchor()
		{
			const int containerX = this->Parent ? this->Parent->X : 0;
			const int containerY = this->Parent ? this->Parent->Y : 0;
			const int containerWidth = this->Parent ? this->Parent->Width : DSurface::ViewBounds.Width;
			const int containerHeight = this->Parent ? this->Parent->Height : DSurface::ViewBounds.Height;

			int newX = this->X;
			int newY = this->Y;

			switch (this->AnchorPoint)
			{
			case Anchor::Center:
				newX = containerX + (containerWidth - this->Width) / 2;
				newY = containerY + (containerHeight - this->Height) / 2;
				break;
			case Anchor::Left:
				newX = containerX;
				newY = containerY + (containerHeight - this->Height) / 2;
				break;
			case Anchor::Right:
				newX = containerX + containerWidth - this->Width;
				newY = containerY + (containerHeight - this->Height) / 2;
				break;
			case Anchor::Top:
				newX = containerX + (containerWidth - this->Width) / 2;
				newY = containerY;
				break;
			case Anchor::TopLeft:
				newX = containerX;
				newY = containerY;
				break;
			case Anchor::TopRight:
				newX = containerX + containerWidth - this->Width;
				newY = containerY;
				break;
			case Anchor::Bottom:
				newX = containerX + (containerWidth - this->Width) / 2;
				newY = containerY + containerHeight - this->Height;
				break;
			case Anchor::BottomLeft:
				newX = containerX;
				newY = containerY + containerHeight - this->Height;
				break;
			case Anchor::BottomRight:
				newX = containerX + containerWidth - this->Width;
				newY = containerY + containerHeight - this->Height;
				break;
			default:
				break;
			}

			newX += this->AnchorOffsetX;
			newY += this->AnchorOffsetY;
			this->SetPos(newX, newY);
		}

		void UpdateRelativePosition()
		{
			if (this->Parent)
			{
				this->RelativeX = this->X - this->Parent->X;
				this->RelativeY = this->Y - this->Parent->Y;
			}
			else
			{
				this->RelativeX = this->X;
				this->RelativeY = this->Y;
			}
		}

	private:
		std::vector<std::unique_ptr<UIComponent>> Children_ { };
		std::vector<std::unique_ptr<BindingBase>> Bindings_ { };
		std::function<void()> OnMouseEnter_ { };
		std::function<void()> OnMouseLeave_ { };
		std::function<void(GadgetFlag, DWORD*, KeyModifier)> OnAction_ { };
		int RelativeX { 0 };
		int RelativeY { 0 };
	};
}
