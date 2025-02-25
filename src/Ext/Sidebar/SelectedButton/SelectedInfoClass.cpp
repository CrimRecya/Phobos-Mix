#include "SelectedInfoClass.h"

#include <AircraftClass.h>
#include <FactoryClass.h>
#include <SpawnManagerClass.h>
#include <SuperClass.h>
#include <MessageListClass.h>

#include <Ext/BuildingType/Body.h>

SelectedInfoClass SelectedInfoClass::Instance;

void SelectedInfoClass::InitClear()
{
	if (this->MainColumn)
	{
		GScreenClass::Instance->RemoveButton(this->MainColumn);
		this->MainColumn = nullptr;
	}

	if (this->PushButton)
	{
		GScreenClass::Instance->RemoveButton(this->PushButton);
		this->PushButton = nullptr;
	}

	if (this->AmmoButton)
	{
		GScreenClass::Instance->RemoveButton(this->AmmoButton);
		this->AmmoButton = nullptr;
	}

	if (this->MainCameo)
	{
		GScreenClass::Instance->RemoveButton(this->MainCameo);
		this->MainCameo = nullptr;
	}

	if (this->InfoIconA)
	{
		GScreenClass::Instance->RemoveButton(this->InfoIconA);
		this->InfoIconA = nullptr;
	}

	if (this->InfoIconD)
	{
		GScreenClass::Instance->RemoveButton(this->InfoIconD);
		this->InfoIconD = nullptr;
	}

	if (this->InfoIconS)
	{
		GScreenClass::Instance->RemoveButton(this->InfoIconS);
		this->InfoIconS = nullptr;
	}

	if (this->MainBottom)
	{
		GScreenClass::Instance->RemoveButton(this->MainBottom);
		this->MainBottom = nullptr;
	}

	for (int i = 0; i < 20; ++i)
	{
		if (auto& pButton = this->Cameos[i])
		{
			GScreenClass::Instance->RemoveButton(pButton);
			pButton = nullptr;
		}
	}

	this->MaxCameo = 0;
	this->Current = 0;
	this->ShouldUpdate = false;
	this->SingleSelect = true;
	this->IsHovering = false;
}

void SelectedInfoClass::InitIO()
{
	if (Unsorted::ArmageddonMode)
		return;

	const auto bottom = DSurface::Composite->GetHeight() - 32;
	{
		const auto pButton = GameCreate<SelectedColumnClass>(SelectedInfoClass::StartID, 0, bottom - 100, 203, 79);
		pButton->Zap();
		GScreenClass::Instance->AddButton(pButton);
		this->MainColumn = pButton;
	}

	{
		const auto pButton = GameCreate<SelectedButtonClass>(SelectedInfoClass::StartID + 1, 197, bottom - 79);
		pButton->Zap();
		GScreenClass::Instance->AddButton(pButton);
		this->PushButton = pButton;
	}

	{
		const auto pButton = GameCreate<SelectedButtonClass>(SelectedInfoClass::StartID + 2, 197, bottom - 50);
		pButton->Zap();
		GScreenClass::Instance->AddButton(pButton);
		this->AmmoButton = pButton;
	}

	{
		const auto pButton = GameCreate<SelectedMainCameoClass>(SelectedInfoClass::StartID + 3, 6, bottom - 95);
		pButton->Zap();
		GScreenClass::Instance->AddButton(pButton);
		this->MainCameo = pButton;
	}

	{
		const auto pButton = GameCreate<SelectedNotButtonClass>(SelectedInfoClass::StartID + 4, 179, bottom - 93);
		pButton->Zap();
		GScreenClass::Instance->AddButton(pButton);
		this->InfoIconA = pButton;
	}

	{
		const auto pButton = GameCreate<SelectedNotButtonClass>(SelectedInfoClass::StartID + 5, 179, bottom - 77);
		pButton->Zap();
		GScreenClass::Instance->AddButton(pButton);
		this->InfoIconD = pButton;
	}

	{
		const auto pButton = GameCreate<SelectedNotButtonClass>(SelectedInfoClass::StartID + 6, 179, bottom - 61);
		pButton->Zap();
		GScreenClass::Instance->AddButton(pButton);
		this->InfoIconS = pButton;
	}

	{
		const auto pButton = GameCreate<SelectedBottomClass>(SelectedInfoClass::StartID + 6, 0, bottom - 21, 236, 21);
		pButton->Zap();
		GScreenClass::Instance->AddButton(pButton);
		this->MainBottom = pButton;
	}

	Point2D position { 0, (bottom - 69) };

	for (int i = 0; i < 20; ++i)
	{
		const auto pButton = GameCreate<SelectedCameoClass>(SelectedInfoClass::StartID + 10 + i, position.X, position.Y);
		position.X += 60;

		pButton->Zap();
		GScreenClass::Instance->AddButton(pButton);
		this->Cameos[i] = pButton;
	}

	this->ShouldUpdate = true;
	this->MaxCameo = std::min(20, DSurface::Composite->GetWidth() / 150);

	for (int i = this->GetMaxCameo(); i < 20; ++i)
	{
		if (const auto& pButton = this->Cameos[i])
			pButton->Disabled = true;
	}
}

