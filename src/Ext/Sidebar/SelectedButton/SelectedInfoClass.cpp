#include "SelectedInfoClass.h"

#include <AircraftClass.h>
#include <FactoryClass.h>
#include <SpawnManagerClass.h>
#include <SuperClass.h>
#include <MessageListClass.h>

#include <Ext/Side/Body.h>
#include <Ext/BuildingType/Body.h>

SelectedInfoClass SelectedInfoClass::Instance;

void SelectedInfoClass::InitClear()
{
	if (this->MainColumn)
	{
		GScreenClass::Instance.RemoveButton(this->MainColumn);
		this->MainColumn = nullptr;
	}

	if (this->PushButton)
	{
		GScreenClass::Instance.RemoveButton(this->PushButton);
		this->PushButton = nullptr;
	}

	if (this->AmmoButton)
	{
		GScreenClass::Instance.RemoveButton(this->AmmoButton);
		this->AmmoButton = nullptr;
	}

	if (this->MainCameo)
	{
		GScreenClass::Instance.RemoveButton(this->MainCameo);
		this->MainCameo = nullptr;
	}

	if (this->InfoIconA)
	{
		GScreenClass::Instance.RemoveButton(this->InfoIconA);
		this->InfoIconA = nullptr;
	}

	if (this->InfoIconD)
	{
		GScreenClass::Instance.RemoveButton(this->InfoIconD);
		this->InfoIconD = nullptr;
	}

	if (this->InfoIconS)
	{
		GScreenClass::Instance.RemoveButton(this->InfoIconS);
		this->InfoIconS = nullptr;
	}

	if (this->MainBottom)
	{
		GScreenClass::Instance.RemoveButton(this->MainBottom);
		this->MainBottom = nullptr;
	}

	if (this->ToggleV)
	{
		GScreenClass::Instance.RemoveButton(this->ToggleV);
		this->ToggleV = nullptr;
	}

	if (this->ToggleE)
	{
		GScreenClass::Instance.RemoveButton(this->ToggleE);
		this->ToggleE = nullptr;
	}

	if (this->ScrollL)
	{
		GScreenClass::Instance.RemoveButton(this->ScrollL);
		this->ScrollL = nullptr;
	}

	if (this->ScrollR)
	{
		GScreenClass::Instance.RemoveButton(this->ScrollR);
		this->ScrollR = nullptr;
	}

	for (int i = 0; i < 20; ++i)
	{
		if (auto& pButton = this->Cameos[i])
		{
			GScreenClass::Instance.RemoveButton(pButton);
			pButton = nullptr;
		}
	}

	this->MaxCameo = 0;
	this->Current = 0;
	this->ShouldUpdate = false;
	this->SingleSelect = true;
	this->ObtainSelect = false;
	this->IsHovering = false;
}

