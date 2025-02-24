#include "SelectedCameoClass.h"
#include "SelectedInfoClass.h"

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
	SelectedInfoClass::Instance.Hovering = this->GetButtonIndex();
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}

void SelectedCameoClass::OnMouseLeave()
{
	this->Y += 10;
	this->Height -= 10;
	this->Hovering = false;
	SelectedInfoClass::Instance.IsHovering = false;
	SelectedInfoClass::Instance.Hovering = -1;
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}

bool SelectedCameoClass::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
{
	if (this->Disabled)
		return false;

	const auto pTypeExt = SelectedInfoClass::Instance.CurrentSelectCameo[this->GetButtonIndex()].TypeExt;
	const auto groupID = pTypeExt->GetSelectionGroupID();

	if (flags & GadgetFlag::LeftPress)
	{
		if (static_cast<int>(modifier) & static_cast<int>(KeyModifier::Shift))
		{
			std::vector<ObjectClass*> deselects;
			deselects.reserve(ObjectClass::CurrentObjects->Count);

			for (const auto& pCurrent : ObjectClass::CurrentObjects())
			{
				if (const auto pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() == groupID)
						continue;
				}

				deselects.push_back(pCurrent);
			}

			for (const auto& pDeselect : deselects)
				pDeselect->Deselect();
		}
		else
		{
			const int counts = ObjectClass::CurrentObjects->Count;
			std::vector<ObjectClass*> selects;
			selects.reserve(counts);
			std::vector<ObjectClass*> deselects;
			deselects.reserve(counts);

			for (const auto& pCurrent : ObjectClass::CurrentObjects())
			{
				if (const auto pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() == groupID)
					{
						selects.push_back(pCurrent);
						continue;
					}
				}

				deselects.push_back(pCurrent);
			}

			for (const auto& pDeselect : deselects)
				pDeselect->Deselect();

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
			std::vector<ObjectClass*> deselects;
			deselects.reserve(ObjectClass::CurrentObjects->Count);

			for (const auto& pCurrent : ObjectClass::CurrentObjects())
			{
				if (const auto pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() != groupID)
						continue;
				}

				deselects.push_back(pCurrent);
			}

			for (const auto& pDeselect : deselects)
				pDeselect->Deselect();
		}
		else
		{
			std::vector<ObjectClass*> selects;
			selects.reserve(ObjectClass::CurrentObjects->Count);

			for (const auto& pCurrent : ObjectClass::CurrentObjects())
			{
				if (const auto pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() == groupID)
						selects.push_back(pCurrent);
				}
			}

			selects[Unsorted::CurrentFrame % selects.size()]->Deselect();
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

	const auto pSelect = SelectedInfoClass::Instance.CurrentSelectCameo[this->GetButtonIndex()];
	const auto pTypeExt = pSelect.TypeExt;

	if (const auto CameoPCX = pTypeExt->CameoPCX.GetSurface())
	{
		RectangleStruct drawRect { this->X, this->Y, this->Width, this->Height };

		if (this->Hovering)
			drawRect.Height -= 10;

		PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
	}
	else if (const auto pSHP = pTypeExt->OwnerObject()->GetCameo())
	{
		if (const auto MissingCameoPCX = SelectedInfoClass::SearchMissingCameo(pTypeExt->OwnerObject()->WhatAmI(), pSHP))
		{
			RectangleStruct drawRect { this->X, this->Y, this->Width, this->Height };

			if (this->Hovering)
				drawRect.Height -= 10;

			PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
		}
		else
		{
			Point2D position { this->X, this->Y };
			RectangleStruct rect { 0, 0, this->X + this->Width, this->Y + this->Height };

			if (this->Hovering)
				rect.Height -= 10;

			DSurface::Composite->DrawSHP(pTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
				BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}
	}

	ColorStruct fillColor { 0, 0, 0 };
	RectangleStruct fillRect { this->X, this->Y + this->Height, this->Width, 21 };

	if (this->Hovering)
	{
		fillRect.Y -= 10;
		fillRect.Height += 10;
	}

	DSurface::Composite->FillRectTrans(&fillRect, &fillColor, 60);

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
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}

void SelectedMainCameoClass::OnMouseLeave()
{
	this->Hovering = false;
	SelectedInfoClass::Instance.IsHovering = false;
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}