void SelectedInfoClass::SwitchVisible()
{
	Phobos::Config::SelectedDisplay_Enable = !Phobos::Config::SelectedDisplay_Enable;

	this->UpdateVisible();

	const auto message = Phobos::Config::SelectedDisplay_Enable
		? GeneralUtils::LoadStringUnlessMissing("TXT_SELECT_VISIBLE", L"Set select info visible.")
		: GeneralUtils::LoadStringUnlessMissing("TXT_SELECT_INVISIBLE", L"Set select info invisible.");

	if (Phobos::Config::SelectedDisplay_Enable)
		this->ShouldUpdate = true;

	MessageListClass::Instance->PrintMessage(message, RulesClass::Instance->MessageDelay, HouseClass::CurrentPlayer->ColorSchemeIndex, true);
}

void SelectedInfoClass::UpdateVisible()
{
	auto disabled = !Phobos::Config::SelectedDisplay_Enable || !this->SingleSelect;

	if (const auto& pButton = this->MainColumn)
		pButton->Disabled = disabled;

	if (const auto& pButton = this->PushButton)
		pButton->Disabled = disabled;

	if (const auto& pButton = this->AmmoButton)
		pButton->Disabled = disabled;

	if (const auto& pButton = this->MainCameo)
		pButton->Disabled = disabled;

	if (const auto& pButton = this->InfoIconA)
		pButton->Disabled = disabled;

	if (const auto& pButton = this->InfoIconD)
		pButton->Disabled = disabled;

	if (const auto& pButton = this->InfoIconS)
		pButton->Disabled = disabled;

	disabled = !Phobos::Config::SelectedDisplay_Enable || this->SingleSelect;
	const int size = this->CurrentSelectCameo.size();

	for (int i = 0; i < this->GetMaxCameo(); ++i)
	{
		if (const auto& pButton = this->Cameos[i])
			pButton->Disabled = disabled || (i >= size);
	}

	const int overflow = size - this->GetMaxCameo();
	int cameoCount = 0;

	if (overflow > 0)
	{
		if (this->Current > overflow)
			this->Current = overflow;

		cameoCount = this->GetMaxCameo();
	}
	else
	{
		this->Current = 0;
		cameoCount = size;
	}

	if (const auto& pButton = this->MainBottom)
	{
		pButton->Disabled = !Phobos::Config::SelectedDisplay_Enable;
		pButton->Width = std::max(236, cameoCount * 60);
	}
}

