// ============================================================================
// SAMPLE - kept as a runnable UI test.
// ============================================================================
#include "MutationDisplayerStrip.h"

#include "MutationArt.h"

#include <UI/Builder.h>
#include <UI/UIRoot.h>
#include <Surface.h>

#include <algorithm>

namespace Mutation
{
	UIExt::IconStrip* MutationDisplayerStrip::s_strip = nullptr;

	void MutationDisplayerStrip::Open(MutationViewModel& viewModel)
	{
		if (s_strip)
		{
			Refresh(viewModel);
			return;
		}

		auto strip = UIExt::Builder::MakeIconStrip(
			DSurface::ViewBounds.Width - 108,
			(DSurface::ViewBounds.Height - 48) / 2,
			60, 48, 4);
		strip->SetAnchor(UIExt::Anchor::Right, -48, 0);
		s_strip = strip.get();
		UIExt::UIRoot::Instance().Open(std::move(strip), UIExt::ModalLevel::None);

		// Build the initial content before attaching bindings, so the strip has its
		// final height before the first anchor/layout pass (avoids a one-frame jump).
		Refresh(viewModel);

		// Keep the displayer in sync when the selection or the mutation list changes.
		s_strip->BindValue(viewModel.SelectedIDs, [&viewModel](const std::vector<int>&)
		{
			MutationDisplayerStrip::Refresh(viewModel);
		});
		viewModel.Mutations.SetOnChanged([&viewModel]
		{
			MutationDisplayerStrip::Refresh(viewModel);
		});
	}

	void MutationDisplayerStrip::Refresh(MutationViewModel& viewModel)
	{
		if (!s_strip)
			return;

		// Fast path: only rebuild when the selected set or the mutation list
		// actually changes. This mirrors DP's RefreshActiveStrip() behavior.
		static std::vector<int> s_lastSelected;
		static size_t s_lastMutationsRevision = static_cast<size_t>(-1);

		const auto& selected = viewModel.SelectedIDs.Get();
		const auto& mutations = viewModel.Mutations.GetItems();
		const auto mutationsRevision = viewModel.Mutations.GetRevision();

		if (s_lastSelected == selected && s_lastMutationsRevision == mutationsRevision)
			return;

		s_lastSelected = selected;
		s_lastMutationsRevision = mutationsRevision;

		auto& children = s_strip->GetChildren();

		while (!children.empty())
		{
			auto* child = children.front().get();
			UIExt::UIRoot::Instance().RemoveRootChild(s_strip, child);
		}

		for (const auto id : selected)
		{
			const auto it = std::find_if(mutations.begin(), mutations.end(),
				[id](const MutationInfo& info) { return info.ID == id; });

			if (it == mutations.end())
				continue;

			auto icon = UIExt::Builder::MakeIconButton(0, 0, 60, 48);

			if (const char* cameoFile = GetTestCameoFile(it->IconIndex))
				icon->SetIconFile(cameoFile);
			else
				icon->SetText(std::wstring(1, it->Name.empty() ? L'?' : it->Name[0]));

			// Right-click test: remove this mutation directly from the strip.
			icon->SetTooltip(it->Name, it->Description + std::wstring(L"\n\n\u53f3\u952e\u70b9\u51fb\uff1a\u79fb\u9664"));
			icon->SetOnRightClick([&viewModel, id = it->ID]()
			{
				viewModel.ToggleSelect(id);
				MutationInterop::CommitSelection(viewModel.SelectedIDs.Get());
			});

			UIExt::UIRoot::Instance().AddRootChild(s_strip, std::move(icon));
		}

		s_strip->Refresh();
	}

	void MutationDisplayerStrip::Close()
	{
		if (!s_strip)
			return;

		UIExt::UIRoot::Instance().Close(s_strip);
		s_strip = nullptr;
	}
}