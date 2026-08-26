#pragma once

#include "UIComponent.h"

#include <GScreenClass.h>

#include <memory>
#include <utility>
#include <vector>

namespace UIExt
{
	// Modal behaviour is optional and per-dialog.
	enum class ModalLevel
	{
		None = 0,
		BlockArea,
		BlockTactical,
		BlockFullScreen
	};

	// Owns the currently open UI screens.
	// Handles registration with GScreenClass, binding flush and modal input blocking.
	class UIRoot final
	{
	public:
		static UIRoot& Instance();

		UIRoot(const UIRoot&) = delete;
		UIRoot& operator=(const UIRoot&) = delete;

		void Open(std::unique_ptr<UIComponent> root, ModalLevel modal = ModalLevel::None);
		void Close(UIComponent* root);
		void CloseAll();

		// Dynamic child management for already-open roots.
		void AddRootChild(UIComponent* root, std::unique_ptr<UIComponent> child);
		void RemoveRootChild(UIComponent* root, UIComponent* child);

		// Called from the main loop hook.
		void UpdateAndDraw();

		bool IsBlockingTactical() const;
		bool IsBlockingFullScreen() const;
		bool IsBlockingAt(int x, int y) const;
		bool IsComponentOpen(UIComponent* component) const;
		bool HandleMouseWheel(int x, int y, bool down);
		void DrawTooltips();

	private:
		struct Screen
		{
			std::unique_ptr<UIComponent> Root { };
			ModalLevel Modal { ModalLevel::None };
		};

		UIRoot() = default;
		~UIRoot();

		void RegisterTree(UIComponent* component);
		void UnregisterTree(UIComponent* component);
		void FlushBindings();
		void DrawModalMasks();
		void ProcessPendingClose();
		void HandleShortcuts();

		std::vector<Screen> Screens_ { };
		std::vector<UIComponent*> PendingClose_ { };
		std::vector<std::pair<DWORD, bool>> ShortcutKeyStates_ { };
	};
}