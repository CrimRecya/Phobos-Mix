#include "Body.h"

#include <AircraftClass.h>
#include <HouseClass.h>
#include <ScenarioClass.h>
#include <SpawnManagerClass.h>
#include <SlaveManagerClass.h>
#include <ParasiteClass.h>
#include <TemporalClass.h>
#include <AirstrikeClass.h>
#include <BombListClass.h>
#include <TacticalClass.h>
#include <OverlayTypeClass.h>

#include <Ext/Anim/Body.h>
#include <Ext/BulletType/Body.h>
#include <Ext/House/Body.h>
#include <Ext/Scenario/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/House/Body.h>
#include <Ext/Cell/Body.h>

#include <Utilities/AresFunctions.h>

TechnoExt::ExtContainer TechnoExt::ExtMap;
UnitClass* TechnoExt::Deployer = nullptr;

TechnoExt::ExtData::~ExtData()
{
	auto const pTypeExt = this->TypeExtData;
	auto const pType = pTypeExt->OwnerObject();
	auto const pThis = this->OwnerObject();
	// Besides BuildingClass, calling pThis->WhatAmI() here will only result in AbstractType::None
	auto const whatAmI = pType->WhatAmI();

	if (whatAmI == AbstractType::UnitType)
	{
		auto invalidateCellPointer = [pThis, pType](CellClass* pCell, bool last)
			{
				auto const pCellExt = CellExt::ExtMap.Find(pCell);

				if (pCellExt->IncomingUnitAlt == pThis)
				{
					pCellExt->IncomingUnitAlt = nullptr;
					pCell->AltOccupationFlags &= ~0x20;
				}

				if (pCellExt->IncomingUnit == pThis)
				{
					pCellExt->IncomingUnit = nullptr;
					pCell->OccupationFlags &= ~0x20;
				}
			};

		if (auto const pCell = this->LastOccupationCell)
			invalidateCellPointer(pCell, true);

		if (auto const pCell = this->ThisOccupationCell)
			invalidateCellPointer(pCell, false);
	}

	if (const auto pSquad = this->SquadManager)
	{
		pSquad->RemoveMember(pThis);

		if (pSquad->Members.empty())
			SquadManagerClass::Remove(pSquad);
	}

	if (!this->ChildAttachments.empty())
	{
		for (auto const& pAttachment : this->ChildAttachments)
			pAttachment->Destroy(nullptr);

		this->ChildAttachments.clear();
	}

	this->MyTrackingLasers.clear();

	if (this->UndergroundTracked)
		ScenarioExt::Global()->UndergroundTracker.Remove(pThis);

	if (this->SpecialTracked)
		ScenarioExt::Global()->SpecialTracker.Remove(pThis);

	if (pTypeExt->AutoDeath_Behavior.isset())
	{
		auto& vec = ScenarioExt::Global()->AutoDeathObjects;
		vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
	}

	if (RulesExt::Global()->ExtendedBuildingPlacing && whatAmI == AbstractType::UnitType && pType->DeploysInto)
	{
		auto& vec = HouseExt::ExtMap.Find(pThis->Owner)->OwnedDeployingUnits;
		vec.erase(std::remove(vec.begin(), vec.end(), pThis), vec.end());
	}

	if (RulesExt::Global()->CheckExtraBaseNormal && pTypeExt->ExtraBaseNormal)
	{
		auto& vec = ScenarioExt::Global()->BaseNormalTechnos;
		vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
	}

	if (pTypeExt->UniqueTechno)
	{
		auto& vec = ScenarioExt::Global()->OwnedUniqueTechnos;
		vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
	}

	if (whatAmI != AbstractType::AircraftType && whatAmI != AbstractType::BuildingType
		&& pType->Ammo > 0 && pTypeExt->ReloadInTransport)
	{
		auto& vec = ScenarioExt::Global()->TransportReloaders;
		vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
	}

	if (this->AnimRefCount > 0)
		AnimExt::InvalidateTechnoPointers(pThis);

	if (this->TypeExtData->Harvester_Counted)
	{
		auto& vec = HouseExt::ExtMap.Find(pThis->Owner)->OwnedCountedHarvesters;
		vec.erase(std::remove(vec.begin(), vec.end(), pThis), vec.end());
	}

	for (auto const pBolt : this->ElectricBolts)
	{
		pBolt->Owner = nullptr;
	}

	this->ElectricBolts.clear();
}

bool TechnoExt::IsActiveIgnoreEMP(TechnoClass* pThis)
{
	return pThis
		&& pThis->IsAlive
		&& pThis->Health > 0
		&& !pThis->InLimbo
		&& !pThis->TemporalTargetingMe
		&& !pThis->BeingWarpedOut
		;
}

bool TechnoExt::IsActive(TechnoClass* pThis)
{
	return TechnoExt::IsActiveIgnoreEMP(pThis)
		&& !pThis->Deactivated
		&& !pThis->IsUnderEMP()
		;
}

