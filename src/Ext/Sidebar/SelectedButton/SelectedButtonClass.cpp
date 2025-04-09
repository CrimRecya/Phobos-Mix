#include "SelectedButtonClass.h"
#include "SelectedInfoClass.h"

#include <Ext/Side/Body.h>
#include <Ext/Event/Body.h>

SelectedButtonClass::SelectedButtonClass(unsigned int id, int x, int y)
	: ControlClass(id, x, y, 30, 30, GadgetFlag::LeftPress, false)
{
	this->Disabled = !Phobos::Config::SelectedDisplay_Enable || !SelectedInfoClass::Instance.SingleSelect || !SelectedInfoClass::Instance.ObtainSelect;
}

bool SelectedButtonClass::Draw(bool forced)
{
	return false;
}

void SelectedButtonClass::OnMouseEnter()
{
	this->Hovering = true;
	SelectedInfoClass::Instance.IsHovering = true;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

void SelectedButtonClass::OnMouseLeave()
{
	this->Hovering = false;
	SelectedInfoClass::Instance.IsHovering = false;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

bool SelectedButtonClass::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
{
	auto& seIns = SelectedInfoClass::Instance;

	if (seIns.ShouldUpdate)
		seIns.UpdateSelected();

	const auto& vec = seIns.CurrentSelectTechno;

	if (vec.empty())
		return false;

	if (flags & GadgetFlag::LeftPress)
	{
		if (this->ID == (SelectedInfoClass::StartID + 1)) // PushButton
		{
			const auto pExt = vec[0];
			const auto pTechno = pExt->OwnerObject();

			if (pTechno->Owner->IsControlledByCurrentPlayer() && pTechno->IsAlive && !pTechno->Berzerk)
			{
				if (pExt->CanToggleAggressiveStance())
				{
					VocClass::PlayGlobal(RulesClass::Instance->GUIMainButtonSound, 0x2000, 1.0);
					EventExt::RaiseToggleAggressiveStance(pTechno);
				}
			}
		}
		else // AmmoButton
		{
			const auto pExt = vec[0];
			const auto pTechno = pExt->OwnerObject();

			if (pTechno->Owner->IsControlledByCurrentPlayer() && pTechno->Ammo > 0 && pTechno->IsAlive && !pTechno->Berzerk)
			{
				const auto pTypeExt = pExt->TypeExtData;

				if (pTechno->Ammo != pTypeExt->OwnerObject()->Ammo && pTypeExt->CanManualReload)
				{
					VocClass::PlayGlobal(RulesClass::Instance->GUIMainButtonSound, 0x2000, 1.0);
					EventExt::RaiseManualReloadEvent(pTechno);
				}
			}
		}
	}

	reinterpret_cast<bool(__thiscall*)(ControlClass*, GadgetFlag, DWORD*, KeyModifier)>(0x48E5A0)(this, flags, pKey, KeyModifier::None);
	return true;
}

void SelectedButtonClass::DrawInfo() const
{
	const auto pExt = SelectedInfoClass::Instance.CurrentSelectTechno[0];
	const auto pTechno = pExt->OwnerObject();
	const auto pTypeExt = pExt->TypeExtData;
	const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array.Items[ScenarioClass::Instance->PlayerSideIndex]);
	const auto pSHP = pTypeExt->SelectedInfo_Button.Get(pSideExt->SelectedInfo_Button.Get());

	if (!pSHP || pSHP->Frames < 7)
		return;

	const auto position = Point2D { this->X, this->Y };
	const auto rect = RectangleStruct { 0, 0, this->X + this->Width, this->Y + this->Height };

	DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
		pSHP, 0, &position, &rect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);

	if (this->ID == (SelectedInfoClass::StartID + 1)) // PushButton
	{
		int frame = 1;

		if (pExt->CanToggleAggressiveStance() && pTechno->IsAlive && !pTechno->Berzerk)
			frame = !pExt->GetAggressiveStance() ? 3 : 2;

		DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
			pSHP, frame, &position, &rect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);

		if (this->Hovering)
		{
			auto location = Point2D { this->X + this->Width + 10, this->Y + 4 };
			const auto text = GeneralUtils::LoadStringUnlessMissing("TIP:AggressiveStance", L"AggressiveStance");
			RectangleStruct drawRect = Drawing::GetTextDimensions(text, location, 0, 3, 2);
			location += Point2D { 4, 1 };
			drawRect.Width += 8;
			ColorStruct color { 0, 0, 0 };
			DSurface::Composite->FillRectTrans(&drawRect, &color, 40);
			DSurface::Composite->DrawRect(&drawRect, COLOR_WHITE);
			DSurface::Composite->DrawText(text, &location, COLOR_WHITE);
		}
	}
	else // AmmoButton
	{
		int frame = 4;

		if (pTypeExt->CanManualReload && pTechno->IsAlive && !pTechno->Berzerk && pTechno->Ammo != pTechno->GetTechnoType()->Ammo)
			frame = pTechno->Ammo ? 6 : 5;

		DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
			pSHP, frame, &position, &rect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);

		if (this->Hovering)
		{
			auto location = Point2D { this->X + this->Width + 10, this->Y + 4 };
			const auto text = GeneralUtils::LoadStringUnlessMissing("TIP:ManualReloadAmmo", L"ManualReloadAmmo");
			RectangleStruct drawRect = Drawing::GetTextDimensions(text, location, 0, 3, 2);
			location += Point2D { 4, 1 };
			drawRect.Width += 8;
			ColorStruct color { 0, 0, 0 };
			DSurface::Composite->FillRectTrans(&drawRect, &color, 40);
			DSurface::Composite->DrawRect(&drawRect, COLOR_WHITE);
			DSurface::Composite->DrawText(text, &location, COLOR_WHITE);
		}
	}
}

