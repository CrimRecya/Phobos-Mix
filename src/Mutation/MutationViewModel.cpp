#include "MutationViewModel.h"

#include "MutationDisplayerStrip.h"

#include <algorithm>

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
		this->CommitCommand.SetExecute([this] { this->Commit(); });
		this->CloseCommand.SetExecute([this] { this->Close(); });
	}

	void MutationViewModel::Refresh()
	{
		this->Mutations.SetItems(MutationInterop::GetAvailableMutations());
		this->SelectedIDs.Set(MutationInterop::GetActiveMutationIds());
		this->UpdateConfirmText();
	}

	void MutationViewModel::ToggleSelect(int id)
	{
		auto ids = this->SelectedIDs.Get();
		const auto it = std::find(ids.begin(), ids.end(), id);

		if (it == ids.end())
			ids.push_back(id);
		else
			ids.erase(it);

		this->SelectedIDs.Set(std::move(ids));
		this->UpdateConfirmText();
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
		swprintf_s(buffer, L"确认 (%d)", static_cast<int>(ids.size()));
		this->ConfirmText.Set(std::wstring(buffer));
	}
}