bool TechnoExt::IsHarvesting(TechnoClass* pThis)
{
	if (!TechnoExt::IsActive(pThis))
		return false;

	auto const pSlaveManager = pThis->SlaveManager;

	if (pSlaveManager && pSlaveManager->State != SlaveManagerStatus::Ready)
		return true;

	if (pThis->WhatAmI() == AbstractType::Building)
		return pThis->IsPowerOnline();

	if (TechnoExt::HasAvailableDock(pThis))
	{
		switch (pThis->GetCurrentMission())
		{
		case Mission::Harvest:
			if (auto const pUnit = abstract_cast<UnitClass*, true>(pThis))
			{
				if (pUnit->HasAnyLink() && !TechnoExt::HasRadioLinkWithDock(pUnit)) // Probably still in factory.
					return false;

				if (pUnit->IsUseless) // Harvesters currently sitting without purpose are idle even if they are on harvest mission.
					return false;
			}
			return true;
		case Mission::Unload:
			return true;
		case Mission::Enter:
			if (pThis->HasAnyLink())
			{
				auto const pLink = pThis->GetNthLink(0);

				if (pLink->WhatAmI() != AbstractType::Building) // Enter mission + non-building link = not trying to unload
					return false;
			}
			return true;
		case Mission::Guard:
			if (auto pUnit = abstract_cast<UnitClass*, true>(pThis))
			{
				if (pUnit->ArchiveTarget && pUnit->GetStoragePercentage() > 0.0 && pUnit->Locomotor->Is_Moving()) // Edge-case, waiting to be able to unload.
					return true;
			}
			return false;
		default:
			return false;
		}
	}

	return false;
}

bool TechnoExt::HasAvailableDock(TechnoClass* pThis)
{
	for (auto pBld : pThis->GetTechnoType()->Dock)
	{
		if (pThis->Owner->CountOwnedAndPresent(pBld))
			return true;
	}

	return false;
}

bool TechnoExt::HasRadioLinkWithDock(TechnoClass* pThis)
{
	if (pThis->HasAnyLink())
	{
		auto const pLink = abstract_cast<BuildingClass*>(pThis->GetNthLink(0));

		if (pLink && pThis->GetTechnoType()->Dock.FindItemIndex(pLink->Type) >= 0)
			return true;
	}

	return false;
}

// Syncs Iron Curtain or Force Shield timer to another techno.
void TechnoExt::SyncInvulnerability(TechnoClass* pFrom, TechnoClass* pTo)
{
	if (pFrom->IsIronCurtained())
	{
		const auto pTypeExt = TechnoExt::ExtMap.Find(pFrom)->TypeExtData;
		const bool isForceShielded = pFrom->ForceShielded;
		const bool allowSyncing = !isForceShielded
			? pTypeExt->IronCurtain_KeptOnDeploy.Get(RulesExt::Global()->IronCurtain_KeptOnDeploy)
			: pTypeExt->ForceShield_KeptOnDeploy.Get(RulesExt::Global()->ForceShield_KeptOnDeploy);

		if (allowSyncing)
		{
			pTo->IronCurtainTimer = pFrom->IronCurtainTimer;
			pTo->IronTintStage = pFrom->IronTintStage;
			pTo->ForceShielded = isForceShielded;
		}
	}
}

double TechnoExt::GetCurrentSpeedMultiplier(FootClass* pThis)
{
	double houseMultiplier = 1.0;
	auto const whatAmI = pThis->WhatAmI();

	if (whatAmI == AbstractType::Aircraft)
		houseMultiplier = pThis->Owner->Type->SpeedAircraftMult;
	else if (whatAmI == AbstractType::Infantry)
		houseMultiplier = pThis->Owner->Type->SpeedInfantryMult;
	else
		houseMultiplier = pThis->Owner->Type->SpeedUnitsMult;

	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	return pThis->SpeedMultiplier * houseMultiplier * pExt->AE.SpeedMultiplier *
		(pThis->HasAbility(Ability::Faster) ? RulesClass::Instance->VeteranSpeed : 1.0);
}

CoordStruct TechnoExt::PassengerKickOutLocation(TechnoClass* pThis, FootClass* pPassenger, int maxAttempts = 1)
{
	if (!pThis || !pPassenger)
		return CoordStruct::Empty;

	if (maxAttempts < 1)
		maxAttempts = 1;

	const auto pTypePassenger = pPassenger->GetTechnoType();
	auto placeCoords = CellStruct::Empty;
	short extraDistance = 1;
	auto speedType = pTypePassenger->SpeedType;
	auto movementZone = pTypePassenger->MovementZone;

	if (pTypePassenger->WhatAmI() == AbstractType::AircraftType)
	{
		speedType = SpeedType::Track;
		movementZone = MovementZone::Normal;
	}
	do
	{
		placeCoords = pThis->GetMapCoords() - CellStruct { static_cast<short>(extraDistance / 2), static_cast<short>(extraDistance / 2) };
		placeCoords = MapClass::Instance.NearByLocation(placeCoords, speedType, -1, movementZone, false, extraDistance, extraDistance, true, false, false, false, CellStruct::Empty, false, false);

		if (placeCoords == CellStruct::Empty)
			return CoordStruct::Empty;

		const auto pCell = MapClass::Instance.GetCellAt(placeCoords);

		if (pThis->IsCellOccupied(pCell, FacingType::None, -1, nullptr, false) == Move::OK)
			break;

		extraDistance++;
	}
	while (extraDistance <= maxAttempts);

	if (const auto pCell = MapClass::Instance.TryGetCellAt(placeCoords))
		return pCell->GetCoordsWithBridge();

	return CoordStruct::Empty;
}