void SelectedInfoClass::InitIO()
{
	if (Unsorted::ArmageddonMode)
		return;

	const auto pSideExt = SideExt::ExtMap.Find(SideClass::Array.Items[ScenarioClass::Instance->PlayerSideIndex]);
	const auto pBottomSHP = pSideExt->SelectedInfo_Bottom.Get();

	if (!pBottomSHP || pBottomSHP->Frames < 3)
		return;

	const auto bottom = DSurface::Composite->GetHeight() - 32;

	if (const auto pMainSHP = pSideExt->SelectedInfo_Main.Get())
	{
		{
			const auto pButton = GameCreate<SelectedColumnClass>(SelectedInfoClass::StartID, 0, bottom - 100, 203, 79);
			pButton->Zap();
			GScreenClass::Instance.AddButton(pButton);
			this->MainColumn = pButton;
		}

		if (const auto pButtonSHP = pSideExt->SelectedInfo_Button.Get())
		{
			if (pButtonSHP->Frames >= 7)
			{
				{
					const auto pButton = GameCreate<SelectedButtonClass>(SelectedInfoClass::StartID + 1, 197, bottom - 79);
					pButton->Zap();
					GScreenClass::Instance.AddButton(pButton);
					this->PushButton = pButton;
				}

				{
					const auto pButton = GameCreate<SelectedButtonClass>(SelectedInfoClass::StartID + 2, 197, bottom - 50);
					pButton->Zap();
					GScreenClass::Instance.AddButton(pButton);
					this->AmmoButton = pButton;
				}
			}
		}

		{
			const auto pButton = GameCreate<SelectedMainCameoClass>(SelectedInfoClass::StartID + 3, 6, bottom - 95);
			pButton->Zap();
			GScreenClass::Instance.AddButton(pButton);
			this->MainCameo = pButton;
		}

		if (const auto pBuffSHP = pSideExt->SelectedInfo_Buff.Get())
		{
			if (pBuffSHP->Frames >= 15)
			{
				{
					const auto pButton = GameCreate<SelectedNotButtonClass>(SelectedInfoClass::StartID + 4, 179, bottom - 93);
					pButton->Zap();
					GScreenClass::Instance.AddButton(pButton);
					this->InfoIconA = pButton;
				}

				{
					const auto pButton = GameCreate<SelectedNotButtonClass>(SelectedInfoClass::StartID + 5, 179, bottom - 77);
					pButton->Zap();
					GScreenClass::Instance.AddButton(pButton);
					this->InfoIconD = pButton;
				}

				{
					const auto pButton = GameCreate<SelectedNotButtonClass>(SelectedInfoClass::StartID + 6, 179, bottom - 61);
					pButton->Zap();
					GScreenClass::Instance.AddButton(pButton);
					this->InfoIconS = pButton;
				}
			}
		}
	}

	{
		const auto pButton = GameCreate<SelectedBottomClass>(SelectedInfoClass::StartID + 7, 0, bottom - 21, 289, 21);
		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->MainBottom = pButton;
	}

	{
		const auto pButton = GameCreate<SelectedToggleClass>(SelectedInfoClass::StartID + 8, Phobos::Config::SelectedDisplay_Enable ? 238 : 2, bottom - 17);
		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->ToggleV = pButton;
	}

	{
		const auto pButton = GameCreate<SelectedToggleClass>(SelectedInfoClass::StartID + 9, 250, bottom - 17);
		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->ToggleE = pButton;
	}

	{
		const auto pButton = GameCreate<SelectedScrollClass>(SelectedInfoClass::StartID + 30, 262, bottom - 17);
		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->ScrollL = pButton;
	}

	{
		const auto pButton = GameCreate<SelectedScrollClass>(SelectedInfoClass::StartID + 31, 274, bottom - 17);
		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->ScrollR = pButton;
	}

	Point2D position { 0, (bottom - 69) };

	for (int i = 0; i < 20; ++i)
	{
		const auto pButton = GameCreate<SelectedCameoClass>(SelectedInfoClass::StartID + 10 + i, position.X, position.Y);
		position.X += 60;

		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->Cameos[i] = pButton;
	}

	this->ShouldUpdate = true;
	this->MaxCameo = std::min(Phobos::Config::SelectedDisplay_MaxCameo, (DSurface::Composite->GetWidth() - 150) / 60);

	for (int i = this->GetMaxCameo(); i < 20; ++i)
	{
		if (const auto& pButton = this->Cameos[i])
			pButton->Disabled = true;
	}
}

void SelectedInfoClass::SwitchExpand()
{
	if (!Phobos::Config::SelectedDisplay_Enable)
		return;

	Phobos::Config::SelectedDisplay_Expand = !Phobos::Config::SelectedDisplay_Expand;
	VocClass::PlayGlobal(RulesClass::Instance->GUIMainButtonSound, 0x2000, 1.0);
	this->UpdateVisible();
	this->ShouldUpdate = true;
}

void SelectedInfoClass::SwitchVisible()
{
	Phobos::Config::SelectedDisplay_Enable = !Phobos::Config::SelectedDisplay_Enable;
	VocClass::PlayGlobal(RulesClass::Instance->GUIMainButtonSound, 0x2000, 1.0);
	this->UpdateVisible();

	if (Phobos::Config::SelectedDisplay_Enable)
		this->ShouldUpdate = true;
}

void SelectedInfoClass::UpdateVisible()
{
	const bool enable = Phobos::Config::SelectedDisplay_Enable;
	auto disabled = !enable || !this->SingleSelect || !this->ObtainSelect;

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

	disabled = !enable || this->SingleSelect;
	int size = this->CurrentSelectCameo.size();
	int cameoCount = 0;

	if (size == 1 || Phobos::Config::SelectedDisplay_Expand)
		size = this->CurrentSelectTechno.size();

	for (int i = 0; i < this->GetMaxCameo(); ++i)
	{
		if (const auto& pButton = this->Cameos[i])
			pButton->Disabled = disabled || (i >= size);
	}

	const int overflow = size - this->GetMaxCameo();

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
		pButton->Width = std::max((enable ? (this->SingleSelect ? 253 : 289) : 17), cameoCount * 60);

	if (const auto& pButton = this->ToggleV)
		pButton->X = enable ? 238 : 2;

	if (const auto& pButton = this->ToggleE)
		pButton->Disabled = disabled;

	if (const auto& pButton = this->ScrollL)
		pButton->Disabled = disabled;

	if (const auto& pButton = this->ScrollR)
		pButton->Disabled = disabled;
}

