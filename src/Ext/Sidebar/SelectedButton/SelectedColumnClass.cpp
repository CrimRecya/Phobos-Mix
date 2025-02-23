#include "SelectedColumnClass.h"
#include "SelectedInfoClass.h"

#include <GameOptionsClass.h>
#include <FPSCounter.h>

#include <Ext/Side/Body.h>

SelectedColumnClass::SelectedColumnClass(unsigned int id, int x, int y, int width, int height)
	: ControlClass(id, x, y, width, height, static_cast<GadgetFlag>(0), false)
{
	this->Disabled = !Phobos::Config::SelectedDisplay_Enable || !SelectedInfoClass::Instance.SingleSelect;
}

bool SelectedColumnClass::Draw(bool forced)
{
	if (!ScenarioClass::Instance->UserInputLocked && Phobos::Config::SelectedDisplay_Enable)
		SelectedInfoClass::Instance.DrawInfo();

	return true;
}

void SelectedColumnClass::OnMouseEnter()
{
	SelectedInfoClass::Instance.IsHovering = true;
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}

void SelectedColumnClass::OnMouseLeave()
{
	SelectedInfoClass::Instance.IsHovering = false;
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}

void SelectedColumnClass::DrawInfo() const
{
	const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array->Items[ScenarioClass::Instance->PlayerSideIndex]);
	auto position = Point2D { this->X, this->Y };
	auto surfaceRect = RectangleStruct { 0, 0, this->X + this->Width, this->Y + this->Height };

	if (const auto pMainSHP = pSideExt->SelectedInfo_Main.Get())
	{
		DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
			pMainSHP, 0, &position, &surfaceRect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}

	const auto pThis = abstract_cast<TechnoClass*>(ObjectClass::CurrentObjects->Items[0]);

	if (!pThis)
		return;

	const auto pType = pThis->GetTechnoType();
	const auto pExt = TechnoExt::ExtMap.Find(pThis);
	const bool canDisplayAll = pThis->Owner && pThis->Owner->IsAlliedWith(HouseClass::CurrentPlayer()) || HouseClass::IsCurrentPlayerObserver();

	auto getDisplayType = [&]() -> ObjectTypeClass*
	{
		if (!canDisplayAll)
		{
			if (pThis->IsDisguisedAs(HouseClass::CurrentPlayer()))
			{
				const auto pDisguiseType = TechnoTypeExt::GetTechnoType(pThis->Disguise);

				if (const auto pDisguiseTypeExt = TechnoTypeExt::ExtMap.Find(pDisguiseType))
				{
					if (const auto pFakeType = pDisguiseTypeExt->FakeOf.Get())
						return pFakeType;
				}

				return pThis->Disguise;
			}

			if (const auto pFakeType = pExt->TypeExtData->FakeOf.Get())
				return pFakeType;
		}

		return pType;
	};

	const auto pDisplayType = getDisplayType();
	const auto pDisplayTypeExt = TechnoTypeExt::ExtMap.Find(TechnoTypeExt::GetTechnoType(pDisplayType));

	TextPrintType printType = TextPrintType::Center | TextPrintType::Point8;
	const COLORREF tooltipColor = Drawing::RGB_To_Int(Drawing::TooltipColor);
	position += Point2D { 128, 5 };

	if (const auto name = (pDisplayTypeExt && !pDisplayTypeExt->EnemyUIName.Get().empty()) ? pDisplayTypeExt->EnemyUIName.Get().Text : pDisplayType->UIName)
	{
		size_t length = Math::min(wcslen(name), static_cast<size_t>(31));

		for (auto check = name; *check != L'\0'; ++check)
		{
			if (*check == L'\n')
			{
				length = static_cast<size_t>(check - name);
				break;
			}
		}

		wchar_t text[0x20] = {0};
		wcsncpy_s(text, name, length);
		text[length] = L'\0';
		DSurface::Composite->DrawTextA(text, &surfaceRect, &position, tooltipColor, 0, printType);
	}

	position.Y += 18;
	{
		auto color = tooltipColor;
		int value = -1, maxValue = 0;

		const auto infoType = pDisplayTypeExt ? pDisplayTypeExt->SelectedInfo_UpperType.Get() : DisplayInfoType::Shield;
		SelectedInfoClass::GetValuesForDisplay(pThis, pDisplayType, infoType, value, maxValue);

		const bool valid = value >= 0 && maxValue > 0;
		const auto ratio = valid ? static_cast<double>(value) / maxValue : 1.0;

		if (!pDisplayTypeExt || pDisplayTypeExt->SelectedInfo_UpperColor.Get() == ColorStruct{ 0, 0, 0 })
			color = (ratio > RulesClass::Instance->ConditionYellow) ? 0x67EC : (ratio > RulesClass::Instance->ConditionRed ? 0xFFEC : 0xF986);
		else
			color = Drawing::RGB_To_Int(pDisplayTypeExt->SelectedInfo_UpperColor.Get());

		wchar_t text[0x20] = {0};

		position.X += 6;
		printType &= ~TextPrintType::Center;

		if (valid)
			swprintf_s(text, L"%d", maxValue);
		else
			swprintf_s(text, L"--");

		DSurface::Composite->DrawTextA(text, &surfaceRect, &position, color, 0, printType);

		position.X -= 12;
		printType |= TextPrintType::Right;

		if (valid)
			swprintf_s(text, L"%d", value);
		else
			swprintf_s(text, L"--");

		DSurface::Composite->DrawTextA(text, &surfaceRect, &position, color, 0, printType);

		position.X += 6;
		printType &= ~TextPrintType::Right;
		printType |= TextPrintType::Center;
		DSurface::Composite->DrawTextA(L"/", &surfaceRect, &position, color, 0, printType);
	}

	position.Y += 14;
	{
		auto color = tooltipColor;
		int value = -1, maxValue = 0;

		const auto infoType = pDisplayTypeExt ? pDisplayTypeExt->SelectedInfo_BelowType.Get() : DisplayInfoType::Health;
		SelectedInfoClass::GetValuesForDisplay(pThis, pDisplayType, infoType, value, maxValue);

		const bool valid = value >= 0 && maxValue > 0;
		const auto ratio = valid ? static_cast<double>(value) / maxValue : 1.0;

		if (!pDisplayTypeExt || pDisplayTypeExt->SelectedInfo_BelowColor.Get() == ColorStruct{ 0, 0, 0 })
			color = (ratio > RulesClass::Instance->ConditionYellow) ? 0x67EC : (ratio > RulesClass::Instance->ConditionRed ? 0xFFEC : 0xF986);
		else
			color = Drawing::RGB_To_Int(pDisplayTypeExt->SelectedInfo_BelowColor.Get());

		wchar_t text[0x20] = {0};

		position.X += 6;
		printType &= ~TextPrintType::Center;

		if (valid)
			swprintf_s(text, L"%d", maxValue);
		else
			swprintf_s(text, L"--");

		DSurface::Composite->DrawTextA(text, &surfaceRect, &position, color, 0, printType);

		position.X -= 12;
		printType |= TextPrintType::Right;

		if (valid)
			swprintf_s(text, L"%d", value);
		else
			swprintf_s(text, L"--");

		DSurface::Composite->DrawTextA(text, &surfaceRect, &position, color, 0, printType);

		position.X += 6;
		printType &= ~TextPrintType::Right;
		printType |= TextPrintType::Center;
		DSurface::Composite->DrawTextA(L"/", &surfaceRect, &position, color, 0, printType);
	}

	const auto pMainCameo = SelectedInfoClass::Instance.MainCameo;

	if (!pMainCameo)
		return;

	auto drawRect = RectangleStruct { pMainCameo->X, pMainCameo->Y, pMainCameo->Width, pMainCameo->Height};

	if (const auto pCameoPCX = pDisplayTypeExt ? pDisplayTypeExt->CameoPCX.GetSurface() : nullptr)
	{
		PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, pCameoPCX);
	}
	else if (const auto pSHP = pDisplayType->GetCameo())
	{
		if (const auto MissingCameoPCX = SelectedInfoClass::SearchMissingCameo(pDisplayType->WhatAmI(), pSHP))
		{
			PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
		}
		else
		{
			surfaceRect = RectangleStruct { 0, 0, pMainCameo->X + pMainCameo->Width, pMainCameo->Y + pMainCameo->Height};
			const auto pPal = pDisplayTypeExt ? pDisplayTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL) : FileSystem::CAMEO_PAL();
			DSurface::Composite->DrawSHP(pPal, pSHP, 0, &position, &surfaceRect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}
	}

	// TODO status

	if (canDisplayAll)
	{
		if (pThis->IsIronCurtained())
		{
			ColorStruct fillColor { 50, 50, 50 };
			DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20);
		}
		else
		{
			const auto pRules = RulesClass::Instance();
			const auto ratio = pThis->GetHealthPercentage();
			int time = Unsorted::CurrentFrame - pExt->LastHurtFrame;

			if (ratio < pRules->ConditionRed)
			{
				ColorStruct fillColor { 255, 0, 0 };
				int trans = 40 - time;

				if (trans < 0)
				{
					const int round = time % 60;
					trans = ((round <= 20) ? 0 : ((round <= 40) ? (round - 20) : (60 - round)));
				}

				if (trans > 0)
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, trans);
			}
			else if (ratio < pRules->ConditionYellow)
			{
				ColorStruct fillColor { 255, 0, 0 };
				int trans = 30 - time;

				if (trans < 0)
				{
					const int round = time % 160;
					trans = ((round <= 140) ? 0 : ((round <= 150) ? (round - 140) : (160 - round)));
				}

				if (trans > 0)
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, trans);
			}
			else if (time < 20)
			{
				ColorStruct fillColor { 255, 0, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, (20 - time));
			}

			time = Unsorted::CurrentFrame - pThis->LastFireBulletFrame;

			if (time < 20)
			{
				ColorStruct fillColor { 255, 255, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20 - time);
			}

			if (pThis->TemporalTargetingMe)
			{
				ColorStruct fillColor { 100, 100, 255 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pThis->AirstrikeTintStage)
			{
				ColorStruct fillColor { 255, 50, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pThis->DrainingMe || pThis->LocomotorSource)
			{
				ColorStruct fillColor { 200, 0, 255 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pThis->IsUnderEMP())
			{
				ColorStruct fillColor { 128, 128, 128 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
		}
	}

	if (pMainCameo->Hovering && pDisplayTypeExt && Phobos::Config::ToolTipDescriptions)
	{
		if (!pDisplayTypeExt->UIDescription.Get().empty())
		{
			// const auto description = pDisplayTypeExt->UIDescription.Get().Text;
			// TODO Draw text
		}
	}
}

// ----------------------------------------

SelectedBottomClass::SelectedBottomClass(unsigned int id, int x, int y, int width, int height)
	: ControlClass(id, x, y, width, height, static_cast<GadgetFlag>(0), false)
{
	this->Disabled = !Phobos::Config::SelectedDisplay_Enable || !SelectedInfoClass::Instance.SingleSelect;
}

bool SelectedBottomClass::Draw(bool forced)
{
	return false;
}

void SelectedBottomClass::OnMouseEnter()
{
	SelectedInfoClass::Instance.IsHovering = true;
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}

void SelectedBottomClass::OnMouseLeave()
{
	SelectedInfoClass::Instance.IsHovering = false;
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}

void SelectedBottomClass::DrawInfo() const
{
	const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array->Items[ScenarioClass::Instance->PlayerSideIndex]);

	if (const auto pSHP = pSideExt->SelectedInfo_Bottom.Get())
	{
		const auto position = Point2D { this->X, this->Y };
		const auto rect = RectangleStruct { 0, 0, this->X + this->Width, this->Y + this->Height };
		DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
			pSHP, 0, &position, &rect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}

	const auto fps = FPSCounter::CurrentFrameRate();
	const auto gameSpeed = GameOptionsClass::Instance->GameSpeed;
	COLORREF color = 0x67EC;

	if (!gameSpeed || fps < static_cast<unsigned int>(60 / gameSpeed))
	{
		if (fps < 10)
			color = 0xF986;
		else if (fps < 20)
			color = 0xFC05;
		else if (fps < 30)
			color = 0xFCE5;
		else if (fps < 45)
			color = 0xFFEC;
		else if (fps < 60)
			color = 0x9FEC;
	}

	auto location = Point2D { this->X + 12, this->Y + 3 };
	wchar_t fpsBuffer[0x20];
	swprintf_s(fpsBuffer, L"FPS: %u", fps);
	DSurface::Composite->DrawText(fpsBuffer, &location, color);

	location.X += 77;
	wchar_t avgBuffer[0x20];
	swprintf_s(avgBuffer, L"Avg: %.2lf", FPSCounter::GetAverageFrameRate());
	DSurface::Composite->DrawText(avgBuffer, &location, COLOR_WHITE);

	location.X += 115;
	// TODO time
}