bool TechnoExt::AllowedTargetByZone(TechnoClass* pThis, TechnoClass* pTarget, TargetZoneScanType zoneScanType, WeaponTypeClass* pWeapon, bool useZone, int zone)
{
	if (!pThis || !pTarget)
		return false;

	if (pThis->WhatAmI() == AbstractType::Aircraft)
		return true;

	auto const pType = pThis->GetTechnoType();
	auto const mZone = pType->MovementZone;
	int currentZone = useZone ? zone : MapClass::Instance.GetMovementZoneType(pThis->GetMapCoords(), mZone, pThis->OnBridge);

	if (currentZone != -1)
	{
		if (zoneScanType == TargetZoneScanType::Any)
			return true;

		int targetZone = MapClass::Instance.GetMovementZoneType(pTarget->GetMapCoords(), mZone, pTarget->OnBridge);

		if (zoneScanType == TargetZoneScanType::Same)
		{
			if (currentZone != targetZone)
				return false;
		}
		else
		{
			if (currentZone == targetZone)
				return true;

			auto const speedType = pType->SpeedType;
			auto const cellStruct = MapClass::Instance.NearByLocation(CellClass::Coord2Cell(pTarget->Location),
				speedType, -1, mZone, false, 1, 1, true,
				false, false, speedType != SpeedType::Float, CellStruct::Empty, false, false);

			if (cellStruct == CellStruct::Empty)
				return false;

			auto const pCell = MapClass::Instance.TryGetCellAt(cellStruct);

			if (!pCell)
				return false;

			if (!pWeapon)
			{
				int weaponIndex = pThis->SelectWeapon(pTarget);

				if (weaponIndex < 0)
					return false;

				pWeapon = pThis->GetWeapon(weaponIndex)->WeaponType;
			}

			const double distance = pCell->GetCoordsWithBridge().DistanceFrom(pTarget->GetCenterCoords());

			if (distance > pWeapon->Range)
				return false;
		}
	}

	return true;
}

bool ConvertToType_Foot(FootClass* pThis, TechnoTypeClass* pToType)
{
	if (AresFunctions::ConvertTypeTo)
	{
		if (AresFunctions::ConvertTypeTo(pThis, pToType))
		{
			auto const pExt = TechnoExt::ExtMap.Find(pThis);
			pExt->UpdateTypeData(pToType);
			pExt->UpdateTypeData_Foot();
			return true;
		}

		return false;
	}

	// In case not using Ares 3.0. Only update necessary vanilla properties

	AbstractType rtti;
	TechnoTypeClass** nowTypePtr;

	// Different types prohibited
	switch (pThis->WhatAmI())
	{
	case AbstractType::Infantry:
		nowTypePtr = reinterpret_cast<TechnoTypeClass**>(&(static_cast<InfantryClass*>(pThis)->Type));
		rtti = AbstractType::InfantryType;
		break;
	case AbstractType::Unit:
		nowTypePtr = reinterpret_cast<TechnoTypeClass**>(&(static_cast<UnitClass*>(pThis)->Type));
		rtti = AbstractType::UnitType;
		break;
	case AbstractType::Aircraft:
		nowTypePtr = reinterpret_cast<TechnoTypeClass**>(&(static_cast<AircraftClass*>(pThis)->Type));
		rtti = AbstractType::AircraftType;
		break;
	default:
		Debug::Log("%s is not FootClass, conversion not allowed\n", pToType->get_ID());
		return false;
	}

	// Detach CLEG targeting
	auto const tempUsing = pThis->TemporalImUsing;
	if (tempUsing && tempUsing->Target)
		tempUsing->LetGo();

	auto const pOwner = pThis->Owner;

	// Remove tracking of old techno
	if (!pThis->InLimbo)
		pOwner->RegisterLoss(pThis, false);
	pOwner->RemoveTracking(pThis);

	const int oldHealth = pThis->Health;

	// Generic type-conversion
	auto const prevType = *nowTypePtr;
	*nowTypePtr = pToType;

	// Readjust health according to percentage
	pThis->SetHealthPercentage((double)(oldHealth) / (double)prevType->Strength);
	pThis->EstimatedHealth = pThis->Health;

	// Add tracking of new techno
	pOwner->AddTracking(pThis);
	if (!pThis->InLimbo)
		pOwner->RegisterGain(pThis, false);
	pOwner->RecheckTechTree = true;

	// Update Ares AttachEffects -- skipped
	// Ares RecalculateStats -- skipped

	// Adjust ammo
	pThis->Ammo = Math::min(pThis->Ammo, pToType->Ammo);
	// Ares ResetSpotlights -- skipped

	// Adjust ROT
	if (rtti == AbstractType::AircraftType)
		pThis->SecondaryFacing.SetROT(pToType->ROT);
	else
		pThis->PrimaryFacing.SetROT(pToType->ROT);
	// Adjust Ares TurretROT -- skipped
	//  pThis->SecondaryFacing.SetROT(TechnoTypeExt::ExtMap.Find(pToType)->TurretROT.Get(pToType->ROT));

	// Locomotor change, referenced from Ares 0.A's abduction code, not sure if correct, untested
	CLSID nowLocoID;
	ILocomotion* iloco = pThis->Locomotor;
	const auto& toLoco = pToType->Locomotor;
	if ((SUCCEEDED(static_cast<LocomotionClass*>(iloco)->GetClassID(&nowLocoID)) && nowLocoID != toLoco))
	{
		// because we are throwing away the locomotor in a split second, piggybacking
		// has to be stopped. otherwise the object might remain in a weird state.
		while (LocomotionClass::End_Piggyback(pThis->Locomotor));
		// throw away the current locomotor and instantiate
		// a new one of the default type for this unit.
		if (auto const newLoco = LocomotionClass::CreateInstance(toLoco))
		{
			newLoco->Link_To_Object(pThis);
			pThis->Locomotor = std::move(newLoco);
		}
	}

	const auto& jjLoco = LocomotionClass::CLSIDs::Jumpjet;
	if (pToType->BalloonHover && pToType->DeployToLand && prevType->Locomotor != jjLoco && toLoco == jjLoco)
		pThis->Locomotor->Move_To(pThis->Location);

	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	pExt->UpdateTypeData(pToType);
	pExt->UpdateTypeData_Foot();
	return true;
}

