// ============================================================================
// SAMPLE - kept as a runnable UI test.
// ============================================================================
#include "OpenMutationUI.h"

#include "MutationDisplayerStrip.h"
#include "MutationSelectorDialog.h"
#include "MutationViewModel.h"

#include <UI/Builder.h>
#include <UI/UIRoot.h>
#include <Surface.h>

#include <Interop/UIExt/UIExtApi.h>

namespace Mutation
{
	namespace
	{
		void OpenMutationUI()
		{
			if (MutationSelectorDialog::IsOpen())
				return;

			auto& viewModel = MutationViewModel::Instance();
			viewModel.Refresh();

			auto dialog = MutationSelectorDialog::Create(viewModel);
			UIExt::UIRoot::Instance().Open(std::move(dialog), UIExt::ModalLevel::BlockTactical);
			MutationSelectorDialog::SetOpen(true);

			MutationDisplayerStrip::Open(viewModel);
		}

		void __stdcall StartButtonClick(void*)
		{
			OpenMutationUI();
		}
	}

	const char* OpenMutationUICommandClass::GetName() const
	{
		return "Open Mutation UI";
	}

	const wchar_t* OpenMutationUICommandClass::GetUIName() const
	{
		return L"Open Mutation UI";
	}

	const wchar_t* OpenMutationUICommandClass::GetUICategory() const
	{
		return CATEGORY_INTERFACE;
	}

	const wchar_t* OpenMutationUICommandClass::GetUIDescription() const
	{
		return L"Opens the mutation selector and displayer UI";
	}

	void OpenMutationUICommandClass::Execute(WWKey eInput) const
	{
		OpenMutationUI();
	}

	void OpenStartButton()
	{
		static void* s_startButton = nullptr;
		if (s_startButton)
			return;

		void* button = nullptr;
		if (UIExt_CreateIconButton(0, 0, 60, 48, &button) != S_OK || !button)
			return;

		// TEMP DEBUG: go through the exact same UIExt_* API sequence that DP's
		// C# P/Invoke path uses.
		UIExt_Button_SetIconFromFile(button, "MutStart.pcx");
		UIExt_SetTooltip(
			button,
			L"\u7a81\u53d8\u56e0\u5b50\u9009\u62e9",
			L"\u70b9\u51fb\u6253\u5f00\u7a81\u53d8\u56e0\u5b50\u9009\u62e9\u5668");
		UIExt_SetPos(
			button,
			DSurface::ViewBounds.Width - 60 - 40,
			(DSurface::ViewBounds.Height - 48) / 2);
		UIExt_SetAnchor(button, UIExtAnchor_Right, -40, 0);
		UIExt_Button_SetOnClick(button, &StartButtonClick, nullptr);
		UIExt_Open(button, UIExtModal_None);

		s_startButton = button;
	}
}