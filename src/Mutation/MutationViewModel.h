#pragma once
// ============================================================================
// DISABLED SAMPLE - kept for reference only.
// ============================================================================
#if 0

#include <UI/Mvvm.h>

#include "MutationInterop.h"

#include <functional>

namespace Mutation
{
	class MutationViewModel
	{
	public:
		static MutationViewModel& Instance();

		void Refresh();
		void SetPageCount(int count);
		void ToggleSelect(int id);
		void Commit();
		void Close();
		void SetCloseCallback(std::function<void()> callback);

		UIExt::ObservableVector<MutationInfo> Mutations;
		UIExt::Observable<int> PageIndex;
		UIExt::Observable<int> PageCount;
		UIExt::Observable<int> MaxSelection;
		UIExt::Observable<std::vector<int>> SelectedIDs;
		UIExt::Observable<std::wstring> ConfirmText;

		UIExt::Command ToggleSelectCommand;
		UIExt::Command PageNextCommand;
		UIExt::Command PagePrevCommand;
		UIExt::Command CommitCommand;
		UIExt::Command CloseCommand;

	private:
		MutationViewModel();

		void UpdateConfirmText();

		std::function<void()> CloseCallback_ { };
	};
}
#endif
