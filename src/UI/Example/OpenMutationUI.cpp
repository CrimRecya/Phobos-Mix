// ============================================================================
// DISABLED EXAMPLE - kept for reference only.
// ============================================================================
#if 0
// ============================================================================
// SAMPLE - kept as a runnable UI test.
// ============================================================================
#include "OpenMutationUI.h"

#include "MutationDisplayerStrip.h"
#include "MutationSelectorDialog.h"
#include "MutationViewModel.h"

#include <UI/UIRoot.h>

namespace Mutation
{
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
		if (MutationSelectorDialog::IsOpen())
			return;

		auto& viewModel = MutationViewModel::Instance();
		viewModel.Refresh();

		auto dialog = MutationSelectorDialog::Create(viewModel);
		UIExt::UIRoot::Instance().Open(std::move(dialog), UIExt::ModalLevel::BlockTactical);
		MutationSelectorDialog::SetOpen(true);

		MutationDisplayerStrip::Open(viewModel);
	}
}
#endif