// Feature for common usage : TechnoType conversion -- Trsdy
// BTW, who said it was merely a Type pointer replacement and he could make a better one than Ares?
bool TechnoExt::ConvertToType(TechnoClass* pThis, TechnoTypeClass* pToType)
{
	const auto pPrevType = pThis->GetTechnoType();

	// Different types prohibited
	if (pPrevType->WhatAmI() != pToType->WhatAmI())
	{
		Debug::Log("Incompatible types between %s and %s\n", pPrevType->get_ID(), pToType->get_ID());
		return false;
	}

	if (const auto pFoot = abstract_cast<FootClass*, true>(pThis))
		return ConvertToType_Foot(pFoot, pToType);

	if (pPrevType->GapGenerator)
		pThis->DestroyGap();

	if (pToType->GapGenerator)
	{
		const auto temp = pPrevType->GapRadiusInCells;
		pThis->GapRadius = pToType->GapRadiusInCells;
		pPrevType->GapRadiusInCells = pToType->GapRadiusInCells;
		pThis->CreateGap();
		pPrevType->GapRadiusInCells = temp;
	}

	const auto pBuilding = static_cast<BuildingClass*>(pThis);
	const auto pToBuildingType = static_cast<BuildingTypeClass*>(pToType);
	const auto pPrevBuildingType = static_cast<BuildingTypeClass*>(pPrevType);

	// Maybe buggy
	for (int i = 0; i < 21; ++i)
	{
		if (const auto pAnim = pBuilding->Anims[i])
			GameDelete(pAnim);
	}

	// Skip audio related

	// Maybe buggy
	pBuilding->SetLinkCount(std::max(pToBuildingType->NumberOfDocks, 1));

	if (pToBuildingType->LoadBuildup())
		pBuilding->HasBuildUp = true;
	else
		pBuilding->AI_Sellable = false;

	// Skip SecretLab related

	// Same as foot

	const auto tempUsing = pThis->TemporalImUsing;

	if (tempUsing && tempUsing->Target)
		tempUsing->LetGo();

	const auto pOwner = pThis->Owner;
	pOwner->RemoveTracking(pThis);

	// Maybe buggy
	const auto coord = pBuilding->Location;

	pBuilding->Limbo();
	pBuilding->ActuallyPlacedOnMap = false;

	pBuilding->Type = pToBuildingType;
	TechnoExt::ExtMap.Find(pThis)->UpdateTypeData(pToType);

	pThis->SetHealthPercentage(static_cast<double>(pThis->Health) / pPrevBuildingType->Strength);
	pThis->EstimatedHealth = pThis->Health;

	pThis->Ammo = Math::min(pThis->Ammo, pToType->Ammo);

	pThis->SecondaryFacing.SetROT(pToType->ROT);
	pThis->PrimaryFacing.SetROT(pToType->ROT);

	if (pPrevBuildingType->Turret != pToBuildingType->Turret)
		pThis->PrimaryFacing.SetCurrent(DirStruct(0));

	pBuilding->unknown_coord_64C = CoordStruct::Empty;
	pOwner->RecheckTechTree = true;

	// ++Unsorted::ScenarioInit;

	if (!pBuilding->Unlimbo(coord, DirType::North))
	{
		pBuilding->UnInit();

		Debug::Log("Failed to place %s with new building type %s, its place has already been occupied.\n", pPrevType->get_ID(), pToType->get_ID());
		return false;
	}

	// --Unsorted::ScenarioInit;

	pBuilding->Place(false);
	pOwner->AddTracking(pThis);

	return true;
}

// Checks if vehicle can deploy into a building at its current location. If unit has no DeploysInto set returns noDeploysIntoDefaultValue (def = false) instead.
bool TechnoExt::CanDeployIntoBuilding(UnitClass* pThis, bool noDeploysIntoDefaultValue)
{
	if (!pThis)
		return false;

	auto const pDeployType = pThis->Type->DeploysInto;

	if (!pDeployType)
		return noDeploysIntoDefaultValue;

	auto mapCoords = CellClass::Coord2Cell(pThis->GetCoords());

	if (pDeployType->GetFoundationWidth() > 2 || pDeployType->GetFoundationHeight(false) > 2)
		mapCoords += CellStruct { -1, -1 };

	// The vanilla game used an inappropriate approach here, resulting in potential risk of desync.
	// Now, through additional checks, we can directly exclude the unit who want to deploy.
	TechnoExt::Deployer = pThis;
	const bool canDeploy = pDeployType->CanCreateHere(mapCoords, pThis->Owner);
	TechnoExt::Deployer = nullptr;

	return canDeploy;
}

bool TechnoExt::IsTypeImmune(TechnoClass* pThis, TechnoClass* pSource)
{
	if (!pThis || !pSource)
		return false;

	auto const pType = pThis->GetTechnoType();

	if (!pType->TypeImmune)
		return false;

	if (pType == pSource->GetTechnoType() && pThis->Owner == pSource->Owner)
		return true;

	return false;
}

