#pragma once
// ============================================================================
// DISABLED EXAMPLE - kept for reference only.
// ============================================================================
#if 0
// ============================================================================
// SAMPLE - kept as a runnable UI test.
// ============================================================================

#include <UI/Controls/Dialog.h>

#include "MutationViewModel.h"

#include <memory>

namespace Mutation
{
	class MutationSelectorDialog
	{
	public:
		static std::unique_ptr<UIExt::Dialog> Create(MutationViewModel& viewModel);
		static bool IsOpen();
		static void SetOpen(bool open);

	private:
		static bool s_open;
	};
}
#endif
