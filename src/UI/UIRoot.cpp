#include "UIRoot.h"

#include "Controls/Tooltip.h"

#include <MouseClass.h>

#include <algorithm>

namespace UIExt
{
	namespace
	{
		void DrawTooltipTree(UIComponent* component)
		{
			if (!component)
				return;

			if (component->Hovering && (!component->TooltipTitle.empty() || !component->TooltipText.empty()))
				TooltipRenderer::Draw(*component);

			for (auto& child : component->GetChildren())
			{
				if (child)
					DrawTooltipTree(child.get());
			}
		}
	}
	UIRoot& UIRoot::Instance()
	{
		static UIRoot instance;
		return instance;
	}

	UIRoot::~UIRoot()
	{
		this->CloseAll();
	}

	void UIRoot::Open(std::unique_ptr<UIComponent> root, ModalLevel modal)
	{
		if (!root)
			return;

		Screen screen;
		screen.Root = std::move(root);
		screen.Modal = modal;

		this->RegisterTree(screen.Root.get());
		this->Screens_.push_back(std::move(screen));
	}

	void UIRoot::Close(UIComponent* root)
	{
		if (!root)
			return;

		// Defer destruction until the next frame so that a control can safely
		// close its own dialog without being destroyed mid-callback.
		if (std::find(this->PendingClose_.begin(), this->PendingClose_.end(), root) == this->PendingClose_.end())
			this->PendingClose_.push_back(root);
	}

	void UIRoot::CloseAll()
	{
		for (auto& screen : this->Screens_)
		{
			if (screen.Root)
				this->UnregisterTree(screen.Root.get());
		}

		this->Screens_.clear();
	}

	void UIRoot::AddRootChild(UIComponent* root, std::unique_ptr<UIComponent> child)
	{
		if (!root || !child)
			return;

		auto* raw = root->AddChild(std::move(child));
		this->RegisterTree(raw);
	}

	void UIRoot::RemoveRootChild(UIComponent* root, UIComponent* child)
	{
		if (!root || !child)
			return;

		this->UnregisterTree(child);

		auto& children = root->GetChildren();
		children.erase(std::remove_if(children.begin(), children.end(),
			[child](const std::unique_ptr<UIComponent>& item)
			{
				return item.get() == child;
			}), children.end());
	}

	void UIRoot::UpdateAndDraw()
	{
		this->ProcessPendingClose();

		if (this->IsBlockingTactical())
			MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);

		this->FlushBindings();
		this->DrawTooltips();
	}

	bool UIRoot::IsBlockingTactical() const
	{
		for (const auto& screen : this->Screens_)
		{
			if (screen.Modal == ModalLevel::BlockTactical || screen.Modal == ModalLevel::BlockFullScreen)
				return true;
		}

		return false;
	}

	bool UIRoot::IsBlockingFullScreen() const
	{
		for (const auto& screen : this->Screens_)
		{
			if (screen.Modal == ModalLevel::BlockFullScreen)
				return true;
		}

		return false;
	}

	void UIRoot::RegisterTree(UIComponent* component)
	{
		if (!component)
			return;

		GScreenClass::Instance.AddButton(component);

		for (auto& child : component->GetChildren())
		{
			if (child)
				this->RegisterTree(child.get());
		}
	}

	void UIRoot::UnregisterTree(UIComponent* component)
	{
		if (!component)
			return;

		for (auto& child : component->GetChildren())
		{
			if (child)
				this->UnregisterTree(child.get());
		}

		GScreenClass::Instance.RemoveButton(component);
	}

	void UIRoot::ProcessPendingClose()
	{
		for (auto* root : this->PendingClose_)
		{
			for (auto it = this->Screens_.begin(); it != this->Screens_.end(); ++it)
			{
				if (it->Root.get() == root)
				{
					this->UnregisterTree(root);
					this->Screens_.erase(it);
					break;
				}
			}
		}

		this->PendingClose_.clear();
	}

	void UIRoot::FlushBindings()
	{
		for (auto& screen : this->Screens_)
		{
			if (screen.Root)
				screen.Root->FlushBindings();
		}
	}

	void UIRoot::DrawTooltips()
	{
		for (auto& screen : this->Screens_)
		{
			if (screen.Root)
				DrawTooltipTree(screen.Root.get());
		}
	}
}