/// <summary>
/// Gets whether or not techno has listed AttachEffect types active on it
/// </summary>
/// <param name="attachEffectTypes">Attacheffect types.</param>
/// <param name="requireAll">Whether or not to require all listed types to be present or if only one will satisfy the check.</param>
/// <param name="ignoreSameSource">Ignore AttachEffects that come from set invoker and source.</param>
/// <param name="pInvoker">Invoker Techno used for same source check.</param>
/// <param name="pSource">Source AbstractClass instance used for same source check.</param>
/// <returns>True if techno has active AttachEffects that satisfy the source, false if not.</returns>
bool TechnoExt::ExtData::HasAttachedEffects(std::vector<AttachEffectTypeClass*> attachEffectTypes, bool requireAll, bool ignoreSameSource,
	TechnoClass* pInvoker, AbstractClass* pSource, std::vector<int> const* minCounts, std::vector<int> const* maxCounts) const
{
	unsigned int foundCount = 0;
	unsigned int typeCounter = 1;

	for (auto const& type : attachEffectTypes)
	{
		for (auto const& attachEffect : this->AttachedEffects)
		{
			if (attachEffect->GetType() == type && attachEffect->IsActive())
			{
				if (ignoreSameSource && pInvoker && pSource && attachEffect->IsFromSource(pInvoker, pSource))
					continue;

				unsigned int minSize = minCounts ? minCounts->size() : 0;
				unsigned int maxSize = maxCounts ? maxCounts->size() : 0;

				if (type->Cumulative && (minSize > 0 || maxSize > 0))
				{
					int cumulativeCount = this->GetAttachedEffectCumulativeCount(type, ignoreSameSource, pInvoker, pSource);

					if (minSize > 0)
					{
						if (cumulativeCount < minCounts->at(typeCounter - 1 >= minSize ? minSize - 1 : typeCounter - 1))
							continue;
					}
					if (maxSize > 0)
					{
						if (cumulativeCount > maxCounts->at(typeCounter - 1 >= maxSize ? maxSize - 1 : typeCounter - 1))
							continue;
					}
				}

				// Only need to find one match, can stop here.
				if (!requireAll)
					return true;

				foundCount++;
				break;
			}
		}

		// One of the required types was not found, can stop here.
		if (requireAll && foundCount < typeCounter)
			return false;

		typeCounter++;
	}

	if (requireAll && foundCount == attachEffectTypes.size())
		return true;

	return false;
}

/// <summary>
/// Gets how many counts of same cumulative AttachEffect type instance techno has active on it.
/// </summary>
/// <param name="pAttachEffectType">AttachEffect type.</param>
/// <param name="ignoreSameSource">Ignore AttachEffects that come from set invoker and source.</param>
/// <param name="pInvoker">Invoker Techno used for same source check.</param>
/// <param name="pSource">Source AbstractClass instance used for same source check.</param>
/// <returns>Number of active cumulative AttachEffect type instances on the techno. 0 if the AttachEffect type is not cumulative.</returns>
int TechnoExt::ExtData::GetAttachedEffectCumulativeCount(AttachEffectTypeClass* pAttachEffectType, bool ignoreSameSource, TechnoClass* pInvoker, AbstractClass* pSource) const
{
	if (!pAttachEffectType->Cumulative)
		return 0;

	unsigned int foundCount = 0;
	const bool checkSource = ignoreSameSource && pInvoker && pSource;

	for (auto const& attachEffect : this->AttachedEffects)
	{
		if (attachEffect->GetType() == pAttachEffectType && attachEffect->IsActive())
		{
			if (checkSource && attachEffect->IsFromSource(pInvoker, pSource))
				continue;

			foundCount++;
		}
	}

	return foundCount;
}

UnitTypeClass* TechnoExt::ExtData::GetUnitTypeExtra() const
{
	if (auto const pUnit = abstract_cast<UnitClass*, true>(this->OwnerObject()))
	{
		if (pUnit->IsRedHP())
		{
			auto const pData = TechnoTypeExt::ExtMap.Find(pUnit->Type);

			if (pUnit->GetCell()->LandType == LandType::Water && !pUnit->OnBridge)
			{
				if (auto const imageRed = pData->WaterImage_ConditionRed)
					return imageRed;
				else if (auto const imageYellow = pData->WaterImage_ConditionYellow)
					return imageYellow;
			}
			else if (auto const imageRed = pData->Image_ConditionRed)
			{
				return imageRed;
			}
			else if (auto const imageYellow = pData->Image_ConditionYellow)
			{
				return imageYellow;
			}
		}
		else if (pUnit->IsYellowHP())
		{
			auto const pData = TechnoTypeExt::ExtMap.Find(pUnit->Type);

			if (pUnit->GetCell()->LandType == LandType::Water && !pUnit->OnBridge)
			{
				if (auto const imageYellow = pData->WaterImage_ConditionYellow)
					return imageYellow;
			}
			else if (auto const imageYellow = pData->Image_ConditionYellow)
			{
				return imageYellow;
			}
		}
	}

	return nullptr;
}

void TechnoExt::ExtData::InitAggressiveStance()
{
	this->AggressiveStance = this->TypeExtData->AggressiveStance.Get();
}

bool TechnoExt::ExtData::GetAggressiveStance() const
{
	// If this is a passenger then obey the configuration of the transport
	if (auto pTransport = this->OwnerObject()->Transporter)
		return TechnoExt::ExtMap.Find(pTransport)->GetAggressiveStance();

	// If this is a child then obey the configuration of the parent
	if (const auto pAttachment = this->ParentAttachment)
		return TechnoExt::ExtMap.Find(pAttachment->Parent)->GetAggressiveStance();

	return this->AggressiveStance;
}

void TechnoExt::ExtData::ToggleAggressiveStance()
{
	this->AggressiveStance = !this->AggressiveStance;
	const auto pThis = this->OwnerObject();

	if (!this->AggressiveStance)
	{
		pThis->QueueVoice(this->TypeExtData->VoiceExitAggressiveStance.Get());
		pThis->QueueMission(Mission::Guard, false);
		pThis->SetTarget(nullptr);
	}
	else
	{
		const auto pTechnoType = this->TypeExtData->OwnerObject();
		int voiceIndex = this->TypeExtData->VoiceEnterAggressiveStance.Get();

		if (voiceIndex < 0)
		{
			const auto& voiceList = pTechnoType->VoiceAttack.Count ? pTechnoType->VoiceAttack : pTechnoType->VoiceMove;

			if (const auto count = voiceList.Count)
				voiceIndex = voiceList.GetItem(Randomizer::Global.Random() % count);
		}

		pThis->QueueVoice(voiceIndex);
	}
}