void SelectedInfoClass::UpdateSelected()
{
	this->ShouldUpdate = false;

	if (this->CurrentSelectCameo.size())
		this->CurrentSelectCameo.clear();

	if (this->CurrentSelectTechno.size())
		this->CurrentSelectTechno.clear();

	{
		std::map<int, SelectRecordStruct> CurrentSelectBuffer;

		for (const auto& pCurrent : ObjectClass::CurrentObjects)
		{
			if (const auto pType = pCurrent->GetTechnoType())
			{
				const auto count = CurrentSelectBuffer.contains(pType->UniqueID) ? CurrentSelectBuffer.at(pType->UniqueID).Count : 0;
				CurrentSelectBuffer[pType->UniqueID] = SelectRecordStruct { TechnoTypeExt::ExtMap.Find(pType), count + 1 };
				this->CurrentSelectTechno.emplace_back(TechnoExt::ExtMap.Find(static_cast<TechnoClass*>(pCurrent)));
			}
		}

		if (CurrentSelectBuffer.size())
		{
			for (const auto& [ID, Record] : CurrentSelectBuffer)
				this->UpdateRecordCameo(Record);
		}
	}

	std::sort(this->CurrentSelectTechno.begin(), this->CurrentSelectTechno.end(),
		[](const TechnoExt::ExtData* const pSelectA, const TechnoExt::ExtData* const pSelectB)
		{
			const auto uniqueA = pSelectA->TypeExtData->OwnerObject()->UniqueID;
			const auto uniqueB = pSelectB->TypeExtData->OwnerObject()->UniqueID;
			if (uniqueA < uniqueB) return true;
			if (uniqueA > uniqueB) return false;
			return pSelectA->OwnerObject()->UniqueID < pSelectB->OwnerObject()->UniqueID;
		});

	const auto size = this->CurrentSelectTechno.size();
	this->SingleSelect = size <= 1;
	this->ObtainSelect = size > 0;
	this->UpdateVisible();
}

void SelectedInfoClass::UpdateRecordCameo(const SelectRecordStruct& Record)
{
	const auto groupID = Record.TypeExt->GetSelectionGroupID();
	const int currentCounts = this->CurrentSelectCameo.size();

	for (int i = 0; i < currentCounts; ++i)
	{
		if (this->CurrentSelectCameo[i].TypeExt->GetSelectionGroupID() == groupID)
		{
			this->CurrentSelectCameo[i].Count += Record.Count;
			return;
		}
	}

	this->CurrentSelectCameo.push_back(Record);
}

void SelectedInfoClass::DrawInfo()
{
	const bool drawAll = Phobos::Config::SelectedDisplay_Enable;

	if (drawAll)
	{
		if (this->ShouldUpdate)
			this->UpdateSelected();

		if (this->ObtainSelect)
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
	}

	if (const auto& pButton = this->MainBottom)
		pButton->DrawInfo();

	if (const auto& pButton = this->ToggleV)
		pButton->DrawInfo();

	if (drawAll && !this->SingleSelect)
	{
		if (const auto& pButton = this->ToggleE)
			pButton->DrawInfo();

		if (const auto& pButton = this->ScrollL)
			pButton->DrawInfo();

		if (const auto& pButton = this->ScrollR)
			pButton->DrawInfo();
	}
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
			PCX::Instance.LoadFile(pFilename);

			if (const auto MissingCameoPCX = PCX::Instance.GetSurface(pFilename))
				return MissingCameoPCX;
		}
	}

	return nullptr;
}

void SelectedInfoClass::GetValuesForDisplay(TechnoClass* pThis, ObjectTypeClass* pFakeType, DisplayInfoType infoType, int& value, int& maxValue, int infoIndex)
{
	const auto pTrueType = pThis->GetTechnoType();

	if (pTrueType == pFakeType)
	{
		TechnoExt::GetValuesForDisplay(pThis, infoType, value, maxValue, infoIndex);
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
		TechnoExt::GetValuesForDisplay(pThis, infoType, value, maxValue, infoIndex);
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

	if ((!pOwner || !pOwner->IsAlliedWith(HouseClass::CurrentPlayer)) && !HouseClass::IsCurrentPlayerObserver())
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
		if (const auto pFoot = real_abstract_cast<FootClass*>(pThis))
		{
			if (pFoot->LocomotorSource)
				return TechnoStatus::Locomotor;

			if (pFoot->Locomotor->Is_Moving_Now())
				return TechnoStatus::Move;
		}

		return TechnoStatus::Guard;
	}

	if (const auto pBuilding = real_abstract_cast<BuildingClass*>(pThis))
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
	else if (const auto pFoot = real_abstract_cast<FootClass*>(pThis))
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

int SelectedInfoClass::GetMaxCameo() const
{
	return this->MaxCameo;
}

bool SelectedInfoClass::CanScrollLeft() const
{
	if (this->SingleSelect)
		return false;

	return this->Current > 0;
}

bool SelectedInfoClass::CanScrollRight() const
{
	if (this->SingleSelect)
		return false;

	int size = this->CurrentSelectCameo.size();

	if (size == 1 || Phobos::Config::SelectedDisplay_Expand)
		size = this->CurrentSelectTechno.size();

	const int overflow = size - this->GetMaxCameo();

	return this->Current < overflow;
}

bool SelectedInfoClass::ScrollLeft()
{
	if (this->CanScrollLeft())
	{
		--this->Current;
		return true;
	}

	return false;
}

bool SelectedInfoClass::ScrollRight()
{
	if (this->CanScrollRight())
	{
		++this->Current;
		return true;
	}

	return false;
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
