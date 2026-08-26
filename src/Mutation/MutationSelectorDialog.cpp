// ============================================================================
// DISABLED SAMPLE - kept for reference only.
// ============================================================================
#if 0
#include "MutationSelectorDialog.h"

#include "MutationArt.h"

#include <UI/Builder.h>
#include <UI/UIRoot.h>
#include <Surface.h>
#include <FileSystem.h>
#include <ScenarioClass.h>
#include <SideClass.h>

#include <Ext/Side/Body.h>
#include <Windows.h>

#include <algorithm>
#include <cstdio>

namespace Mutation
{
	bool MutationSelectorDialog::s_open = false;

	bool MutationSelectorDialog::IsOpen()
	{
		return s_open;
	}

	void MutationSelectorDialog::SetOpen(bool open)
	{
		s_open = open;
	}

	std::unique_ptr<UIExt::Dialog> MutationSelectorDialog::Create(MutationViewModel& viewModel)
	{
		constexpr int dialogWidth = 640;
		constexpr int dialogHeight = 480;
		const int dialogX = (DSurface::ViewBounds.Width - dialogWidth) / 2;
		const int dialogY = (DSurface::ViewBounds.Height - dialogHeight) / 2;

		auto dialog = UIExt::Builder::MakeDialog(dialogX, dialogY, dialogWidth, dialogHeight, L"\u7a81\u53d8\u56e0\u5b50\u9009\u62e9");
		dialog->SetAnchor(UIExt::Anchor::Center);
		dialog->SetBackColor({ 0, 0, 0 }, 60).SetBorder(true);
		dialog->SetCustomDraw([](UIExt::Panel& panel)
		{
			const auto pSideExt = SideExt::Fetch(SideClass::Array.Items[ScenarioClass::Instance->PlayerSideIndex]);
			auto position = Point2D { panel.X, panel.Y };
			auto surfaceRect = RectangleStruct { 0, 0, panel.X + panel.Width, panel.Y + panel.Height };

			if (const auto pMainSHP = pSideExt->SelectedInfo_Main.Get())
			{
				DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
					pMainSHP, 0, &position, &surfaceRect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
			}

			if (const auto pBottomSHP = pSideExt->SelectedInfo_Bottom.Get())
			{
				if (pBottomSHP->Frames >= 3)
				{
					position = Point2D { panel.X, panel.Y + panel.Height - 30 };
					DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
						pBottomSHP, 1, &position, &surfaceRect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
				}
			}
		});

		auto title = UIExt::Builder::MakeLabel(dialogX + 20, dialogY + 14, L"\u9009\u62e9\u4f60\u60f3\u8981\u7684\u7a81\u53d8\u56e0\u5b50");
		dialog->AddChild(std::move(title));

		auto pageView = UIExt::Builder::MakePageView(dialogX + 20, dialogY + 40, 600, 300);
		pageView->SetGrid(3, 3, 190, 86, 6, 6);

		const auto& selected = viewModel.SelectedIDs.Get();
		const auto& mutations = viewModel.Mutations.GetItems();

		for (const auto& info : mutations)
		{
			auto itemPanel = UIExt::Builder::MakePanel(0, 0, 190, 86);
			itemPanel->SetBackColor({ 0, 0, 0 }, 0).SetBorder(true, COLOR_WHITE);

			BSurface* cameo = Mutation::GetTestCameoSurface(info.IconIndex);
			auto icon = UIExt::Builder::MakeIconButton(8, 12, 60, 60, cameo);
			if (!cameo)
				icon->SetText(std::wstring(1, info.Name.empty() ? L'?' : info.Name[0]));

			auto checkBox = UIExt::Builder::MakeCheckBox(74, 6, 108, 20, info.Name);
			checkBox->SetChecked(std::find(selected.begin(), selected.end(), info.ID) != selected.end());
			checkBox->SetTooltip(info.Name, info.Description);
			checkBox->SetOnToggle([&viewModel, id = info.ID, checkBox = checkBox.get()](bool)
			{
				viewModel.ToggleSelect(id);

				const auto& ids = viewModel.SelectedIDs.Get();
				checkBox->SetChecked(std::find(ids.begin(), ids.end(), id) != ids.end());
			});

			// Keep the checkbox in sync when the selection changes outside the dialog
			// (e.g. right-clicking an icon in the right-side displayer strip).
			checkBox->BindValue(viewModel.SelectedIDs, [id = info.ID, checkBox = checkBox.get()](const std::vector<int>& ids)
			{
				checkBox->SetChecked(std::find(ids.begin(), ids.end(), id) != ids.end());
			});

			// Right-click test: the cameo icon also toggles the mutation.
			icon->SetTooltip(info.Name, info.Description + std::wstring(L"\n\n\u53f3\u952e\u70b9\u51fb\uff1a\u5207\u6362\u9009\u62e9"));
			icon->SetOnRightClick([&viewModel, id = info.ID, checkBox = checkBox.get()]()
			{
				viewModel.ToggleSelect(id);

				const auto& ids = viewModel.SelectedIDs.Get();
				checkBox->SetChecked(std::find(ids.begin(), ids.end(), id) != ids.end());
			});

			auto description = UIExt::Builder::MakeLabel(74, 30, info.Description);
			description->SetSize(108, 50);
			description->SetColor(0xC0C0C0);

			itemPanel->AddChild(std::move(icon));
			itemPanel->AddChild(std::move(checkBox));
			itemPanel->AddChild(std::move(description));
			pageView->AddChild(std::move(itemPanel));
		}

		pageView->Refresh();
		pageView->SetPage(viewModel.PageIndex.Get());
		viewModel.PageIndex.Set(pageView->GetPageIndex());
		auto* rawPageView = pageView.get();
		dialog->AddChild(std::move(pageView));

		viewModel.SetPageCount(rawPageView->GetPageCount());

		// Keep the ViewModel page index and the PageView in sync.
		rawPageView->BindValue(viewModel.PageIndex, [rawPageView](const int& page)
		{
			rawPageView->SetPage(page);
		});
		rawPageView->SetOnPageChanged([&viewModel](int page)
		{
			if (viewModel.PageIndex.Get() != page)
			{
				viewModel.PageIndex.Set(page);
				viewModel.PageNextCommand.NotifyCanExecuteChanged();
				viewModel.PagePrevCommand.NotifyCanExecuteChanged();
			}
		});

		auto pageLabel = UIExt::Builder::MakeLabel(dialogX + 230, dialogY + 360, L"");
		auto* rawPageLabel = pageLabel.get();
		pageLabel->BindValue(viewModel.PageIndex, [rawPageLabel, &viewModel](const int&)
		{
			wchar_t buffer[0x40];
			swprintf_s(buffer, L"\u7b2c %d / %d \u9875", viewModel.PageIndex.Get() + 1, viewModel.PageCount.Get());
			rawPageLabel->SetText(buffer);
		});
		pageLabel->BindValue(viewModel.PageCount, [rawPageLabel, &viewModel](const int&)
		{
			wchar_t buffer[0x40];
			swprintf_s(buffer, L"\u7b2c %d / %d \u9875", viewModel.PageIndex.Get() + 1, viewModel.PageCount.Get());
			rawPageLabel->SetText(buffer);
		});
		pageLabel->SetText(L"\u7b2c 1 / 1 \u9875");
		dialog->AddChild(std::move(pageLabel));

		auto prevButton = UIExt::Builder::MakeButton(dialogX + 20, dialogY + 360, 90, 30, L"\u4e0a\u4e00\u9875");
		prevButton->BindCommand(viewModel.PagePrevCommand);
		dialog->AddChild(std::move(prevButton));

		auto nextButton = UIExt::Builder::MakeButton(dialogX + 120, dialogY + 360, 90, 30, L"\u4e0b\u4e00\u9875");
		nextButton->BindCommand(viewModel.PageNextCommand);
		dialog->AddChild(std::move(nextButton));

		auto confirmButton = UIExt::Builder::MakeButton(dialogX + 520, dialogY + 360, 100, 30, viewModel.ConfirmText.Get());
		confirmButton->BindCommand(viewModel.CommitCommand);
		// BindCommand overwrites OnClick, so set text binding after it.
		confirmButton->BindText(viewModel.ConfirmText);
		confirmButton->SetShortcut(VK_RETURN);
		dialog->AddChild(std::move(confirmButton));

		auto closeButton = UIExt::Builder::MakeButton(dialogX + 410, dialogY + 360, 100, 30, L"\u53d6\u6d88");
		closeButton->BindCommand(viewModel.CloseCommand);
		closeButton->SetShortcut(VK_ESCAPE);
		dialog->AddChild(std::move(closeButton));

		auto* rawDialog = dialog.get();
		viewModel.SetCloseCallback([rawDialog]()
		{
			MutationSelectorDialog::SetOpen(false);
			UIExt::UIRoot::Instance().Close(rawDialog);
		});

		// The Dialog owns a built-in close button; wire it to the same close command.
		dialog->SetCloseAction([&viewModel]()
		{
			viewModel.CloseCommand.Execute();
		});

		return dialog;
	}
}

#endif