bool TechnoExt::ExtData::CanToggleAggressiveStance()
{
	if (!RulesExt::Global()->EnableAggressiveStance)
		return false;

	const auto pTypeExt = this->TypeExtData;

	if (!pTypeExt->AggressiveStance_Togglable.isset())
	{
		const auto pType = pTypeExt->OwnerObject();

		// Only techno that are armed and open-topped can be aggressive stance.
		if (!this->OwnerObject()->IsArmed() && !pType->OpenTopped)
		{
			pTypeExt->AggressiveStance_Togglable = false;
			return false;
		}

		const auto absType = pType->WhatAmI();

		// Engineers and Agents are default to not allow aggressive stance.
		if (absType == AbstractType::InfantryType)
		{
			const auto pInfantryType = static_cast<InfantryTypeClass*>(pType);

			if (pInfantryType->Engineer || pInfantryType->Agent)
			{
				pTypeExt->AggressiveStance_Togglable = false;
				return false;
			}
		}
		else if (absType == AbstractType::BuildingType)
		{
			const auto pBuildingType = static_cast<BuildingTypeClass*>(pType);

			if (pBuildingType->EMPulseCannon)
			{
				pTypeExt->AggressiveStance_Togglable = false;
				return false;
			}
		}

		pTypeExt->AggressiveStance_Togglable = true;
		return true;
	}

	return pTypeExt->AggressiveStance_Togglable.Get(true);
}

void TechnoExt::ExtData::InitCeaseFireStance()
{
	this->CeaseFireStance = this->TypeExtData->CeaseFireStance.Get();
}

bool TechnoExt::ExtData::GetCeaseFireStance() const
{
	// If this is a passenger then obey the configuration of the transport
	if (const auto pTransport = this->OwnerObject()->Transporter)
		return TechnoExt::ExtMap.Find(pTransport)->GetCeaseFireStance();

	// If this is a child then obey the configuration of the parent
	if (const auto pAttachment = this->ParentAttachment)
		return TechnoExt::ExtMap.Find(pAttachment->Parent)->GetCeaseFireStance();

	return this->CeaseFireStance;
}

void TechnoExt::ExtData::ToggleCeaseFireStance()
{
	this->CeaseFireStance = !this->CeaseFireStance;
	const auto pThis = this->OwnerObject();
	const auto pTechnoType = this->TypeExtData->OwnerObject();
	int voiceIndex;

	if (!this->CeaseFireStance)
	{
		voiceIndex = this->TypeExtData->VoiceExitCeaseFireStance.Get();

		if (voiceIndex < 0)
		{
			const auto& voiceList = pTechnoType->VoiceAttack.Count ? pTechnoType->VoiceAttack : pTechnoType->VoiceMove;

			if (const auto count = voiceList.Count)
				voiceIndex = voiceList.GetItem(Randomizer::Global.Random() % count);
		}
	}
	else
	{
		pThis->SetTarget(nullptr);
		voiceIndex = this->TypeExtData->VoiceEnterCeaseFireStance.Get();

		if (voiceIndex < 0)
		{
			const auto& voiceList = pTechnoType->VoiceSelect.Count ? pTechnoType->VoiceSelect : pTechnoType->VoiceMove;

			if (const auto count = voiceList.Count)
				voiceIndex = voiceList.GetItem(Randomizer::Global.Random() % count);
		}
	}

	pThis->QueueVoice(voiceIndex);
}

bool TechnoExt::ExtData::CanToggleCeaseFireStance()
{
	if (!RulesExt::Global()->EnableCeaseFireStance)
		return false;

	const auto pTypeExt = this->TypeExtData;

	if (!pTypeExt->CeaseFireStance_Togglable.isset())
	{
		const auto pType = pTypeExt->OwnerObject();

		// Only techno that are armed and open-topped can be CeaseFire stance.
		if (!this->OwnerObject()->IsArmed() && !pType->OpenTopped)
		{
			pTypeExt->CeaseFireStance_Togglable = false;
			return false;
		}

		const auto absType = pType->WhatAmI();

		// Engineers and Agents are default to not allow CeaseFire stance.
		if (absType == AbstractType::InfantryType)
		{
			const auto pInfantryType = static_cast<InfantryTypeClass*>(pType);

			if (pInfantryType->Engineer || pInfantryType->Agent)
			{
				pTypeExt->CeaseFireStance_Togglable = false;
				return false;
			}
		}
		else if (absType == AbstractType::BuildingType)
		{
			const auto pBuildingType = static_cast<BuildingTypeClass*>(pType);

			if (pBuildingType->EMPulseCannon)
			{
				pTypeExt->CeaseFireStance_Togglable = false;
				return false;
			}
		}

		pTypeExt->CeaseFireStance_Togglable = true;
		return true;
	}

	return pTypeExt->CeaseFireStance_Togglable.Get(true);
}

void TechnoExt::ExtData::UpdateTrackingLasers()
{
	const auto pThis = this->OwnerObject();
	const auto pTarget = pThis->Target;

	if (pTarget && pTarget == this->MyTrackingLasersTarget)
	{
		const size_t size = this->MyTrackingLasers.size();
		const auto coords = pTarget->GetCoords();

		for (size_t i = 0; i < size; ++i)
		{
			const auto& data = this->MyTrackingLasers[i];
			// Refresh the laser state to keep it alive.
			data.Laser->Progress.Value = 0;
			// Change the end point.
			data.Laser->Target = coords;
			// Change the start point.
			const int burstIdx = pThis->CurrentBurstIndex;
			pThis->CurrentBurstIndex = data.BurstIdx;
			data.Laser->Source = pThis->GetFLH(data.WeaponIdx, CoordStruct::Empty);
			pThis->CurrentBurstIndex = burstIdx;
		}
	}
	else
	{
		// Stop tracking and delete all lasers if target changed.
		if (!this->MyTrackingLasers.empty())
			this->MyTrackingLasers.clear();

		this->MyTrackingLasersTarget = nullptr;
	}
}

