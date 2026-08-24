#include "UIRoot.h"

#include "Controls/Button.h"
#include "Controls/PageView.h"
#include "Controls/Tooltip.h"

#include <MouseClass.h>
#include <Unsorted.h>
#include <Windows.h>
#include <WWMouseClass.h>

#include <algorithm>

namespace UIExt
{
	namespace
	{
		void CallOnCreateTree(UIComponent* component)
		{
			if (!component)
				return;

			component->OnCreate();

			for (auto& child : component->GetChildren())
			{
				if (child)
					CallOnCreateTree(child.get());
			}
		}

		void CallOnDestroyTree(UIComponent* component)
		{
			if (!component)
				return;

			for (auto& child : component->GetChildren())
			{
				if (child)
					CallOnDestroyTree(child.get());
			}

			component->OnDestroy();
		}

		void CollectShortcutButtons(UIComponent* component, std::vector<Button*>& buttons)
		{
			if (!component)
				return;

			if (component->IsButton())
			{
				auto* button = static_cast<Button*>(component);

				if (button->GetShortcutKey() != 0)
					buttons.push_back(button);
			}

			for (auto& child : component->GetChildren())
				CollectShortcutButtons(child.get(), buttons);
		}

		UIComponent* FindTooltipComponentAt(UIComponent* component, int x, int y)
		{
			if (!component || !component->IsVisibleInTree())
				return nullptr;

			// Children are drawn on top of their parent, so search them first.
			for (auto& child : component->GetChildren())
			{
				if (auto* result = FindTooltipComponentAt(child.get(), x, y))
					return result;
			}

			if (x >= component->X && x < component->X + component->Width
				&& y >= component->Y && y < component->Y + component->Height
				&& (!component->TooltipTitle.empty() || !component->TooltipText.empty()))
			{
				return component;
			}

			return nullptr;
		}

		PageView* FindPageViewAt(UIComponent* component, int x, int y)
		{
			if (!component || !component->IsVisibleInTree() || !component->IsEnabledInTree())
				return nullptr;

			// Children are drawn on top, so prefer a hit on a child PageView first.
			for (auto& child : component->GetChildren())
			{
				if (auto* result = FindPageViewAt(child.get(), x, y))
					return result;
			}

			if (component->IsPageView()
				&& x >= component->X && x < component->X + component->Width
				&& y >= component->Y && y < component->Y + component->Height)
			{
				return static_cast<PageView*>(component);
			}

			return nullptr;
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

		CallOnCreateTree(screen.Root.get());
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
			{
				CallOnDestroyTree(screen.Root.get());
				this->UnregisterTree(screen.Root.get());
			}
		}

		this->Screens_.clear();
		this->PendingClose_.clear();
	}

	void UIRoot::AddRootChild(UIComponent* root, std::unique_ptr<UIComponent> child)
	{
		if (!root || !child)
			return;

		auto* raw = root->AddChild(std::move(child));
		CallOnCreateTree(raw);
		this->RegisterTree(raw);
	}

	void UIRoot::RemoveRootChild(UIComponent* root, UIComponent* child)
	{
		if (!root || !child)
			return;

		CallOnDestroyTree(child);
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
		this->HandleShortcuts();

		if (this->IsBlockingTactical())
			MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);

		// BlockFullScreen disables every screen except the active full-screen dialog.
		UIComponent* activeFullScreen = nullptr;

		for (auto it = this->Screens_.rbegin(); it != this->Screens_.rend(); ++it)
		{
			if (it->Modal == ModalLevel::BlockFullScreen)
			{
				activeFullScreen = it->Root.get();
				break;
			}
		}

		for (auto& screen : this->Screens_)
		{
			if (screen.Root)
			{
				screen.Root->SetEnabled(!activeFullScreen || screen.Root.get() == activeFullScreen);
				screen.Root->ApplyAnchors();
				screen.Root->UpdateTreePositions();
				screen.Root->UpdateTree();
			}
		}

