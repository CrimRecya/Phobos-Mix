// ============================================================================
// SAMPLE - kept as a runnable UI test.
// ============================================================================
#include "MutationInterop.h"

#include "MutationArt.h"
#include "MutationDisplayerStrip.h"
#include "MutationViewModel.h"

#include <algorithm>
#include <cstdio>

namespace Mutation
{
	MutationInterop::GetAvailableCountFn MutationInterop::s_getAvailableCount = nullptr;
	MutationInterop::GetAvailableNameFn MutationInterop::s_getAvailableName = nullptr;
	MutationInterop::GetAvailableDescriptionFn MutationInterop::s_getAvailableDescription = nullptr;
	MutationInterop::GetAvailableScoreFn MutationInterop::s_getAvailableScore = nullptr;
	MutationInterop::GetAvailableIconFn MutationInterop::s_getAvailableIcon = nullptr;
	MutationInterop::GetActiveCountFn MutationInterop::s_getActiveCount = nullptr;
	MutationInterop::GetActiveIdFn MutationInterop::s_getActiveId = nullptr;
	MutationInterop::ActivateFn MutationInterop::s_activate = nullptr;
	MutationInterop::DeactivateFn MutationInterop::s_deactivate = nullptr;
	MutationInterop::CommitFn MutationInterop::s_commit = nullptr;
	MutationInterop::NotifyChangedFn MutationInterop::s_notifyChanged = nullptr;

	std::vector<int> MutationInterop::s_fallbackActive;

	void MutationInterop::SetProvider(
		GetAvailableCountFn getAvailableCount,
		GetAvailableNameFn getAvailableName,
		GetAvailableDescriptionFn getAvailableDescription,
		GetAvailableScoreFn getAvailableScore,
		GetAvailableIconFn getAvailableIcon,
		GetActiveCountFn getActiveCount,
		GetActiveIdFn getActiveId,
		ActivateFn activate,
		DeactivateFn deactivate,
		CommitFn commit,
		NotifyChangedFn notifyChanged)
	{
		s_getAvailableCount = getAvailableCount;
		s_getAvailableName = getAvailableName;
		s_getAvailableDescription = getAvailableDescription;
		s_getAvailableScore = getAvailableScore;
		s_getAvailableIcon = getAvailableIcon;
		s_getActiveCount = getActiveCount;
		s_getActiveId = getActiveId;
		s_activate = activate;
		s_deactivate = deactivate;
		s_commit = commit;
		s_notifyChanged = notifyChanged;
	}

	void MutationInterop::ClearProvider()
	{
		s_getAvailableCount = nullptr;
		s_getAvailableName = nullptr;
		s_getAvailableDescription = nullptr;
		s_getAvailableScore = nullptr;
		s_getAvailableIcon = nullptr;
		s_getActiveCount = nullptr;
		s_getActiveId = nullptr;
		s_activate = nullptr;
		s_deactivate = nullptr;
		s_commit = nullptr;
		s_notifyChanged = nullptr;
	}

	bool MutationInterop::IsProviderAvailable()
	{
		return s_getAvailableCount && s_getAvailableName && s_getAvailableDescription
			&& s_getAvailableScore && s_getAvailableIcon && s_getActiveCount
			&& s_getActiveId && s_activate && s_deactivate && s_commit;
	}

	std::vector<MutationInfo> MutationInterop::GetAvailableMutations()
	{
		const int count = IsProviderAvailable() ? s_getAvailableCount() : Mutation::GetCustomPcxCount();
		std::vector<MutationInfo> result;
		result.reserve(count < 0 ? 0 : count);

		for (int i = 0; i < count; ++i)
		{
			MutationInfo info;
			info.ID = i;

			if (IsProviderAvailable())
			{
				info.Name = s_getAvailableName(i);
				info.Description = s_getAvailableDescription(i);
				info.Score = s_getAvailableScore(i);
				info.IconIndex = s_getAvailableIcon(i);
			}
			else
			{
				wchar_t buffer[0x40];
				swprintf_s(buffer, L"Mutator %d", i + 1);
				info.Name = buffer;
				info.Description = L"Custom PCX test";
				info.Score = 10;
				info.IconIndex = i;
			}

			result.push_back(std::move(info));
		}

		return result;
	}

	std::vector<int> MutationInterop::GetActiveMutationIds()
	{
		if (!IsProviderAvailable())
			return s_fallbackActive;

		const int count = s_getActiveCount();

		if (count <= 0)
			return { };

		std::vector<int> ids;
		ids.reserve(count);

		for (int i = 0; i < count; ++i)
			ids.push_back(s_getActiveId(i));

		return ids;
	}

	bool MutationInterop::ActivateMutation(int id)
	{
		if (IsProviderAvailable())
			return s_activate(id);

		const auto it = std::find(s_fallbackActive.begin(), s_fallbackActive.end(), id);

		if (it == s_fallbackActive.end())
			s_fallbackActive.push_back(id);

		return true;
	}

	bool MutationInterop::DeactivateMutation(int id)
	{
		if (IsProviderAvailable())
			return s_deactivate(id);

		const auto it = std::find(s_fallbackActive.begin(), s_fallbackActive.end(), id);

		if (it != s_fallbackActive.end())
			s_fallbackActive.erase(it);

		return true;
	}

	bool MutationInterop::CommitSelection(const std::vector<int>& ids)
	{
		if (IsProviderAvailable())
			return s_commit(ids.data(), static_cast<int>(ids.size()));

		s_fallbackActive = ids;
		return true;
	}

	void MutationInterop::NotifyDataChanged()
	{
		// C# side calls this when the mutation model changes.
		// Refresh the C++ ViewModel and any open displayer so the UI follows the model.
		MutationViewModel::Instance().Refresh();
		MutationDisplayerStrip::Refresh(MutationViewModel::Instance());
	}
}