void SelectedInfoClass::UpdateSelected()
{
	this->ShouldUpdate = false;

	if (this->CurrentSelectCameo.size())
		this->CurrentSelectCameo.clear();

	this->CurrentSelectCameo.reserve(10);

	std::map<int, int> CurrentSelectInfantry;
	std::map<int, int> CurrentSelectUnit;
	std::map<int, int> CurrentSelectAircraft;
	std::map<int, int> CurrentSelectBuilding;

	const auto& vec = ObjectClass::CurrentObjects();
	this->SingleSelect = vec.Count <= 1;

	for (const auto& pCurrent : vec)
	{
		const auto absType = pCurrent->WhatAmI();

		if (absType == AbstractType::Infantry)
			++CurrentSelectInfantry[static_cast<InfantryClass*>(pCurrent)->Type->ArrayIndex];
		else if (absType == AbstractType::Unit)
			++CurrentSelectUnit[static_cast<UnitClass*>(pCurrent)->Type->ArrayIndex];
		else if (absType == AbstractType::Aircraft)
			++CurrentSelectAircraft[static_cast<AircraftClass*>(pCurrent)->Type->ArrayIndex];
		else if (absType == AbstractType::Building)
			++CurrentSelectBuilding[static_cast<BuildingClass*>(pCurrent)->Type->ArrayIndex];
	}

	if (CurrentSelectInfantry.size())
	{
		for (const auto& [index, count] : CurrentSelectInfantry)
		{
			if (const auto pTypeExt = TechnoTypeExt::ExtMap.Find(InfantryTypeClass::Array->Items[index]))
				this->AddToSelected(pTypeExt, count, 0);
		}
	}

	if (CurrentSelectUnit.size())
	{
		const int checkStart = this->CurrentSelectCameo.size();

		for (const auto& [index, count] : CurrentSelectUnit)
		{
			if (const auto pTypeExt = TechnoTypeExt::ExtMap.Find(UnitTypeClass::Array->Items[index]))
				this->AddToSelected(pTypeExt, count, checkStart);
		}
	}

	if (CurrentSelectAircraft.size())
	{
		const int checkStart = this->CurrentSelectCameo.size();

		for (const auto& [index, count] : CurrentSelectAircraft)
		{
			if (const auto pTypeExt = TechnoTypeExt::ExtMap.Find(AircraftTypeClass::Array->Items[index]))
				this->AddToSelected(pTypeExt, count, checkStart);
		}
	}

	if (CurrentSelectBuilding.size())
	{
		const int checkStart = this->CurrentSelectCameo.size();

		for (const auto& [index, count] : CurrentSelectBuilding)
		{
			if (const auto pTypeExt = TechnoTypeExt::ExtMap.Find(BuildingTypeClass::Array->Items[index]))
				this->AddToSelected(pTypeExt, count, checkStart);
		}
	}

	this->UpdateVisible();
}

void SelectedInfoClass::AddToSelected(TechnoTypeExt::ExtData* pTypeExt, int count, int checkIndex)
{
	const auto groupID = pTypeExt->GetSelectionGroupID();
	const int currentCounts = this->CurrentSelectCameo.size();

	for (int i = checkIndex; i < currentCounts; ++i)
	{
		if (this->CurrentSelectCameo[i].TypeExt->GetSelectionGroupID() == groupID)
		{
			this->CurrentSelectCameo[i].Count += count;
			return;
		}
	}

	SelectRecordStruct select { pTypeExt, count };
	this->CurrentSelectCameo.push_back(select);
}