// Attaches this techno in a first available attachment "slot".
// Returns true if the attachment is successful.
bool TechnoExt::AttachTo(TechnoClass* pThis, TechnoClass* pParent)
{
	auto const pParentExt = TechnoExt::ExtMap.Find(pParent);

	for (auto const& pAttachment : pParentExt->ChildAttachments)
	{
		if (pAttachment->AttachChild(pThis))
			return true;
	}

	return false;
}

bool TechnoExt::DetachFromParent(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	return pExt->ParentAttachment->DetachChild();
}

void TechnoExt::ExtData::InitializeAttachments()
{
	const auto& types = this->TypeExtData->AttachmentTypes;
	const auto pThis = this->OwnerObject();

	for (const auto& type : types)
	{
		this->ChildAttachments.emplace_back(std::make_unique<AttachmentClass>(type, pThis, nullptr));
		this->ChildAttachments.back()->Initialize();
	}
}

void TechnoExt::DestroyAttachments(TechnoClass* pThis, TechnoClass* pSource)
{
	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	if (!pExt)
		return;

	for (auto const& pAttachment : pExt->ChildAttachments)
		pAttachment->Destroy(pSource);

	// TODO I am not sure, without clearing the attachments it sometimes crashes under
	// weird circumstances, like if the techno exists but the parent attachment isn't,
	// in particular in can enter cell hook, this may be a bandaid fix for something
	// way worse like improper occupation clearance or whatever - Kerbiter
	pExt->ChildAttachments.clear();
}

void TechnoExt::HandleDestructionAsChild(TechnoClass* pThis)
{
	auto const& pExt = TechnoExt::ExtMap.Find(pThis);

	if (pExt->ParentAttachment)
		pExt->ParentAttachment->ChildDestroyed();
}

void TechnoExt::UnlimboAttachments(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	for (auto const& pAttachment : pExt->ChildAttachments)
		pAttachment->Unlimbo();
}

void TechnoExt::LimboAttachments(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	for (auto const& pAttachment : pExt->ChildAttachments)
		pAttachment->Limbo();
}

void TechnoExt::TransferAttachments(TechnoClass* pThis, TechnoClass* pThat)
{
	auto const pThisExt = TechnoExt::ExtMap.Find(pThis);
	auto const pThatExt = TechnoExt::ExtMap.Find(pThat);

	for (auto& pAttachment : pThisExt->ChildAttachments)
	{
		pAttachment->Parent = pThat;
		pThatExt->ChildAttachments.push_back(std::move(pAttachment));
	}

	pThisExt->ChildAttachments.clear();
}

bool TechnoExt::IsAttached(TechnoClass* pThis)
{
	auto const& pExt = TechnoExt::ExtMap.Find(pThis);
	return pExt && pExt->ParentAttachment;
}

bool TechnoExt::HasAttachmentLoco(FootClass* pThis)
{
	return locomotion_cast<AttachmentLocomotionClass*>(pThis->Locomotor) != nullptr;
}

bool TechnoExt::DoesntOccupyCellAsChild(TechnoClass* pThis)
{
	auto const& pExt = TechnoExt::ExtMap.Find(pThis);
	return pExt && pExt->ParentAttachment
		&& !pExt->ParentAttachment->GetType()->OccupiesCell;
}

bool TechnoExt::IsChildOf(TechnoClass* pThis, TechnoClass* pParent, bool deep)
{
	auto const pThisExt = TechnoExt::ExtMap.Find(pThis);

	return pThis && pThisExt && pParent  // sanity check, sometimes crashes because ext is null - Kerbiter
		&& pThisExt->ParentAttachment
		&& (pThisExt->ParentAttachment->Parent == pParent
			|| (deep && TechnoExt::IsChildOf(pThisExt->ParentAttachment->Parent, pParent)));
}

bool TechnoExt::AreRelatives(TechnoClass* pThis, TechnoClass* pThat)
{
	return TechnoExt::GetTopLevelParent(pThis)
		== TechnoExt::GetTopLevelParent(pThat);
}

// Returns this if no parent.
TechnoClass* TechnoExt::GetTopLevelParent(TechnoClass* pThis)
{
	auto const pThisExt = TechnoExt::ExtMap.Find(pThis);

	return pThis && pThisExt  // sanity check, sometimes crashes because ext is null - Kerbiter
		&& pThisExt->ParentAttachment
		? TechnoExt::GetTopLevelParent(pThisExt->ParentAttachment->Parent)
		: pThis;
}

bool TechnoExt::IsHealthInThreshold(TechnoClass* pObject, double min, double max)
{
	double hp = pObject->GetHealthPercentage();
	return hp <= max && hp >= min;
}

// =============================
// load / save

