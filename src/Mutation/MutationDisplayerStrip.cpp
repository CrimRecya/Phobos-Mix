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

		auto strip = UIExt::Builder::MakeIconStrip(DSurface::ViewBounds.Width - 80, 40, 60, 60, 4);
		strip->SetAnchor(UIExt::Anchor::Right);
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

		auto& children = s_strip->GetChildren();

		while (!children.empty())
		{
			auto* child = children.front().get();
			UIExt::UIRoot::Instance().RemoveRootChild(s_strip, child);
		}

		const auto& mutations = viewModel.Mutations.GetItems();
		const auto& selected = viewModel.SelectedIDs.Get();

		for (const auto id : selected)
		{
			const auto it = std::find_if(mutations.begin(), mutations.end(),
				[id](const MutationInfo& info) { return info.ID == id; });

			if (it == mutations.end())
				continue;

			auto icon = UIExt::Builder::MakeIconButton(0, 0, 60, 60, nullptr);

			if (auto* cameo = GetTestCameoSurface(it->IconIndex))
				icon->SetIcon(cameo);
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