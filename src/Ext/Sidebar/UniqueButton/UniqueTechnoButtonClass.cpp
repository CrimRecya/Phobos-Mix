#include "UniqueTechnoButtonClass.h"
#include "UniqueTechnoColumnClass.h"

#include <FactoryClass.h>
#include <TacticalClass.h>

#include <Ext/Scenario/Body.h>

UniqueTechnoButtonClass::UniqueTechnoButtonClass(unsigned int id, int x, int y)
	: ControlClass(id, x, y, 60, 48, GadgetFlag::LeftPress, false)
{
	this->Disabled = !UniqueTechnoColumnClass::Instance.Visible;
}

bool UniqueTechnoButtonClass::Draw(bool forced)
{
	if (!UniqueTechnoColumnClass::Instance.Visible)
		return false;

	auto& vec = ScenarioExt::Global()->OwnedUniqueTechnos;

	if (vec.empty())
		return false;

	const int index = this->ID - UniqueTechnoColumnClass::StartID;

	if (index >= static_cast<int>(vec.size()))
		return false;

	const auto pExt = vec[index];
	const auto pTechno = pExt->OwnerObject();
	const auto pTypeExt = pExt->TypeExtData;
	const auto pType = pTypeExt->OwnerObject();

	Point2D position { this->X, this->Y };
	RectangleStruct drawRect { this->X, this->Y, 60, 48 };

	if (const auto CameoPCX = pTypeExt->CameoPCX.GetSurface())
	{
		PCX::Instance.BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
	}
	else if (const auto pSHP = pType->GetCameo())
	{
		auto getMissingCameo = [pSHP]() -> BSurface*
		{
			const auto pCameoRef = pSHP->AsReference();
			char pFilename[0x20];
			strcpy_s(pFilename, RulesExt::Global()->MissingCameo.data());
			_strlwr_s(pFilename);

			if (!_stricmp(pCameoRef->Filename, GameStrings::XXICON_SHP) && strstr(pFilename, ".pcx"))
			{
				PCX::Instance.LoadFile(pFilename);

				if (const auto MissingCameoPCX = PCX::Instance.GetSurface(pFilename))
					return MissingCameoPCX;
			}

			return nullptr;
		};

		if (const auto MissingCameoPCX = getMissingCameo())
		{
			PCX::Instance.BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
		}
		else
		{
			RectangleStruct rect { 0, 0, position.X + 60, position.Y + 48 };
			DSurface::Composite->DrawSHP(pTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
				BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}
	}

	if (!pTechno->InLimbo)
	{
		const auto pRules = RulesClass::Instance;
		auto ratio = pTechno->GetHealthPercentage();

		if (pTechno->IsIronCurtained())
		{
			ColorStruct fillColor { 50, 50, 50 };
			DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20);
		}
		else
		{
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

			time = Unsorted::CurrentFrame - pTechno->LastFireBulletFrame;

			if (time < 20)
			{
				ColorStruct fillColor { 255, 255, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20 - time);
			}

			if (pTechno->TemporalTargetingMe)
			{
				ColorStruct fillColor { 100, 100, 255 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pTechno->AirstrikeTintStage)
			{
				ColorStruct fillColor { 255, 50, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pTechno->DrainingMe || pTechno->LocomotorSource)
			{
				ColorStruct fillColor { 200, 0, 255 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pTechno->IsUnderEMP() || pTechno->Deactivated)
			{
				ColorStruct fillColor { 128, 128, 128 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
		}

		if (pTechno->BunkerLinkedItem && pTechno->WhatAmI() != AbstractType::Building)
		{
			RectangleStruct rect { (position.X + 3), (position.Y + 1), 54, 7 };
			DSurface::Composite->DrawRect(&rect, 0x781F);
		}

		RectangleStruct rect { (position.X + 4), (position.Y + 2), 52, 5 };
		DSurface::Composite->FillRect(&rect, 0);

		++rect.X;
		++rect.Y;
		rect.Width = static_cast<int>(50 * ratio + 0.5);
		rect.Height = 3;

		const int color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
		DSurface::Composite->FillRect(&rect, color);

		const auto pShield = pExt->Shield.get();

		if (pShield && !pShield->IsBrokenAndNonRespawning())
		{
			ratio = (static_cast<double>(pShield->GetHP()) / pShield->GetType()->Strength.Get());
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			ColorStruct fillColor { 153, 153, 255 };
			DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
		}

		if (pTechno->IsIronCurtained())
		{
			const auto& timer = pTechno->IronCurtainTimer;
			ratio = static_cast<double>(timer.GetTimeLeft()) / timer.TimeLeft;
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			ColorStruct fillColor { 200, 50, 50 };
			DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
		}
	}
	else if (auto pSelect = pTechno->Transporter)
	{
		for (auto pTrans = pSelect; pTrans; pTrans = pTrans->Transporter)
			pSelect = pTrans;

		const auto pSelectExt = TechnoExt::ExtMap.Find(pSelect);
		const auto pRules = RulesClass::Instance;
		auto ratio = pTechno->GetHealthPercentage();

		if (pSelect->IsIronCurtained())
		{
			ColorStruct fillColor { 50, 50, 50 };
			DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20);
		}
		else
		{
			int time = Unsorted::CurrentFrame - pSelectExt->LastHurtFrame;

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

			time = Unsorted::CurrentFrame - pSelect->LastFireBulletFrame;

			if (time < 20)
			{
				ColorStruct fillColor { 255, 255, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20 - time);
			}

			if (pSelect->TemporalTargetingMe)
			{
				ColorStruct fillColor { 100, 100, 255 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pSelect->LocomotorSource)
			{
				ColorStruct fillColor { 200, 0, 255 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
			else if (pSelect->IsUnderEMP() || pSelect->Deactivated)
			{
				ColorStruct fillColor { 128, 128, 128 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
			}
		}

		RectangleStruct rect { (position.X + 3), (position.Y + 1), 0, 7 };

		if (pSelect->BunkerLinkedItem && pSelect->WhatAmI() != AbstractType::Building)
		{
			rect.Width = 54;
			DSurface::Composite->DrawRect(&rect, 0x781F);
		}
		else
		{
			rect.Width = static_cast<int>(54 * pSelect->GetHealthPercentage() + 0.5);
			DSurface::Composite->DrawRect(&rect, 0xFB20);
		}

		++rect.X;
		++rect.Y;
		rect.Width = 52;
		rect.Height = 5;

		DSurface::Composite->FillRect(&rect, 0);

		++rect.X;
		++rect.Y;
		rect.Width = static_cast<int>(50 * ratio + 0.5);
		rect.Height = 3;

		const int color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
		DSurface::Composite->FillRect(&rect, color);

		const auto pShield = pSelectExt->Shield.get();

		if (pShield && !pShield->IsBrokenAndNonRespawning())
		{
			ratio = (static_cast<double>(pShield->GetHP()) / pShield->GetType()->Strength.Get());
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			ColorStruct fillColor { 153, 153, 255 };
			DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
		}

		if (pSelect->IsIronCurtained())
		{
			const auto& timer = pTechno->IronCurtainTimer;
			ratio = static_cast<double>(timer.GetTimeLeft()) / timer.TimeLeft;
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			ColorStruct fillColor { 200, 50, 50 };
			DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
		}
	}
	else
	{
		const auto absType = pTechno->WhatAmI();
		const auto buildCat = (absType == AbstractType::Building) ? static_cast<BuildingClass*>(pTechno)->Type->BuildCat : BuildCat::DontCare;
		const auto pFactory = pTechno->Owner->GetPrimaryFactory(absType, pType->Naval, buildCat);

		if (pFactory && pFactory->Object == pTechno)
		{
			ColorStruct fillColor { 0, 0, 0 };
			DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 30);

			RectangleStruct rect { (position.X + 4), (position.Y + 2), 52, 5 };
			DSurface::Composite->FillRect(&rect, 0);

			const auto ratio = static_cast<double>(pFactory->GetProgress()) / 54;
			rect = RectangleStruct { (position.X + 5), (position.Y + 3), static_cast<int>(50 * ratio), 3 };
			DSurface::Composite->FillRect(&rect, 0xFFFF);
		}
		else
		{
			RectangleStruct rect { (position.X + 3), (position.Y + 1), 54, 7 };
			DSurface::Composite->DrawRect(&rect, 0x781F);

			++rect.X;
			++rect.Y;
			rect.Width = 52;
			rect.Height = 5;

			DSurface::Composite->FillRect(&rect, 0);

			const auto ratio = pTechno->GetHealthPercentage();

			++rect.X;
			++rect.Y;
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			rect.Height = 3;

			const auto pRules = RulesClass::Instance;
			const int color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
			DSurface::Composite->FillRect(&rect, color);
		}
	}

	if (this->Hovering)
	{
		RectangleStruct rect { 0, 0, position.X + 60, position.Y + 48 };
		DSurface::Composite->DrawRectEx(&rect, &drawRect, Drawing::RGB_To_Int(Drawing::TooltipColor));
	}

	return true;
}

void UniqueTechnoButtonClass::OnMouseEnter()
{
	if (!UniqueTechnoColumnClass::Instance.Visible)
		return;

	auto& vec = ScenarioExt::Global()->OwnedUniqueTechnos;
	const int index = this->ID - UniqueTechnoColumnClass::StartID;

	if (index >= static_cast<int>(vec.size()))
		return;

	this->Hovering = true;
	UniqueTechnoColumnClass::Instance.Hovering = index;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

void UniqueTechnoButtonClass::OnMouseLeave()
{
	this->Hovering = false;
	UniqueTechnoColumnClass::Instance.Hovering = -1;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

bool UniqueTechnoButtonClass::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
{
	if (!UniqueTechnoColumnClass::Instance.Visible || !(flags & GadgetFlag::LeftPress))
		return false;

	auto& vec = ScenarioExt::Global()->OwnedUniqueTechnos;
	const int index = this->ID - UniqueTechnoColumnClass::StartID;

	if (index >= static_cast<int>(vec.size()))
		return false;

	VocClass::PlayGlobal(RulesClass::Instance->GUIMainButtonSound, 0x2000, 1.0);
	auto pSelect = vec[index]->OwnerObject();

	for (auto pTrans = pSelect->Transporter; pTrans; pTrans = pTrans->Transporter)
		pSelect = pTrans;

	if (ObjectClass::CurrentObjects.Count != 1 || !pSelect->IsSelected)
		MapClass::UnselectAll();

	if (!pSelect->InLimbo && !pSelect->Select())
		TacticalClass::Instance->SetTacticalPosition(&pSelect->Location);

	reinterpret_cast<bool(__thiscall*)(ControlClass*, GadgetFlag, DWORD*, KeyModifier)>(0x48E5A0)(this, flags, pKey, KeyModifier::None);
	return true;
}
