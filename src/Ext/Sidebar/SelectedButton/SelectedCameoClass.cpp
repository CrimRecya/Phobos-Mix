#include "SelectedCameoClass.h"
#include "SelectedInfoClass.h"

#include "SpawnManagerClass.h"

SelectedCameoClass::SelectedCameoClass(unsigned int id, int x, int y)
	: ControlClass(id, x, y, 60, 48, (GadgetFlag::LeftPress | GadgetFlag::RightPress), false)
{
	this->Disabled = !Phobos::Config::SelectedDisplay_Enable || SelectedInfoClass::Instance.SingleSelect;
}

bool SelectedCameoClass::Draw(bool forced)
{
	return false;
}

void SelectedCameoClass::OnMouseEnter()
{
	this->Y -= 10;
	this->Height += 10;
	this->Hovering = true;
	SelectedInfoClass::Instance.IsHovering = true;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

void SelectedCameoClass::OnMouseLeave()
{
	this->Y += 10;
	this->Height -= 10;
	this->Hovering = false;
	SelectedInfoClass::Instance.IsHovering = false;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

bool SelectedCameoClass::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
{
	if (this->Disabled)
		return false;

	auto& seIns = SelectedInfoClass::Instance;

	if (seIns.ShouldUpdate)
		seIns.UpdateSelected();

	VocClass::PlayGlobal(RulesClass::Instance->GUIMainButtonSound, 0x2000, 1.0);
	const auto& seCST = seIns.CurrentSelectTechno;

	if (seIns.CurrentSelectCameo.size() == 1 || Phobos::Config::SelectedDisplay_Expand)
	{
		const auto pSelect = seCST[this->GetButtonIndex() + seIns.Current]->OwnerObject();

		if (flags & GadgetFlag::LeftPress)
		{
			for (const auto& pCurrent : seCST)
			{
				if (pCurrent->OwnerObject() != pSelect)
					pCurrent->OwnerObject()->Deselect();
			}
		}
		else if (flags & GadgetFlag::RightPress)
		{
			pSelect->Deselect();
		}
	}
	else
	{
		const auto pTypeExt = seIns.CurrentSelectCameo[this->GetButtonIndex() + seIns.Current].TypeExt;
		const auto groupID = pTypeExt->GetSelectionGroupID();

		if (flags & GadgetFlag::LeftPress)
		{
			if (static_cast<int>(modifier) & static_cast<int>(KeyModifier::Shift))
			{
				for (const auto& pCurrent : seCST)
				{
					if (pCurrent->TypeExtData->GetSelectionGroupID() != groupID)
						pCurrent->OwnerObject()->Deselect();
				}
			}
			else
			{
				std::vector<TechnoClass*> selects;

				for (const auto& pCurrent : seCST)
				{
					if (pCurrent->TypeExtData->GetSelectionGroupID() == groupID)
					{
						selects.push_back(pCurrent->OwnerObject());
						continue;
					}

					pCurrent->OwnerObject()->Deselect();
				}

				const int size = selects.size();
				const int random = Unsorted::CurrentFrame % size;

				for (int i = 0; i < size; ++i)
				{
					if (i != random)
						selects[i]->Deselect();
				}
			}
		}
		else if (flags & GadgetFlag::RightPress)
		{
			if (static_cast<int>(modifier) & static_cast<int>(KeyModifier::Shift))
			{
				for (const auto& pCurrent : seCST)
				{
					if (pCurrent->TypeExtData->GetSelectionGroupID() == groupID)
						pCurrent->OwnerObject()->Deselect();
				}
			}
			else
			{
				std::vector<TechnoClass*> selects;

				for (const auto& pCurrent : seCST)
				{
					if (pCurrent->TypeExtData->GetSelectionGroupID() == groupID)
						selects.push_back(pCurrent->OwnerObject());
				}

				selects[Unsorted::CurrentFrame % selects.size()]->Deselect();
			}
		}
	}

	reinterpret_cast<bool(__thiscall*)(ControlClass*, GadgetFlag, DWORD*, KeyModifier)>(0x48E5A0)(this, flags, pKey, KeyModifier::None);
	return true;
}

inline int SelectedCameoClass::GetButtonIndex() const
{
	return this->ID - SelectedInfoClass::StartID - 10;
}

void SelectedCameoClass::DrawInfo() const
{
	if (this->Disabled)
		return;

	auto drawCameo = [this](TechnoTypeExt::ExtData* pTypeExt)
	{
		if (const auto CameoPCX = pTypeExt->CameoPCX.GetSurface())
		{
			RectangleStruct drawRect { this->X, this->Y, 60, 48 };
			PCX::Instance.BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
		}
		else if (const auto pSHP = pTypeExt->OwnerObject()->GetCameo())
		{
			if (const auto MissingCameoPCX = SelectedInfoClass::SearchMissingCameo(pTypeExt->OwnerObject()->WhatAmI(), pSHP))
			{
				RectangleStruct drawRect { this->X, this->Y, 60, 48 };
				PCX::Instance.BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
			}
			else
			{
				Point2D position { this->X, this->Y };
				RectangleStruct rect { 0, 0, this->X + 60, this->Y + 48 };
				DSurface::Composite->DrawSHP(pTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
					BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
			}
		}

		ColorStruct fillColor { 0, 0, 0 };
		RectangleStruct fillRect { this->X, this->Y + 48, this->Width, 21 };

		if (this->Hovering)
			fillRect.Height += 10;

		DSurface::Composite->FillRectTrans(&fillRect, &fillColor, 30);
	};

	const auto& seIns = SelectedInfoClass::Instance;

	if (seIns.CurrentSelectCameo.size() == 1 || Phobos::Config::SelectedDisplay_Expand)
	{
		const auto pExt = seIns.CurrentSelectTechno[this->GetButtonIndex() + seIns.Current];
		const auto pTechno = pExt->OwnerObject();
		const auto pType = pTechno->GetTechnoType();
		const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

		drawCameo(pTypeExt);

		const auto pRules = RulesClass::Instance;
		const auto ratio = static_cast<double>(pTechno->Health) / pType->Strength;
		auto rect = RectangleStruct { this->X + 4, this->Y + 2, 52, 8 };
		DSurface::Composite->FillRect(&rect, COLOR_BLACK);

		rect = RectangleStruct { rect.X + 1, rect.Y + 1, static_cast<int>(50 * ratio + 0.5), 3 };
		const auto color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
		DSurface::Composite->FillRect(&rect, color);
		const auto pShield = pExt->Shield.get();

		if (pShield && !pShield->IsBrokenAndNonRespawning())
		{
			rect.Width = static_cast<int>(50 * (static_cast<double>(pShield->GetHP()) / pShield->GetType()->Strength.Get()) + 0.5);
			ColorStruct fillColor { 153, 153, 255 };
			DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
		}

		if (pTechno->IsIronCurtained())
		{
			const auto& timer = pTechno->IronCurtainTimer;
			rect.Width = static_cast<int>(50 * (static_cast<double>(timer.GetTimeLeft()) / timer.TimeLeft) + 0.5);
			ColorStruct fillColor { 200, 50, 50 };
			DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
		}

		int value = -1, maxValue = 0;
		TechnoExt::GetValuesForDisplay(pTechno, pTypeExt->SelectedInfo_CameoType.Get(), value, maxValue);

		rect.Y += 4;
		rect.Width = static_cast<int>(50 * ((value <= -1 || maxValue <= 0) ? 1.0 : (static_cast<double>(value) / maxValue)) + 0.5);
		--rect.Height;
		DSurface::Composite->FillRect(&rect, COLOR_WHITE);
	}
	else
	{
		const auto pSelect = seIns.CurrentSelectCameo[this->GetButtonIndex() + seIns.Current];
		drawCameo(pSelect.TypeExt);
		const int count = pSelect.Count;

		if (count > 1)
		{
			wchar_t text[0x20];
			swprintf_s(text, L"%d", count);
			TextPrintType printType = TextPrintType::Background | TextPrintType::FullShadow | TextPrintType::Point8;
			const COLORREF color = Drawing::RGB_To_Int(Drawing::TooltipColor);
			Point2D textPosition { this->X + 1, this->Y + 1 };
			RectangleStruct surfaceRect { 0, 0, this->X + this->Width, this->Y + this->Height };
			DSurface::Composite->DrawTextA(text, &surfaceRect, &textPosition, color, 0, printType);
		}
	}
}

// ----------------------------------------

SelectedMainCameoClass::SelectedMainCameoClass(unsigned int id, int x, int y)
	: ControlClass(id, x, y, 60, 48, static_cast<GadgetFlag>(0), false)
{
	this->Disabled = !Phobos::Config::SelectedDisplay_Enable || !SelectedInfoClass::Instance.SingleSelect;
}

bool SelectedMainCameoClass::Draw(bool forced)
{
	return false;
}

void SelectedMainCameoClass::OnMouseEnter()
{
	this->Hovering = true;
	SelectedInfoClass::Instance.IsHovering = true;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

void SelectedMainCameoClass::OnMouseLeave()
{
	this->Hovering = false;
	SelectedInfoClass::Instance.IsHovering = false;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}