BSurface* SelectedInfoClass::SearchMissingCameo(AbstractType absType, SHPStruct* pSHP)
{
	const auto pRulesExt = RulesExt::Global();
	const auto pCameoRef = pSHP->AsReference();
	char pFilename[0x20];
	strcpy_s(pFilename, pRulesExt->MissingCameo.data());
	_strlwr_s(pFilename);

	if (!_stricmp(pCameoRef->Filename, GameStrings::XXICON_SHP))
	{
		if (absType == AbstractType::InfantryType)
		{
			if (const auto MissingCameoPCX = pRulesExt->SelectedInfantryMissingPCX.GetSurface())
				return MissingCameoPCX;
		}
		else if (absType == AbstractType::UnitType)
		{
			if (const auto MissingCameoPCX = pRulesExt->SelectedVehicleMissingPCX.GetSurface())
				return MissingCameoPCX;
		}
		else if (absType == AbstractType::AircraftType)
		{
			if (const auto MissingCameoPCX = pRulesExt->SelectedAircraftMissingPCX.GetSurface())
				return MissingCameoPCX;
		}
		else if (absType == AbstractType::BuildingType)
		{
			if (const auto MissingCameoPCX = pRulesExt->SelectedBuildingMissingPCX.GetSurface())
				return MissingCameoPCX;
		}

		if (strstr(pFilename, ".pcx"))
		{
			PCX::Instance->LoadFile(pFilename);

			if (const auto MissingCameoPCX = PCX::Instance->GetSurface(pFilename))
				return MissingCameoPCX;
		}
	}

	return nullptr;
}

void SelectedInfoClass::GetValuesForDisplay(TechnoClass* pThis, ObjectTypeClass* pFakeType, DisplayInfoType infoType, int& value, int& maxValue)
{
	const auto pTrueType = pThis->GetTechnoType();

	if (pTrueType == pFakeType)
	{
		TechnoExt::GetValuesForDisplay(pThis, infoType, value, maxValue);
		return;
	}

	const auto pType = TechnoTypeExt::GetTechnoType(pFakeType);
	int fakeValue = 0;

	if (infoType == DisplayInfoType::Health)
	{
		value = pThis->Health;
		maxValue = pTrueType->Strength;
		fakeValue = pFakeType->Strength;
	}
	else if (pType)
	{
		TechnoExt::GetValuesForDisplay(pThis, infoType, value, maxValue);
		fakeValue = maxValue;
	}

	if (fakeValue <= 0)
	{
		maxValue = fakeValue;
	}
	else if (value >= 0 && maxValue > 0 && fakeValue != maxValue)
	{
		value = (value * fakeValue / maxValue);
		maxValue = fakeValue;
	}
}

TechnoStatus SelectedInfoClass::GetCurrentStatus(TechnoClass* pThis)
{
	const auto pOwner = pThis->Owner;
	ObjectTypeClass* pDisguise = nullptr;

	if ((!pOwner || !pOwner->IsAlliedWith(HouseClass::CurrentPlayer())) && !HouseClass::IsCurrentPlayerObserver())
	{
		pDisguise = pThis->Disguise;

		if (!abstract_cast<TechnoClass*>(pDisguise))
			return TechnoStatus::Unknown;
	}

	const auto mission = pThis->CurrentMission;

	if (pThis->IsUnderEMP() || pThis->Deactivated)
		return TechnoStatus::Deactive;

	if (pDisguise)
	{
		if (const auto pFoot = abstract_cast<FootClass*>(pThis))
		{
			if (pFoot->LocomotorSource)
				return TechnoStatus::Locomotor;

			if (pFoot->Locomotor->Is_Moving_Now())
				return TechnoStatus::Move;
		}

		return TechnoStatus::Guard;
	}

	if (const auto pBuilding = abstract_cast<BuildingClass*>(pThis))
	{
		if (!pOwner->IsControlledByHuman())
		{
			if (const auto pFactory = pBuilding->Factory)
			{
				if (const auto pProduct = pFactory->Object)
					return TechnoStatus::Produce;
			}
		}
		else if (pBuilding->IsPrimaryFactory)
		{
			const auto pBuildingType = pBuilding->Type;
			const auto factoryType = pBuildingType->Factory;

			if (const auto pFactory = pOwner->GetPrimaryFactory(factoryType, pBuildingType->Naval, BuildCat::DontCare))
			{
				if (const auto pProduct = pFactory->Object)
					return TechnoStatus::Produce;
			}

			if (factoryType == AbstractType::BuildingType)
			{
				if (const auto pFactory = pOwner->Primary_ForDefenses)
				{
					if (const auto pProduct = pFactory->Object)
						return TechnoStatus::Produce;
				}
			}
		}
	}
	else if (const auto pFoot = abstract_cast<FootClass*>(pThis))
	{
		if (pFoot->LocomotorSource)
			return TechnoStatus::Locomotor;
		else if (pFoot->MegaMission == Mission::AttackMove)
			return TechnoStatus::AttackMove;
		else if (mission == Mission::Area_Guard && abstract_cast<FootClass*>(pFoot->ArchiveTarget))
			return TechnoStatus::FollowGuard;
	}

	switch (mission)
	{
	case Mission::ParadropApproach:
		return TechnoStatus::Paradrop;
	case Mission::ParadropOverfly:
	case Mission::SpyplaneApproach:
	case Mission::SpyplaneOverfly:
		return TechnoStatus::Move;
	default:
		break;
	}

	const auto status = static_cast<TechnoStatus>(mission);

	return (status > TechnoStatus::None || status < TechnoStatus::Sleep) ? TechnoStatus::Unknown : status;
}

