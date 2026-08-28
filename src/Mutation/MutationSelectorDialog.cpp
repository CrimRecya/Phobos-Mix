#include "MutationSelectorDialog.h"

#include <UI/Builder.h>
#include <UI/UIRoot.h>
#include <Surface.h>

#include <algorithm>

namespace Mutation
{
	std::unique_ptr<UIExt::Dialog> MutationSelectorDialog::Create(MutationViewModel& viewModel)
	{
		constexpr int dialogWidth = 640;
		constexpr int dialogHeight = 480;
		const int dialogX = (DSurface::ViewBounds.Width - dialogWidth) / 2;
		const int dialogY = (DSurface::ViewBounds.Height - dialogHeight) / 2;

		auto dialog = UIExt::Builder::MakeDialog(dialogX, dialogY, dialogWidth, dialogHeight, L"\u7a81\u53d8\u56e0\u5b50\u9009\u62e9");
		dialog->SetBackColor({ 0, 0, 0 }, 60).SetBorder(true);

		auto title = UIExt::Builder::MakeLabel(dialogX + 20, dialogY + 14, L"\u9009\u62e9\u4f60\u60f3\u8981\u7684\u7a81\u53d8\u56e0\u5b50");
		dialog->AddChild(std::move(title));

		auto pageView = UIExt::Builder::MakePageView(dialogX + 20, dialogY + 40, 600, 300);
		pageView->SetGrid(3, 3, 190, 86, 6, 6);

		const auto& selected = viewModel.SelectedIDs.Get();
		const auto& mutations = viewModel.Mutations.GetItems();

		for (const auto& info : mutations)
		{
			auto checkBox = UIExt::Builder::MakeCheckBox(0, 0, 190, 86, info.Name);
			checkBox->SetChecked(std::find(selected.begin(), selected.end(), info.ID) != selected.end());
			checkBox->SetTooltip(info.Name, info.Description);
			checkBox->SetOnToggle([&viewModel, id = info.ID](bool)
			{
				viewModel.ToggleSelect(id);
			});

			pageView->AddChild(std::move(checkBox));
		}

		pageView->Refresh();
		auto* rawPageView = pageView.get();
		dialog->AddChild(std::move(pageView));

		auto prevButton = UIExt::Builder::MakeButton(dialogX + 20, dialogY + 360, 90, 30, L"\u4e0a\u4e00\u9875");
		prevButton->SetOnClick([rawPageView, &viewModel]()
		{
			rawPageView->PrevPage();
			viewModel.PageIndex.Set(rawPageView->GetPageIndex());
		});

		auto nextButton = UIExt::Builder::MakeButton(dialogX + 120, dialogY + 360, 90, 30, L"\u4e0b\u4e00\u9875");
		nextButton->SetOnClick([rawPageView, &viewModel]()
		{
			rawPageView->NextPage();
			viewModel.PageIndex.Set(rawPageView->GetPageIndex());
		});

		auto confirmButton = UIExt::Builder::MakeButton(dialogX + 520, dialogY + 360, 100, 30, viewModel.ConfirmText.Get());
		auto* rawConfirmButton = confirmButton.get();
		confirmButton->BindValue(viewModel.ConfirmText, [rawConfirmButton](const std::wstring& text)
		{
			rawConfirmButton->SetText(text);
		});
		confirmButton->SetOnClick([&viewModel]()
		{
			viewModel.CommitCommand.Execute();
		});

		auto closeButton = UIExt::Builder::MakeButton(dialogX + 410, dialogY + 360, 100, 30, L"\u53d6\u6d88");
		closeButton->SetOnClick([&viewModel]()
		{
			viewModel.CloseCommand.Execute();
		});

		dialog->AddChild(std::move(prevButton));
		dialog->AddChild(std::move(nextButton));
		dialog->AddChild(std::move(confirmButton));
		dialog->AddChild(std::move(closeButton));

		auto* rawDialog = dialog.get();
		viewModel.SetCloseCallback([rawDialog]()
		{
			UIExt::UIRoot::Instance().Close(rawDialog);
		});

		return dialog;
	}
}