// ----------------------------------------

SelectedNotButtonClass::SelectedNotButtonClass(unsigned int id, int x, int y)
	: ControlClass(id, x, y, 14, 14, static_cast<GadgetFlag>(0), false)
{
	this->Disabled = !Phobos::Config::SelectedDisplay_Enable || !SelectedInfoClass::Instance.SingleSelect || !SelectedInfoClass::Instance.ObtainSelect;
}

bool SelectedNotButtonClass::Draw(bool forced)
{
	return false;
}

void SelectedNotButtonClass::OnMouseEnter()
{
	this->Hovering = true;
	SelectedInfoClass::Instance.IsHovering = true;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

void SelectedNotButtonClass::OnMouseLeave()
{
	this->Hovering = false;
	SelectedInfoClass::Instance.IsHovering = false;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

void SelectedNotButtonClass::DrawInfo() const
{
	const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array.Items[ScenarioClass::Instance->PlayerSideIndex]);
	const auto pSHP = pSideExt->SelectedInfo_Buff.Get();

	if (!pSHP || pSHP->Frames < 15)
		return;

	const auto position = Point2D { this->X, this->Y };
	const auto pExt = SelectedInfoClass::Instance.CurrentSelectTechno[0];
	const auto pTechno = pExt->OwnerObject();

	if (this->ID == (SelectedInfoClass::StartID + 4)) // InfoIconA
	{
		const double mult = pTechno->FirepowerMultiplier * pExt->AE.FirepowerMultiplier * (pTechno->HasAbility(Ability::Firepower) ? RulesClass::Instance->VeteranCombat : 1.0);
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
			auto location = Point2D { this->X + this->Width + 10, this->Y - 3 };
			wchar_t buffer[0x20];
			swprintf_s(buffer, GeneralUtils::LoadStringUnlessMissing("TIP:PowerMult", L"PowerMult:%5.2f"), mult);
			RectangleStruct drawRect = Drawing::GetTextDimensions(buffer, location, 0, 3, 2);
			location += Point2D { 4, 1 };
			drawRect.Width += 8;
			ColorStruct color { 0, 0, 0 };
			DSurface::Composite->FillRectTrans(&drawRect, &color, 40);
			DSurface::Composite->DrawRect(&drawRect, COLOR_WHITE);
			DSurface::Composite->DrawText(buffer, &location, COLOR_WHITE);
		}
	}
	else if (this->ID == (SelectedInfoClass::StartID + 5)) // InfoIconD
	{
		const auto mult = pTechno->ArmorMultiplier * pExt->AE.ArmorMultiplier * (pTechno->HasAbility(Ability::Stronger) ? RulesClass::Instance->VeteranArmor : 1.0);
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
			auto location = Point2D { this->X + this->Width + 10, this->Y - 3 };
			wchar_t buffer[0x20];
			swprintf_s(buffer, GeneralUtils::LoadStringUnlessMissing("TIP:ArmorMult", L"ArmorMult:%5.2f"), mult);
			RectangleStruct drawRect = Drawing::GetTextDimensions(buffer, location, 0, 3, 2);
			location += Point2D { 4, 1 };
			drawRect.Width += 8;
			ColorStruct color { 0, 0, 0 };
			DSurface::Composite->FillRectTrans(&drawRect, &color, 40);
			DSurface::Composite->DrawRect(&drawRect, COLOR_WHITE);
			DSurface::Composite->DrawText(buffer, &location, COLOR_WHITE);
		}
	}
	else // InfoIconS
	{
		double mult = 1.0;

		if (const auto pFoot = abstract_cast<FootClass*>(pTechno))
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
			auto location = Point2D { this->X + this->Width + 10, this->Y - 3 };
			wchar_t buffer[0x20];
			swprintf_s(buffer, GeneralUtils::LoadStringUnlessMissing("TIP:SpeedMult", L"SpeedMult:%5.2f"), mult);
			RectangleStruct drawRect = Drawing::GetTextDimensions(buffer, location, 0, 3, 2);
			location += Point2D { 4, 1 };
			drawRect.Width += 8;
			ColorStruct color { 0, 0, 0 };
			DSurface::Composite->FillRectTrans(&drawRect, &color, 40);
			DSurface::Composite->DrawRect(&drawRect, COLOR_WHITE);
			DSurface::Composite->DrawText(buffer, &location, COLOR_WHITE);
		}
	}
}