inline int SelectedInfoClass::GetMaxCameo() const
{
	return this->MaxCameo;
}

void SelectedInfoClass::DrawInfo()
{
	if (!Phobos::Config::SelectedDisplay_Enable)
		return;

	if (this->ShouldUpdate)
		this->UpdateSelected();

	if (ObjectClass::CurrentObjects->Count > 0)
	{
		if (this->SingleSelect)
		{
			if (const auto& pButton = this->MainColumn)
				pButton->DrawInfo();

			if (const auto& pButton = this->PushButton)
				pButton->DrawInfo();

			if (const auto& pButton = this->AmmoButton)
				pButton->DrawInfo();

			if (const auto& pButton = this->InfoIconA)
				pButton->DrawInfo();

			if (const auto& pButton = this->InfoIconD)
				pButton->DrawInfo();

			if (const auto& pButton = this->InfoIconS)
				pButton->DrawInfo();
		}
		else
		{
			for (int i = 0; i < this->GetMaxCameo(); ++i)
			{
				if (const auto& pButton = this->Cameos[i])
					pButton->DrawInfo();
			}
		}
	}

	if (const auto& pButton = this->MainBottom)
		pButton->DrawInfo();
}

void SelectedInfoClass::ScrollLeft()
{
	if (this->Current > 0)
		--this->Current;
}

void SelectedInfoClass::ScrollRight()
{
	const int overflow = this->CurrentSelectCameo.size() - this->GetMaxCameo();

	if (overflow <= 0)
		this->Current = 0;
	else if (this->Current < overflow)
		++this->Current;
}

// ----------------------------------------

DEFINE_HOOK_AGAIN(0x5F4718, ObjectClass_Select, 0x7)
DEFINE_HOOK(0x5F46AE, ObjectClass_Select, 0x7)
{
	GET(ObjectClass*, pThis, ESI);

	pThis->IsSelected = true;

	if (Phobos::Config::ShowFlashOnSelecting && RulesExt::Global()->SelectionFlashDuration > 0 && pThis->GetOwningHouse()->IsControlledByCurrentPlayer())
		pThis->Flash(RulesExt::Global()->SelectionFlashDuration);

	SelectedInfoClass::Instance.ShouldUpdate = true;

	return 0;
}

DEFINE_HOOK(0x5F44FC, ObjectClass_Deselect, 0x7)
{
	GET(ObjectClass*, pThis, ESI);

	pThis->IsSelected = false;

	SelectedInfoClass::Instance.ShouldUpdate = true;

	return 0;
}