		this->FlushBindings();

		// Bindings may change sizes/visibility (e.g. an IconStrip gaining icons),
		// so re-run anchor + position propagation before drawing to avoid a
		// one-frame jump caused by stale layout data.
		for (auto& screen : this->Screens_)
		{
			if (screen.Root)
			{
				screen.Root->ApplyAnchors();
				screen.Root->UpdateTreePositions();
			}
		}

		this->DrawModalMasks();
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

	bool UIRoot::IsBlockingAt(int x, int y) const
	{
		for (const auto& screen : this->Screens_)
		{
			switch (screen.Modal)
			{
			case ModalLevel::BlockTactical:
			case ModalLevel::BlockFullScreen:
				return true;
			case ModalLevel::BlockArea:
				if (screen.Root
					&& x >= screen.Root->X && x < screen.Root->X + screen.Root->Width
					&& y >= screen.Root->Y && y < screen.Root->Y + screen.Root->Height)
				{
					return true;
				}
				break;
			default:
				break;
			}
		}

		return false;
	}

	bool UIRoot::HandleMouseWheel(int x, int y, bool down)
	{
		for (auto it = this->Screens_.rbegin(); it != this->Screens_.rend(); ++it)
		{
			if (!it->Root)
				continue;

			if (auto* pageView = FindPageViewAt(it->Root.get(), x, y))
			{
				if (down)
					pageView->NextPage();
				else
					pageView->PrevPage();

				return true;
			}
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
					CallOnDestroyTree(root);
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

	void UIRoot::HandleShortcuts()
	{
		if (!Game::IsFocused || Game::SpecialDialog != 0)
			return;

		std::vector<Button*> buttons;
		std::vector<DWORD> handled;

		// Topmost screens first.
		for (auto it = this->Screens_.rbegin(); it != this->Screens_.rend(); ++it)
		{
			if (it->Root)
				CollectShortcutButtons(it->Root.get(), buttons);
		}

		for (auto* button : buttons)
		{
			if (!button)
				continue;

			const DWORD key = button->GetShortcutKey();

			if (key == 0 || std::find(handled.begin(), handled.end(), key) != handled.end())
				continue;

			const bool isDown = (GetAsyncKeyState(static_cast<int>(key)) & 0x8000) != 0;
			bool prevDown = false;
			bool found = false;

			for (auto& state : this->ShortcutKeyStates_)
			{
				if (state.first == key)
				{
					prevDown = state.second;
					state.second = isDown;
					found = true;
					break;
				}
			}

			if (!found)
			{
				// First time we track this key: don't fire if it is already held.
				this->ShortcutKeyStates_.push_back({ key, isDown });
				continue;
			}

			if (isDown && !prevDown)
			{
				button->Click();
				handled.push_back(key);
				return;
			}
		}
	}

	void UIRoot::DrawModalMasks()
	{
		ColorStruct maskColor { 0, 0, 0 };

		for (const auto& screen : this->Screens_)
		{
			if (!screen.Root || !screen.Root->IsVisibleInTree())
				continue;

			if (screen.Modal == ModalLevel::BlockFullScreen)
			{
				RectangleStruct fullRect = DSurface::ViewBounds;
				DSurface::Composite->FillRectTrans(&fullRect, &maskColor, 80);
			}
			else if (screen.Modal == ModalLevel::BlockArea)
			{
				RectangleStruct areaRect { screen.Root->X, screen.Root->Y, screen.Root->Width, screen.Root->Height };
				DSurface::Composite->FillRectTrans(&areaRect, &maskColor, 40);
			}
		}
	}

	void UIRoot::DrawTooltips()
	{
		const auto& mouse = WWMouseClass::Instance->XY1;

		for (auto& screen : this->Screens_)
		{
			if (!screen.Root)
				continue;

			if (auto* component = FindTooltipComponentAt(screen.Root.get(), mouse.X, mouse.Y))
				TooltipRenderer::Draw(*component);
		}
	}
}