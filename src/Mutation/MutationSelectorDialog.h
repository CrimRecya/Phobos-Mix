#pragma once

#include <UI/Controls/Dialog.h>

#include "MutationViewModel.h"

#include <memory>

namespace Mutation
{
	class MutationSelectorDialog
	{
	public:
		static std::unique_ptr<UIExt::Dialog> Create(MutationViewModel& viewModel);
	};
}