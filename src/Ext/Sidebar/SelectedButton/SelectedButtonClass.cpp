#include "SelectedButtonClass.h"
#include "SelectedInfoClass.h"

#include <EventClass.h>
#include <HouseClass.h>

#include <Ext/Side/Body.h>

SelectedButtonClass::SelectedButtonClass(unsigned int id, int x, int y)
	: ControlClass(id, x, y, 30, 30, GadgetFlag::LeftPress, false)
{
	this->Disabled = !Phobos::Config::SelectedDisplay_Enable || !SelectedInfoClass::Instance.SingleSelect;
}

bool SelectedButtonClass::Draw(bool forced)
{
	return false;
}

void SelectedButtonClass::OnMouseEnter()
{
	this->Hovering = true;
	SelectedInfoClass::Instance.IsHovering = true;
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}

void SelectedButtonClass::OnMouseLeave()
{
	this->Hovering = false;
	SelectedInfoClass::Instance.IsHovering = false;
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}

bool SelectedButtonClass::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
{
	if (flags & GadgetFlag::LeftPress)
	{
		if (this->ID == (SelectedInfoClass::StartID + 1)) // PushButton
		{
			// TODO event
		}
		else // AmmoButton
		{
			// TODO event
		}
	}

	reinterpret_cast<bool(__thiscall*)(ControlClass*, GadgetFlag, DWORD*, KeyModifier)>(0x48E5A0)(this, flags, pKey, KeyModifier::None);
	return true;
}

void SelectedButtonClass::DrawInfo() const
{
	const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array->Items[ScenarioClass::Instance->PlayerSideIndex]);
	const auto pSHP = pSideExt->SelectedInfo_Button.Get();

	if (!pSHP || pSHP->Frames < 6)
		return;

	const auto position = Point2D { this->X + 5, this->Y + 5 };
	const auto pObject = ObjectClass::CurrentObjects->Items[0];
	const auto pTechno = abstract_cast<TechnoClass*>(pObject);

	if (this->ID == (SelectedInfoClass::StartID + 1)) // PushButton
	{
		int frame = 0;

		if (false)
			frame = false ? 2 : 1;

		RectangleStruct rect { 0, 0, this->X + 25, this->Y + 25 };
		DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
			pSHP, frame, &position, &rect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);

		if (this->Hovering)
		{
			// TODO Draw text
		}
	}
	else // AmmoButton
	{
		int frame = 3;

		if (pTechno && TechnoTypeExt::ExtMap.Find(pTechno->GetTechnoType())->CanManualReload)
			frame = pTechno->Ammo ? 5 : 4;

		RectangleStruct rect { 0, 0, position.X + 20, position.Y + 20 };
		DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
			pSHP, frame, &position, &rect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);

		if (this->Hovering)
		{
			// TODO Draw text
		}
	}
}

// ----------------------------------------

SelectedNotButtonClass::SelectedNotButtonClass(unsigned int id, int x, int y)
	: ControlClass(id, x, y, 14, 14, static_cast<GadgetFlag>(0), false)
{
	this->Disabled = !Phobos::Config::SelectedDisplay_Enable || !SelectedInfoClass::Instance.SingleSelect;
}

bool SelectedNotButtonClass::Draw(bool forced)
{
	return false;
}

void SelectedNotButtonClass::OnMouseEnter()
{
	this->Hovering = true;
	SelectedInfoClass::Instance.IsHovering = true;
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}

void SelectedNotButtonClass::OnMouseLeave()
{
	this->Hovering = false;
	SelectedInfoClass::Instance.IsHovering = false;
	MouseClass::Instance->UpdateCursor(MouseCursorType::Default, false);
}

void SelectedNotButtonClass::DrawInfo() const
{
	const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array->Items[ScenarioClass::Instance->PlayerSideIndex]);
	const auto pSHP = pSideExt->SelectedInfo_Buff.Get();

	if (!pSHP || pSHP->Frames < 15)
		return;

	const auto position = Point2D { this->X, this->Y };
	const auto pObject = ObjectClass::CurrentObjects->Items[0];
	const auto pTechno = abstract_cast<TechnoClass*>(pObject);

	if (this->ID == (SelectedInfoClass::StartID + 4)) // InfoIconA
	{
		double mult = 1.0;

		if (const auto pExt = TechnoExt::ExtMap.Find(pTechno))
			mult = pTechno->FirepowerMultiplier * pExt->AE.FirepowerMultiplier * (pTechno->HasAbility(Ability::Firepower) ? RulesClass::Instance->VeteranCombat : 1.0);

		int frame = 0;

		if (mult - 1.0 > 1e-10)
			frame = mult > 2.0 ? 4 : 3;
		else if (mult - 1.0 < -1e-10)
			frame = mult < 0.5 ? 2 : 1;

		RectangleStruct rect { 0, 0, this->X + this->Width, this->Y + this->Height };
		DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
			pSHP, frame, &position, &rect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);

		if (this->Hovering)
		{
			// TODO Draw text
		}
	}
	else if (this->ID == (SelectedInfoClass::StartID + 5)) // InfoIconD
	{
		double mult = 1.0;

		if (const auto pExt = TechnoExt::ExtMap.Find(pTechno))
			mult = pTechno->ArmorMultiplier * pExt->AE.ArmorMultiplier * (pTechno->HasAbility(Ability::Stronger) ? RulesClass::Instance->VeteranArmor : 1.0);

		int frame = 5;

		if (mult - 1.0 > 1e-10)
			frame = mult > 2.0 ? 9 : 8;
		else if (mult - 1.0 < -1e-10)
			frame = mult < 0.5 ? 7 : 6;

		RectangleStruct rect { 0, 0, this->X + this->Width, this->Y + this->Height };
		DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
			pSHP, frame, &position, &rect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);

		if (this->Hovering)
		{
			// TODO Draw text
		}
	}
	else // InfoIconS
	{
		double mult = 1.0;

		if (const auto pFoot = abstract_cast<FootClass*>(pObject))
			mult = pFoot->SpeedMultiplier * TechnoExt::ExtMap.Find(pFoot)->AE.SpeedMultiplier * (pFoot->HasAbility(Ability::Faster) ? RulesClass::Instance->VeteranSpeed : 1.0);

		int frame = 10;

		if (mult - 1.0 > 1e-10)
			frame = mult > 2.0 ? 14 : 13;
		else if (mult - 1.0 < -1e-10)
			frame = mult < 0.5 ? 12 : 11;

		RectangleStruct rect { 0, 0, this->X + this->Width, this->Y + this->Height };
		DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
			pSHP, frame, &position, &rect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);

		if (this->Hovering)
		{
			// TODO Draw text
		}
	}
}
