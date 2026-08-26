// ============================================================================
// SAMPLE - kept as a runnable UI test.
// ============================================================================
#include "MutationViewModel.h"

#include "MutationDisplayerStrip.h"

#include <algorithm>
#include <cstdio>

namespace Mutation
{
	MutationViewModel& MutationViewModel::Instance()
	{
		static MutationViewModel instance;
		return instance;
	}

	MutationViewModel::MutationViewModel()
	{
		this->ToggleSelectCommand.SetExecute([] { });
		this->PageNextCommand.SetExecute([this]
		{
			if (this->PageIndex.Get() + 1 < this->PageCount.Get())
			{
				this->PageIndex.Set(this->PageIndex.Get() + 1);
				this->PageNextCommand.NotifyCanExecuteChanged();
				this->PagePrevCommand.NotifyCanExecuteChanged();
			}
		});
		this->PageNextCommand.SetCanExecute([this]
		{
			return this->PageIndex.Get() + 1 < this->PageCount.Get();
		});
		this->PagePrevCommand.SetExecute([this]
		{
			if (this->PageIndex.Get() > 0)
			{
				this->PageIndex.Set(this->PageIndex.Get() - 1);
				this->PageNextCommand.NotifyCanExecuteChanged();
				this->PagePrevCommand.NotifyCanExecuteChanged();
			}
		});
		this->PagePrevCommand.SetCanExecute([this]
		{
			return this->PageIndex.Get() > 0;
		});
		this->CommitCommand.SetExecute([this] { this->Commit(); });
		this->CommitCommand.SetCanExecute([this]
		{
			const auto max = this->MaxSelection.Get();
			const auto count = static_cast<int>(this->SelectedIDs.Get().size());
			return count > 0 && (max <= 0 || count <= max);
		});
		this->CloseCommand.SetExecute([this] { this->Close(); });
	}

	void MutationViewModel::Refresh()
	{
		this->Mutations.SetItems(MutationInterop::GetAvailableMutations());
		this->SelectedIDs.Set(MutationInterop::GetActiveMutationIds());
		this->UpdateConfirmText();
		this->CommitCommand.NotifyCanExecuteChanged();
	}

	void MutationViewModel::SetPageCount(int count)
	{
		this->PageCount.Set(count);
		this->PageNextCommand.NotifyCanExecuteChanged();
		this->PagePrevCommand.NotifyCanExecuteChanged();
	}

	void MutationViewModel::ToggleSelect(int id)
	{
		auto ids = this->SelectedIDs.Get();
		const auto it = std::find(ids.begin(), ids.end(), id);

		if (it == ids.end())
		{
			const auto max = this->MaxSelection.Get();
			if (max > 0 && static_cast<int>(ids.size()) >= max)
				return;

			ids.push_back(id);
		}
		else
		{
			ids.erase(it);
		}

		this->SelectedIDs.Set(std::move(ids));
		this->UpdateConfirmText();
		this->CommitCommand.NotifyCanExecuteChanged();
	}

	void MutationViewModel::Commit()
	{
		MutationInterop::CommitSelection(this->SelectedIDs.Get());
		this->Refresh();
		MutationDisplayerStrip::Refresh(*this);
		this->Close();
	}

	void MutationViewModel::Close()
	{
		// Closing without committing must roll back any local selection changes,
		// so the displayer reflects the last committed state again.
		this->SelectedIDs.Set(MutationInterop::GetActiveMutationIds());
		this->UpdateConfirmText();
		this->CommitCommand.NotifyCanExecuteChanged();
		MutationDisplayerStrip::Refresh(*this);

		if (this->CloseCallback_)
			this->CloseCallback_();
	}

	void MutationViewModel::SetCloseCallback(std::function<void()> callback)
	{
		this->CloseCallback_ = std::move(callback);
	}

	void MutationViewModel::UpdateConfirmText()
	{
		const auto& ids = this->SelectedIDs.Get();
		wchar_t buffer[0x40];
		swprintf_s(buffer, L"\u786e\u5b9a (%d)", static_cast<int>(ids.size()));
		this->ConfirmText.Set(std::wstring(buffer));
	}
}