// ----------------------------------------

SelectedToggleClass::SelectedToggleClass(unsigned int id, int x, int y)
	: ControlClass(id, x, y, 10, 14, GadgetFlag::LeftPress, false)
{
	this->Disabled = (id == (SelectedInfoClass::StartID + 9)) && (!Phobos::Config::SelectedDisplay_Enable || !SelectedInfoClass::Instance.SingleSelect || !SelectedInfoClass::Instance.ObtainSelect);
}

bool SelectedToggleClass::Draw(bool forced)
{
	return false;
}

void SelectedToggleClass::OnMouseEnter()
{
	this->Hovering = true;
	SelectedInfoClass::Instance.IsHovering = true;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

void SelectedToggleClass::OnMouseLeave()
{
	this->Hovering = false;
	SelectedInfoClass::Instance.IsHovering = false;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

bool SelectedToggleClass::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
{
	if (flags & GadgetFlag::LeftPress)
	{
		if (this->ID == (SelectedInfoClass::StartID + 8)) // Toggle on/off
			SelectedInfoClass::Instance.SwitchVisible();
		else // Toggle expand/storage
			SelectedInfoClass::Instance.SwitchExpand();
	}

	reinterpret_cast<bool(__thiscall*)(ControlClass*, GadgetFlag, DWORD*, KeyModifier)>(0x48E5A0)(this, flags, pKey, KeyModifier::None);
	return true;
}

void SelectedToggleClass::DrawInfo() const
{
	const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array.Items[ScenarioClass::Instance->PlayerSideIndex]);
	auto rect = RectangleStruct { 0, 0, this->X + this->Width, this->Y + this->Height };
	const auto pSHP = pSideExt->SelectedInfo_Toggle.Get();

	if (!pSHP || pSHP->Frames < 4)
		return;

	const auto position = Point2D { this->X, this->Y };
	const auto frame = this->ID == (SelectedInfoClass::StartID + 8) ? (Phobos::Config::SelectedDisplay_Enable ? 1 : 0) : (Phobos::Config::SelectedDisplay_Expand ? 3 : 2);
	DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
		pSHP, frame, &position, &rect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
}

// ----------------------------------------

SelectedScrollClass::SelectedScrollClass(unsigned int id, int x, int y)
	: ControlClass(id, x, y, 10, 14, GadgetFlag::LeftPress, false)
{
	this->Disabled = !Phobos::Config::SelectedDisplay_Enable || !SelectedInfoClass::Instance.SingleSelect || !SelectedInfoClass::Instance.ObtainSelect;
}

bool SelectedScrollClass::Draw(bool forced)
{
	return false;
}

void SelectedScrollClass::OnMouseEnter()
{
	this->Hovering = true;
	SelectedInfoClass::Instance.IsHovering = true;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

void SelectedScrollClass::OnMouseLeave()
{
	this->Hovering = false;
	SelectedInfoClass::Instance.IsHovering = false;
	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

bool SelectedScrollClass::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
{
	if (flags & GadgetFlag::LeftPress)
	{
		if (this->ID == (SelectedInfoClass::StartID + 30)) // Scroll left
		{
			if (SelectedInfoClass::Instance.ScrollLeft())
				VocClass::PlayGlobal(RulesClass::Instance->GUIMainButtonSound, 0x2000, 1.0);
		}
		else // Scroll right
		{
			if (SelectedInfoClass::Instance.ScrollRight())
				VocClass::PlayGlobal(RulesClass::Instance->GUIMainButtonSound, 0x2000, 1.0);
		}
	}

	reinterpret_cast<bool(__thiscall*)(ControlClass*, GadgetFlag, DWORD*, KeyModifier)>(0x48E5A0)(this, flags, pKey, KeyModifier::None);
	return true;
}

void SelectedScrollClass::DrawInfo() const
{
	const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array.Items[ScenarioClass::Instance->PlayerSideIndex]);
	auto rect = RectangleStruct { 0, 0, this->X + this->Width, this->Y + this->Height };
	const auto pSHP = pSideExt->SelectedInfo_Toggle.Get();

	if (!pSHP || pSHP->Frames < 8)
		return;

	const auto position = Point2D { this->X, this->Y };
	auto& seIns = SelectedInfoClass::Instance;
	const auto frame = this->ID == (SelectedInfoClass::StartID + 30) ? (seIns.CanScrollLeft() ? 4 : 5) : (seIns.CanScrollRight() ? 6 : 7);
	DSurface::Composite->DrawSHP(pSideExt->SelectedInfo_Palette.GetOrDefaultConvert(FileSystem::ANIM_PAL),
		pSHP, frame, &position, &rect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
}