template <typename T>
void TechnoExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->TypeExtData)
		.Process(this->Shield)
		.Process(this->LaserTrails)
		.Process(this->AttachedEffects)
		.Process(this->AE)
		.Process(this->PreviousType)
		//.Process(this->ElectricBolts)
		.Process(this->AnimRefCount)
		.Process(this->SubterraneanHarvFreshFromFactory)
		.Process(this->SubterraneanHarvRallyDest)
		.Process(this->ReceiveDamage)
		.Process(this->LastKillWasTeamTarget)
		.Process(this->PassengerDeletionTimer)
		.Process(this->CurrentShieldType)
		.Process(this->LastWarpDistance)
		.Process(this->ChargeTurretTimer)
		.Process(this->AutoDeathTimer)
		.Process(this->MindControlRingAnimType)
		.Process(this->DamageNumberOffset)
		.Process(this->Strafe_BombsDroppedThisRound)
		.Process(this->CurrentAircraftWeaponIndex)
		.Process(this->IsInTunnel)
		.Process(this->IsBurrowed)
		.Process(this->HasBeenPlacedOnMap)
		.Process(this->DeployFireTimer)
		.Process(this->SkipTargetChangeResetSequence)
		.Process(this->ForceFullRearmDelay)
		.Process(this->LastRearmWasFullDelay)
		.Process(this->CanCloakDuringRearm)
		.Process(this->WHAnimRemainingCreationInterval)
		.Process(this->UnitIdleIsSelected)
		.Process(this->UnitIdleActionTimer)
		.Process(this->UnitIdleActionGapTimer)
		.Process(this->UnitAutoDeployTimer)
		.Process(this->LastWeaponType)
		.Process(this->LastWeaponFLH)
		.Process(this->TrajectoryGroup)
		.Process(this->ExtraTurretRecoil)
		.Process(this->ExtraBarrelRecoil)
		.Process(this->ScatteringStopFrame)
		.Process(this->MyTargetingFrame)
		.Process(this->AutoTargetedWallCell)
		.Process(this->HasCachedClickMission)
		.Process(this->CachedMission)
		.Process(this->CachedCell)
		.Process(this->CachedTarget)
		.Process(this->HasCachedClickEvent)
		.Process(this->CachedEventType)
		.Process(this->FiringObstacleCell)
		.Process(this->IsDetachingForCloak)
		.Process(this->BeControlledThreatFrame)
		.Process(this->LastHurtFrame)
		.Process(this->LastTargetID)
		.Process(this->AccumulatedGattlingValue)
		.Process(this->ShouldUpdateGattlingValue)
		.Process(this->OriginalPassengerOwner)
		.Process(this->HasRemainingWarpInDelay)
		.Process(this->LastWarpInDelay)
		.Process(this->IsBeingChronoSphered)
		.Process(this->AggressiveStance)
		.Process(this->CeaseFireStance)
		.Process(this->KeepTargetOnMove)
		.Process(this->LastSensorsMapCoords)
		.Process(this->IsWreckage)
		.Process(this->JumpjetFromAirport)
		.Process(this->BuildingOccupying)
		.Process(this->TiberiumEater_Timer)
		.Process(this->AirstrikeTargetingMe)
		//.Process(this->MyTrackingLasers)
		.Process(this->MyTrackingLasersTarget)
		.Process(this->SquadManager)
		.Process(this->ParentAttachment)
		.Process(this->ChildAttachments)
		.Process(this->ThisOccupationCell)
		.Process(this->LastOccupationCell)
		.Process(this->AltOccupation)
		.Process(this->FiringAnimationTimer)
		.Process(this->AttachedEffectInvokerCount)
		.Process(this->TintColorOwner)
		.Process(this->TintColorAllies)
		.Process(this->TintColorEnemies)
		.Process(this->TintIntensityOwner)
		.Process(this->TintIntensityAllies)
		.Process(this->TintIntensityEnemies)
		.Process(this->AttackMoveFollowerTempCount)
		.Process(this->UndergroundTracked)
		.Process(this->SpecialTracked)
		.Process(this->BulletsTargetingMe)
		;
}

void TechnoExt::ExtData::InvalidatePointer(void* ptr, bool bRemoved)
{
	AnnounceInvalidPointer(this->AirstrikeTargetingMe, ptr);
	AnnounceInvalidPointer(this->MyTrackingLasersTarget, ptr);

	for (auto const& pAttachment : ChildAttachments)
		pAttachment->InvalidatePointer(ptr);

	if (this->HasCachedClickMission && this->CachedTarget == ptr)
	{
		this->HasCachedClickMission = false;
		this->CachedMission = Mission::None;
		this->CachedCell = nullptr;
		this->CachedTarget = nullptr;
	}
}

void TechnoExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<TechnoClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TechnoExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<TechnoClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool TechnoExt::LoadGlobals(PhobosStreamReader& Stm)
{
	return Stm
		.Success();
}

bool TechnoExt::SaveGlobals(PhobosStreamWriter& Stm)
{
	return Stm
		.Success();
}

// =============================
// container

TechnoExt::ExtContainer::ExtContainer() : Container("TechnoClass") { }
TechnoExt::ExtContainer::~ExtContainer() = default;

bool TechnoExt::ExtContainer::InvalidateExtDataIgnorable(void* const ptr) const
{
	auto const abs = static_cast<AbstractClass*>(ptr)->WhatAmI();

	switch (abs)
	{
	case AbstractType::Airstrike:
	case AbstractType::Aircraft:
	case AbstractType::Building:
	case AbstractType::Infantry:
	case AbstractType::Unit:
	case AbstractType::Terrain:
	case AbstractType::Bullet:
		return false;
	}

	return true;
}

// =============================
// container hooks

DEFINE_HOOK(0x6F3260, TechnoClass_CTOR, 0x5)
{
	GET(TechnoClass*, pItem, ESI);

	TechnoExt::ExtMap.TryAllocate(pItem);

	return 0;
}

DEFINE_HOOK(0x6F4500, TechnoClass_DTOR, 0x5)
{
	GET(TechnoClass*, pItem, ECX);

	TechnoExt::ExtMap.Remove(pItem);

	return 0;
}

DEFINE_HOOK_AGAIN(0x70C250, TechnoClass_SaveLoad_Prefix, 0x8)
DEFINE_HOOK(0x70BF50, TechnoClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(TechnoClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	TechnoExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x70C249, TechnoClass_Load_Suffix, 0x5)
{
	TechnoExt::ExtMap.LoadStatic();

	return 0;
}

DEFINE_HOOK(0x70C264, TechnoClass_Save_Suffix, 0x5)
{
	TechnoExt::ExtMap.SaveStatic();

	return 0;
}

