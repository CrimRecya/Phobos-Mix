#pragma once
// ============================================================================
// DISABLED SAMPLE - kept for reference only.
// ============================================================================
#if 0

#include <UI/Controls/IconStrip.h>

#include "MutationViewModel.h"

namespace Mutation
{
	// Shows the currently selected mutations as an always-visible icon strip.
	class MutationDisplayerStrip
	{
	public:
		static void Open(MutationViewModel& viewModel);
		static void Refresh(MutationViewModel& viewModel);
		static void Close();

	private:
		static UIExt::IconStrip* s_strip;
	};
}
#endif
