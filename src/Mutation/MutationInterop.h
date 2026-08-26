#pragma once
// ============================================================================
// DISABLED SAMPLE - kept for reference only.
// ============================================================================
#if 0

#include <string>
#include <vector>

namespace Mutation
{
	struct MutationInfo
	{
		int ID { 0 };
		std::wstring Name { };
		std::wstring Description { };
		int Score { 0 };
		int IconIndex { 0 };
	};

	// Thin bridge between Phobos C++ UI and the DynamicPatcher C# mutation model.
	// The C# side can register function pointers at runtime using the existing
	// InteropUtils pattern. When no provider is registered, a small built-in
	// fallback data set keeps the UI testable.
	//
	// The provider ABI is C-compatible on purpose: all strings cross the
	// C++/C# boundary as const wchar_t* pointers.
	class MutationInterop
	{
	public:
		using GetAvailableCountFn = int (*)();
		using GetAvailableNameFn = const wchar_t* (*)(int index);
		using GetAvailableDescriptionFn = const wchar_t* (*)(int index);
		using GetAvailableScoreFn = int (*)(int index);
		using GetAvailableIconFn = int (*)(int index);
		using GetActiveCountFn = int (*)();
		using GetActiveIdFn = int (*)(int index);
		using ActivateFn = bool (*)(int id);
		using DeactivateFn = bool (*)(int id);
		using CommitFn = bool (*)(const int* ids, int count);
		using NotifyChangedFn = void (*)();

		static void SetProvider(
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
			NotifyChangedFn notifyChanged);

		static void ClearProvider();
		static bool IsProviderAvailable();

		static std::vector<MutationInfo> GetAvailableMutations();
		static std::vector<int> GetActiveMutationIds();
		static bool ActivateMutation(int id);
		static bool DeactivateMutation(int id);
		static bool CommitSelection(const std::vector<int>& ids);
		static void NotifyDataChanged();

	private:
		static GetAvailableCountFn s_getAvailableCount;
		static GetAvailableNameFn s_getAvailableName;
		static GetAvailableDescriptionFn s_getAvailableDescription;
		static GetAvailableScoreFn s_getAvailableScore;
		static GetAvailableIconFn s_getAvailableIcon;
		static GetActiveCountFn s_getActiveCount;
		static GetActiveIdFn s_getActiveId;
		static ActivateFn s_activate;
		static DeactivateFn s_deactivate;
		static CommitFn s_commit;
		static NotifyChangedFn s_notifyChanged;

		static std::vector<int> s_fallbackActive;
	};
}
#endif
