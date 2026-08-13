#include <Ext/Aircraft/Body.h>
#include <Ext/Anim/Body.h>
#include <Ext/Building/Body.h>
#include <Ext/BuildingType/Body.h>
#include <Ext/House/Body.h>
#include <Ext/Infantry/Body.h>
#include <Ext/Unit/Body.h>
#include <Ext/Scenario/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/Event/Body.h>

#include <Utilities/AresFunctions.h>
#include <Utilities/AresHelper.h>
#include <Interop/TechnoExt.h>

TechnoExt::~TechnoExt()
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
				auto const pCellExt = CellExt::Fetch(pCell);

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
			pAttachment->UnInit();

		this->ChildAttachments.clear();
	}

	if (pTypeExt->AutoDeath_Behavior.isset())
	{
		auto& vec = ScenarioExt::Global()->AutoDeathObjects;
		vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
	}

	if (RulesExt::Global()->ExtendedBuildingPlacing && whatAmI == AbstractType::UnitType && pType->DeploysInto)
	{
		auto& vec = HouseExt::Fetch(pThis->Owner)->OwnedDeployingUnits;
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
		&& pType->Ammo > 0 && pTypeExt->ReloadInTransport.Get(RulesExt::Global()->ReloadInTransport))
	{
		auto& vec = ScenarioExt::Global()->TransportReloaders;
		vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
	}

	if (this->IsSelected)
	{
		auto& vec = ScenarioExt::Global()->LimboLaunchers;
		vec.erase(std::remove(vec.begin(), vec.end(), this), vec.end());
	}

	if (this->AnimRefCount > 0)
		AnimExt::InvalidateTechnoPointers(pThis);

	if (pTypeExt->Harvester_Counted)
	{
		auto& vec = HouseExt::Fetch(pThis->Owner)->OwnedCountedHarvesters;
		vec.erase(std::remove(vec.begin(), vec.end(), pThis), vec.end());
	}

	for (auto const pBolt : this->ElectricBolts)
	{
		pBolt->Owner = nullptr;
	}

	this->ElectricBolts.clear();

	if (this->SpecialTracked)
		ScenarioExt::Global()->SpecialTracker.Remove(pThis);

	if (this->FallingDownTracked)
		ScenarioExt::Global()->FallingDownTracker.Remove(pThis);
}

bool TechnoExt::IsActive(TechnoClass* pThis)
{
	return pThis
		&& pThis->IsAlive
		&& pThis->Health > 0
		&& !pThis->InLimbo
		&& !pThis->TemporalTargetingMe
		&& !pThis->BeingWarpedOut
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
			if (auto const pUnit = abstract_cast<UnitClass*, true>(pThis))
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
	for (auto const pBld : pThis->GetTechnoType()->Dock)
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
		auto const pLink = abstract_cast<BuildingClass*, true>(pThis->GetNthLink(0));

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
		const auto pTypeExt = TechnoExt::Fetch(pFrom)->TypeExtData;
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

	return pThis->SpeedMultiplier * houseMultiplier * TechnoExt::Fetch(pThis)->AE.SpeedMultiplier *
		(pThis->HasAbility(Ability::Faster) ? RulesClass::Instance->VeteranSpeed : 1.0);
}

double TechnoExt::GetCurrentFirepowerMultiplier(TechnoClass* pThis)
{
	double mult = pThis->FirepowerMultiplier * pThis->Owner->FirepowerMultiplier * TechnoExt::Fetch(pThis)->AE.FirepowerMultiplier *
		(pThis->HasAbility(Ability::Firepower) ? RulesClass::Instance->VeteranCombat : 1.0);

	if (const auto pBuilding = abstract_cast<BuildingClass*, true>(pThis))
	{
		const auto pBuildingType = pBuilding->Type;

		if (pBuildingType->CanBeOccupied && pBuildingType->CanOccupyFire && pBuildingType->MaxNumberOccupants)
		{
			const auto pBuildingTypeExt = BuildingTypeExt::Fetch(pBuildingType);
			mult *= pBuildingTypeExt->BuildingOccupyDamageMult.Get(RulesClass::Instance->OccupyDamageMultiplier);
		}
	}
	else if (const auto pBunker = abstract_cast<BuildingClass*>(pThis->BunkerLinkedItem))
	{
		const auto pBunkerTypeExt = BuildingTypeExt::Fetch(pBunker->Type);
		mult *= pBunkerTypeExt->BuildingBunkerDamageMult.Get(RulesClass::Instance->BunkerDamageMultiplier);
	}
	else if (pThis->InOpenToppedTransport && pThis->Transporter)
	{
		const auto pTransporterTypeExt = TechnoExt::Fetch(pThis->Transporter)->TypeExtData;
		mult *= pTransporterTypeExt->OpenTopped_DamageMultiplier.Get(RulesClass::Instance->OpenToppedDamageMultiplier);
		mult *= TechnoExt::Fetch(pThis)->TypeExtData->OpenTransport_DamageMultiplier.Get(RulesExt::Global()->OpenTransport_DamageMultiplier);
	}

	return mult;
}

double TechnoExt::GetCurrentArmorMultiplier(TechnoClass* pThis, TechnoTypeClass* pType, HouseClass* pSourceHouse, WarheadTypeClass* pWarhead)
{
	return pThis->ArmorMultiplier * pThis->Owner->GetArmorMultiplier(pType) * TechnoExt::CalculateArmorMultipliers(pThis, pWarhead, pSourceHouse) *
		(pThis->HasAbility(Ability::Stronger) ? RulesClass::Instance->VeteranArmor : 1.0);
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
	const int currentZone = useZone ? zone : MapClass::Instance.GetMovementZoneType(pThis->GetMapCoords(), mZone, pThis->OnBridge);

	if (currentZone != -1)
	{
		if (zoneScanType == TargetZoneScanType::Any)
			return true;

		const int targetZone = MapClass::Instance.GetMovementZoneType(pTarget->GetMapCoords(), mZone, pTarget->OnBridge);

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
				const int weaponIndex = pThis->SelectWeapon(pTarget);

				if (weaponIndex < 0)
					return false;

				pWeapon = pThis->GetWeapon(weaponIndex)->WeaponType;
			}

			const double distanceSq = pCell->GetCoordsWithBridge().DistanceFromSquared(pTarget->GetCenterCoords());
			const double range = (double)pWeapon->Range;

			if (distanceSq > static_cast<double>(range) * range)
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
			TechnoExt::Fetch(pThis)->UpdateTypeData(pToType);
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
	const int originalAmmo = pThis->Ammo;
	const int maxAmmo = pToType->Ammo;
	pThis->Ammo = Math::min(originalAmmo, maxAmmo);

	if (originalAmmo > maxAmmo)
		pThis->Mark(MarkType::Change);

	// Ares ResetSpotlights -- skipped

	// Adjust ROT
	if (rtti == AbstractType::AircraftType)
		pThis->SecondaryFacing.SetROT(pToType->ROT);
	else
		pThis->PrimaryFacing.SetROT(pToType->ROT);
	// Adjust Ares TurretROT -- skipped
	//  pThis->SecondaryFacing.SetROT(TechnoTypeExt::Fetch(pToType)->TurretROT.Get(pToType->ROT));

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

	TechnoExt::Fetch(pThis)->UpdateTypeData(pToType);
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
	TechnoExt::Fetch(pThis)->UpdateTypeData(pToType);

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
bool TechnoExt::HasAttachedEffects(std::vector<AttachEffectTypeClass*> attachEffectTypes, bool requireAll, bool ignoreSameSource,
	TechnoClass* pInvoker, AbstractClass* pSource, std::vector<int> const* minCounts, std::vector<int> const* maxCounts) const
{
	unsigned int foundCount = 0;
	unsigned int typeCounter = 1;
	const bool checkSource = ignoreSameSource && pInvoker && pSource;

	for (auto const& type : attachEffectTypes)
	{
		if (type->Cumulative)
		{
			const int cumulativeCount = this->GetAttachedEffectCumulativeCount(type, ignoreSameSource, pInvoker, pSource);
			bool matched = cumulativeCount > 0;
			const unsigned int minSize = minCounts ? minCounts->size() : 0;
			const unsigned int maxSize = maxCounts ? maxCounts->size() : 0;

			if (matched && minSize > 0)
			{
				if (cumulativeCount < minCounts->at(typeCounter - 1 >= minSize ? minSize - 1 : typeCounter - 1))
					matched = false;
			}

			if (matched && maxSize > 0)
			{
				if (cumulativeCount > maxCounts->at(typeCounter - 1 >= maxSize ? maxSize - 1 : typeCounter - 1))
					matched = false;
			}

			if (matched)
			{
				// Only need to find one match, can stop here.
				if (!requireAll)
					return true;

				foundCount++;
			}
		}
		else
		{
			for (auto const& attachEffect : this->AttachedEffects)
			{
				if (attachEffect->GetType() == type && attachEffect->IsActive())
				{
					if (checkSource && attachEffect->IsFromSource(pInvoker, pSource))
						continue;

					// Only need to find one match, can stop here.
					if (!requireAll)
						return true;

					foundCount++;
					break;
				}
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
int TechnoExt::GetAttachedEffectCumulativeCount(AttachEffectTypeClass* pAttachEffectType, bool ignoreSameSource, TechnoClass* pInvoker, AbstractClass* pSource) const
{
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

void TechnoExt::InitAggressiveStance()
{
	this->AggressiveStance = this->TypeExtData->AggressiveStance.Get();
}

bool TechnoExt::GetAggressiveStance() const
{
	// If this is a passenger then obey the configuration of the transport
	if (auto pTransport = this->OwnerObject()->Transporter)
		return TechnoExt::Fetch(pTransport)->GetAggressiveStance();

	// If this is a child then obey the configuration of the parent
	if (const auto pAttachment = this->ParentAttachment)
		return TechnoExt::Fetch(pAttachment->Parent)->GetAggressiveStance();

	return this->AggressiveStance;
}

void TechnoExt::ToggleAggressiveStance()
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

bool TechnoExt::CanToggleAggressiveStance()
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

void TechnoExt::InitCeaseFireStance()
{
	this->CeaseFireStance = this->TypeExtData->CeaseFireStance.Get();
}

bool TechnoExt::GetCeaseFireStance() const
{
	// If this is a passenger then obey the configuration of the transport
	if (const auto pTransport = this->OwnerObject()->Transporter)
		return TechnoExt::Fetch(pTransport)->GetCeaseFireStance();

	// If this is a child then obey the configuration of the parent
	if (const auto pAttachment = this->ParentAttachment)
		return TechnoExt::Fetch(pAttachment->Parent)->GetCeaseFireStance();

	return this->CeaseFireStance;
}

void TechnoExt::ToggleCeaseFireStance()
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

bool TechnoExt::CanToggleCeaseFireStance()
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

// Attaches this techno in a first available attachment "slot".
// Returns true if the attachment is successful.
bool TechnoExt::AttachTo(TechnoClass* pThis, TechnoClass* pParent)
{
	auto const pParentExt = TechnoExt::Fetch(pParent);

	for (auto const& pAttachment : pParentExt->ChildAttachments)
	{
		if (pAttachment->AttachChild(pThis))
			return true;
	}

	return false;
}

bool TechnoExt::DetachFromParent(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::Fetch(pThis);

	return pExt->ParentAttachment->DetachChild();
}

void TechnoExt::InitializeAttachments()
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
	auto const pExt = TechnoExt::TryFetch(pThis);

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
	auto const pExt = TechnoExt::Fetch(pThis);

	if (pExt->ParentAttachment)
		pExt->ParentAttachment->ChildDestroyed();
}

void TechnoExt::UnlimboAttachments(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::Fetch(pThis);

	for (auto const& pAttachment : pExt->ChildAttachments)
		pAttachment->Unlimbo();
}

void TechnoExt::LimboAttachments(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::Fetch(pThis);

	for (auto const& pAttachment : pExt->ChildAttachments)
		pAttachment->Limbo();
}

void TechnoExt::TransferAttachments(TechnoClass* pThis, TechnoClass* pThat)
{
	auto const pExt = TechnoExt::Fetch(pThis);
	auto const pThatExt = TechnoExt::Fetch(pThat);

	for (auto& pAttachment : pExt->ChildAttachments)
	{
		pAttachment->Parent = pThat;
		pThatExt->ChildAttachments.push_back(std::move(pAttachment));
	}

	pExt->ChildAttachments.clear();
}

bool TechnoExt::ShouldInheritTarget(TechnoClass* pThis)
{
	if (auto const pExt = TechnoExt::TryFetch(pThis))
	{
		if (auto const pAttachment = pExt->ParentAttachment)
		{
			auto const pType = pAttachment->GetType();

			return pType->InheritTarget && pType->InheritTarget_Force;
		}
	}

	return false;
}

TechnoClass* TechnoExt::GetTrainParent(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::TryFetch(pThis);

	return pExt && pExt->ParentAttachment
		&& pExt->ParentAttachment->GetType()->InheritExperience
		? TechnoExt::GetTrainParent(pExt->ParentAttachment->Parent)
		: pThis;
}

bool TechnoExt::IsAttached(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::TryFetch(pThis);

	return pExt && pExt->ParentAttachment;
}

bool TechnoExt::HasAttachmentLoco(FootClass* pThis)
{
	return locomotion_cast<AttachmentLocomotionClass*>(pThis->Locomotor) != nullptr;
}

bool TechnoExt::DoesntOccupyCellAsChild(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::TryFetch(pThis);

	return pExt && pExt->ParentAttachment
		&& !pExt->ParentAttachment->GetType()->OccupiesCell;
}

bool TechnoExt::IsChildOf(TechnoClass* pThis, TechnoClass* pParent, bool deep)
{
	auto const pExt = TechnoExt::TryFetch(pThis);

	return pExt && pParent  // sanity check, sometimes crashes because ext is null - Kerbiter
		&& pExt->ParentAttachment
		&& (pExt->ParentAttachment->Parent == pParent
			|| (deep && TechnoExt::IsChildOf(pExt->ParentAttachment->Parent, pParent)));
}

bool TechnoExt::AreRelatives(TechnoClass* pThis, TechnoClass* pThat)
{
	return TechnoExt::GetTopLevelParent(pThis) == TechnoExt::GetTopLevelParent(pThat);
}

// Returns this if no parent.
TechnoClass* TechnoExt::GetTopLevelParent(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::TryFetch(pThis);

	return pExt  // sanity check, sometimes crashes because ext is null - Kerbiter
		&& pExt->ParentAttachment
		? TechnoExt::GetTopLevelParent(pExt->ParentAttachment->Parent)
		: pThis;
}

// Check adjacent cells from the center
// The current MapClass::Instance.PlacePowerupCrate(...) doesn't like slopes and maybe other cases
bool TechnoExt::TryToCreateCrate(CoordStruct location, Powerup selectedPowerup, int maxCellRange)
{
	CellStruct centerCell = CellClass::Coord2Cell(location);
	short currentRange = 0;
	bool placed = false;

	do
	{
		short x = -currentRange;
		short y = -currentRange;

		CellStruct checkedCell;
		checkedCell.Y = centerCell.Y + y;

		// Check upper line
		for (short i = -currentRange; i <= currentRange; i++)
		{
			checkedCell.X = centerCell.X + i;
			placed = MapClass::Instance.PlacePowerupCrate(checkedCell, selectedPowerup);

			if (placed)
				break;
		}

		if (placed)
			break;

		checkedCell.Y = centerCell.Y + (short)std::abs(y);

		// Check lower line
		for (short i = -currentRange; i <= currentRange; i++)
		{
			checkedCell.X = centerCell.X + i;
			placed = MapClass::Instance.PlacePowerupCrate(checkedCell, selectedPowerup);

			if (placed)
				break;
		}

		if (placed)
			break;

		checkedCell.X = centerCell.X + x;

		// Check left line
		for (short j = -currentRange + 1; j < currentRange; j++)
		{
			checkedCell.Y = centerCell.Y + j;
			placed = MapClass::Instance.PlacePowerupCrate(checkedCell, selectedPowerup);

			if (placed)
				break;
		}

		if (placed)
			break;

		checkedCell.X = centerCell.X + (short)std::abs(x);

		// Check right line
		for (short j = -currentRange + 1; j < currentRange; j++)
		{
			checkedCell.Y = centerCell.Y + j;
			placed = MapClass::Instance.PlacePowerupCrate(checkedCell, selectedPowerup);

			if (placed)
				break;
		}

		currentRange++;
	}
	while (!placed && currentRange < (short)maxCellRange);

	if (!placed)
		Debug::Log(__FUNCTION__": Failed to place a crate in the cell (%d,%d) and around that location.\n", centerCell.X, centerCell.Y, maxCellRange);

	return placed;
}

void TechnoExt::ResetDelayedFireTimer()
{
	this->DelayedFireTimer.Stop();
	this->DelayedFireWeaponIndex = -1;
	this->DelayedFireSequencePaused = false;

	if (this->CurrentDelayedFireAnim)
	{
		if (AnimExt::Fetch(this->CurrentDelayedFireAnim)->DelayedFireRemoveOnNoDelay)
			this->CurrentDelayedFireAnim->UnInit();
	}
}

void TechnoExt::CreateDelayedFireAnim(TechnoClass* pThis, AnimTypeClass* pAnimType, int weaponIndex, bool attach, bool center, bool removeOnNoDelay, bool onTurret, CoordStruct firingCoords)
{
	if (pAnimType)
	{
		CoordStruct coords;

		if (center)
			coords = pThis->GetCenterCoords();
		else
			coords = TechnoExt::GetFLHAbsoluteCoords(pThis, firingCoords, onTurret);

		auto const pAnim = GameCreate<AnimClass>(pAnimType, coords);

		if (attach)
			pAnim->SetOwnerObject(pThis);

		auto const pAnimExt = AnimExt::Fetch(pAnim);
		pAnim->Owner = pThis->Owner;
		pAnimExt->SetInvoker(pThis);

		if (attach)
		{
			pAnimExt->DelayedFireRemoveOnNoDelay = removeOnNoDelay;
			TechnoExt::Fetch(pThis)->CurrentDelayedFireAnim = pAnim;
		}
	}
}

bool TechnoExt::HandleDelayedFireWithPauseSequence(TechnoClass* pThis, WeaponTypeClass* pWeapon, int weaponIndex, int frame, int firingFrame)
{
	auto const pExt = TechnoExt::Fetch(pThis);
	auto& timer = pExt->DelayedFireTimer;
	auto const pWeaponExt = WeaponTypeExt::Fetch(pWeapon);

	if (pExt->DelayedFireWeaponIndex >= 0 && pExt->DelayedFireWeaponIndex != weaponIndex)
	{
		pExt->ResetDelayedFireTimer();
		pExt->DelayedFireSequencePaused = false;
	}

	if (pWeaponExt->DelayedFire_PauseFiringSequence && pWeaponExt->DelayedFire_Duration.isset() && (!pThis->Transporter || !pWeaponExt->DelayedFire_SkipInTransport))
	{
		if (pWeapon->Burst <= 1 || !pWeaponExt->DelayedFire_OnlyOnInitialBurst || pThis->CurrentBurstIndex == 0)
		{
			if (frame == firingFrame)
				pExt->DelayedFireSequencePaused = true;

			if (!timer.HasStarted())
			{
				pExt->DelayedFireWeaponIndex = weaponIndex;
				timer.Start(Math::max(GeneralUtils::GetRangedRandomOrSingleValue(pWeaponExt->DelayedFire_Duration), 0));
				auto pAnimType = pWeaponExt->DelayedFire_Animation;

				if (pThis->Transporter && pWeaponExt->DelayedFire_OpenToppedAnimation.isset())
					pAnimType = pWeaponExt->DelayedFire_OpenToppedAnimation;

				auto firingCoords = pThis->GetWeapon(weaponIndex)->FLH;

				if (pWeaponExt->DelayedFire_AnimOffset.isset())
					firingCoords = pWeaponExt->DelayedFire_AnimOffset;

				TechnoExt::CreateDelayedFireAnim(pThis, pAnimType, weaponIndex, pWeaponExt->DelayedFire_AnimIsAttached, pWeaponExt->DelayedFire_CenterAnimOnFirer,
					pWeaponExt->DelayedFire_RemoveAnimOnNoDelay, pWeaponExt->DelayedFire_AnimOnTurret, firingCoords);

				return true;
			}
			else if (timer.InProgress())
			{
				return true;
			}

			if (timer.Completed())
				pExt->ResetDelayedFireTimer();
		}

		pExt->DelayedFireSequencePaused = false;
	}

	return false;
}

bool TechnoExt::IsHealthInThreshold(TechnoClass* pObject, double min, double max)
{
	if (!pObject->Health && !pObject->GetType()->Strength)
		return true;

	const double hp = pObject->GetHealthPercentage();
	return (hp > 0 ? hp > min : hp >= min) && hp <= max;
}

void TechnoExt::ClickedApproachObject(FootClass* pThis, ObjectClass* pObject)
{
	if (Unsorted::MoveFeedback)
		pThis->VoiceMove();

	EventExt event {};
	event.Type = EventTypeExt::ApproachObject;
	event.HouseIndex = static_cast<char>(pThis->Owner->ArrayIndex);
	event.Frame = Unsorted::CurrentFrame;
	event.ApproachObject.Whom = TargetClass(pThis);
	event.ApproachObject.Target = TargetClass(pObject);
	event.AddEvent();
}

bool TechnoExt::CanBeRecruitedFix(FootClass* pThis, HouseClass* pHouse)
{
    if (pThis->Team != nullptr ||
        !pThis->IsAlive ||
        pThis->Health <= 0 ||
        pThis->InLimbo ||
        pThis->Owner != pHouse)
    {
        return false;
    }

    if (!(pThis->RecruitableA && pThis->RecruitableB))
    {
        return false;
    }

    const Mission mission = pThis->GetCurrentMission();
    if (!MissionClass::IsRecruitableMission(mission))
    {
        return false;
    }

    if (pThis->ShouldEnterAbsorber ||
        pThis->ShouldEnterOccupiable ||
        pThis->ShouldGarrisonStructure ||
        pThis->DrainTarget != nullptr ||
        pThis->BunkerLinkedItem ||
        pThis->LocomotorSource != nullptr)
    {
        return false;
    }

    return true;
}

bool TechnoExt::EjectRandomly(FootClass* pEjectee, const CoordStruct& coords, int distance, bool select)
{
	std::vector<CoordStruct> usableCoords;

	for (int direction = 0; direction < 8; ++direction)
	{
		const CellStruct tmpCoords = Unsorted::AdjacentCell[direction];
		CoordStruct ejectCoords { coords.X + tmpCoords.X * distance, coords.Y + tmpCoords.Y * distance, coords.Z };
		const auto pCell = MapClass::Instance.TryGetCellAt(ejectCoords);

		if (!pCell)
			continue;

		const auto occupied = pEjectee->IsCellOccupied(pCell, FacingType::None, -1, nullptr, true);

		if (occupied != Move::OK && occupied != Move::MovingBlock)
			continue;

		if (pEjectee->WhatAmI() == InfantryClass::AbsID)
		{
			ejectCoords = pCell->FindInfantrySubposition(ejectCoords, false, false, false);

			// Jan 31, 2026 - Starkku: FindInfantrySubposition has several code paths that return empty CoordStruct. We should ignore those.
			if (ejectCoords == CoordStruct::Empty)
				continue;

			ejectCoords.Z = coords.Z;
		}
		else
		{
			ejectCoords = CellClass::Cell2Coord(pCell->MapCoords, coords.Z);
		}

		usableCoords.emplace_back(ejectCoords);
	}

	const int count = static_cast<int>(usableCoords.size());

	if (!count)
		return false;

	return TechnoExt::EjectSurvivor(pEjectee, usableCoords[ScenarioClass::Instance->Random(0, count - 1)], select);
}

bool TechnoExt::EjectSurvivor(FootClass* pSurvivor, CoordStruct coords, bool select)
{
	const auto pCell = MapClass::Instance.GetCellAt(coords);

	pSurvivor->OnBridge = pCell->ContainsBridge();

	const int floorZ = pCell->GetCoordsWithBridge().Z;
	const bool chuted = (coords.Z - floorZ > 2 * Unsorted::LevelHeight);

	if (chuted)
	{
		pSurvivor->Limbo();

		++Unsorted::ScenarioInit;
		const bool result = pSurvivor->SpawnParachuted(coords);
		--Unsorted::ScenarioInit;

		if (!result)
			return false;
	}
	else
	{
		coords.Z = floorZ;

		++Unsorted::ScenarioInit;
		const bool result = pSurvivor->Unlimbo(coords, static_cast<DirType>(ScenarioClass::Instance->Random(0, 7)));
		--Unsorted::ScenarioInit;

		if (!result)
			return false;
	}

	if (const auto pTransporter = pSurvivor->Transporter)
	{
		if (pTransporter->GetTechnoType()->OpenTopped)
			pTransporter->ExitedOpenTopped(pSurvivor);

		pSurvivor->Transporter = nullptr;
	}

	pSurvivor->LastMapCoords = pCell->MapCoords;

	if (chuted)
	{
		const bool scat = pSurvivor->OnBridge;
		const auto occupation = scat ? pCell->AltOccupationFlags : pCell->OccupationFlags;

		if (occupation & 0x1C)
			pCell->ScatterContent(CoordStruct::Empty, true, true, scat);
	}
	else
	{
		pSurvivor->Scatter(CoordStruct::Empty, true, false);
		pSurvivor->QueueMission(pSurvivor->Owner->IsControlledByHuman() ? Mission::Guard : Mission::Hunt, 0);
	}

	pSurvivor->ShouldEnterOccupiable = false;
	pSurvivor->ShouldGarrisonStructure = false;

	if (select)
		pSurvivor->Select();

	return true;
}

struct DummyExtHere
{
	char _pad0[0x50];
	CDTimerClass DisableWeaponsTimer;
	char _pad1[0x40];
	bool DriverKilled;
};

struct DummyTypeExtHere
{
	char _[0xF4];
	ValueableVector<TechnoTypeClass*> Operators;
	bool Operator_Any;
};

bool __fastcall TechnoExt::ApplyKillDriver(TechnoClass** pData, void*, HouseClass* pToHouse, TechnoClass* pKiller, bool resetVeterancy)
{
	const auto pThis = abstract_cast<FootClass*, true>(*pData);

	if (!pThis)
		return false;

	const bool passive = pToHouse->IsNeutral();
	const auto pExt_Ares = reinterpret_cast<DummyExtHere*>(pThis->align_154);
	pExt_Ares->DriverKilled = passive;

	if (pThis->Owner == pToHouse)
		return false;

	const auto pType = pThis->GetTechnoType();
	const auto pTypeExt_Ares = reinterpret_cast<DummyTypeExtHere*>(pType->align_2FC);
	auto& passengers = pThis->Passengers;

	do
	{
		if (!passengers.GetFirstPassenger())
			break;

		if (pTypeExt_Ares->Operator_Any)
		{
			const auto pOperator = pThis->RemoveFirstPassenger();
			pOperator->RegisterDestruction(pKiller);
			pOperator->UnInit();
		}
		else if (!pTypeExt_Ares->Operators.empty())
		{
			for (NextObject passenger(passengers.GetFirstPassenger()); passenger; ++passenger)
			{
				if (!pTypeExt_Ares->Operators.Contains(passenger->GetTechnoType()))
					continue;

				const auto pOperator = static_cast<FootClass*>(*passenger);
				passengers.RemovePassenger(pOperator);

				if (pType->Gunner && !passengers.NumPassengers)
					pThis->RemoveGunner(pOperator);

				pOperator->RegisterDestruction(pKiller);
				pOperator->UnInit();
				break;
			}
		}

		const auto pTypeExt = TechnoTypeExt::Fetch(pType);

		if (passive && pTypeExt->DriverKilled_KeptPassengers.Get(RulesExt::Global()->DriverKilled_KeptPassengers))
			break;

		const bool kill = pTypeExt->DriverKilled_KillPassengers.Get(RulesExt::Global()->DriverKilled_KillPassengers);

		while (auto pPassenger = passengers.GetFirstPassenger())
		{
			const auto pNextPassenger = abstract_cast<FootClass*>(pPassenger->NextObject);
			passengers.RemovePassenger(pPassenger);

			if (pType->Gunner && !passengers.NumPassengers)
				pThis->RemoveGunner(pPassenger);

			if (kill || !TechnoExt::EjectRandomly(pPassenger, pThis->Location, 128, false))
			{
				pPassenger->RegisterDestruction(nullptr);
				pPassenger->UnInit();
			}
			else if (pType->OpenTopped)
			{
				pThis->ExitedOpenTopped(pPassenger);
			}

			pPassenger = pNextPassenger;
		}
	}
	while (false);

	pThis->HijackerInfantryType = -1;

	if (resetVeterancy)
		pThis->Veterancy.SetRookie(false);

	if (const auto pControlledBy = pThis->MindControlledBy)
	{
		if (const auto pManager = pControlledBy->CaptureManager)
			pManager->FreeUnit(pThis);
	}

	pThis->MindControlledByAUnit = false;
	pThis->MindControlledByHouse = nullptr;

	if (const auto pRingAnim = pThis->MindControlRingAnim)
	{
		pRingAnim->UnInit();
		pThis->MindControlRingAnim = nullptr;
	}

	if (const auto pTeam = pThis->Team)
		pTeam->LiberateMember(pThis);

	if (const auto pManager = pThis->CaptureManager)
		pManager->FreeAll();

	if (const auto pManager = pThis->SpawnManager)
	{
		pManager->KillNodes();
		pManager->ResetTarget();
	}

	if (const auto pManager = pThis->SlaveManager)
	{
		pManager->Killed(pKiller);
		pManager->AllGuard();
		pManager->Owner = pThis;

		if (passive)
			pManager->SuspendWork();
		else
			pManager->ResumeWork();
	}

	pThis->SetOwningHouse(pToHouse);

	if (passive)
		pThis->QueueMission(Mission::Harmless, true);

	pThis->SetTarget(nullptr);
	pThis->SetDestination(nullptr, false);

	auto pTag = pThis->AttachedTag;

	if (pTag)
		pTag->RaiseEvent(static_cast<TriggerEvent>(0x44), pThis, CellStruct::Empty, false, pKiller);

	pTag = pThis->AttachedTag;

	if (pTag && pThis->IsAlive)
		pTag->RaiseEvent(static_cast<TriggerEvent>(0x43), pThis, CellStruct::Empty);

	return true;
}

int TechnoExt::GetSight()
{
	double sight = this->TypeExtData->OwnerObject()->Sight;

	for (auto& callback : TechnoExtInterop::CalculateSightCallbacks)
	{
		if (callback)
			sight = callback(this->OwnerObject(), sight);
	}

	return static_cast<int>(sight);
}

bool TechnoExt::CanReceiveEvent(TechnoClass* pThis, HouseClass* pHouse)
{
	if (pThis->Berzerk)
		return false;

	if (pThis->GetTechnoType()->Spawned)
		return false;

	if (pThis->SlaveOwner)
		return false;

	auto const pOwner = pThis->GetOwningHouse();

	if (pOwner != pHouse && !(pHouse->IsCurrentPlayer() && pOwner->IsControlledByCurrentPlayer()))
		return false;

	return true;
}

bool TechnoExt::HasWeaponsDisabled(TechnoClass* pThis)
{
	if (TechnoExt::Fetch(pThis)->AE.DisableWeapons)
		return true;

	if (AresHelper::CanUseAres)
	{
		const auto pExt_Ares = reinterpret_cast<DummyExtHere*>(pThis->align_154);

		if (pExt_Ares->DisableWeaponsTimer.InProgress())
			return true;
	}

	return false;
}

FireError TechnoExt::GetFireErrorIgnoreDisableWeapons(TechnoClass* pThis, AbstractClass* pTarget, int weaponIndex, bool ignoreRange)
{
	auto const pExt = TechnoExt::Fetch(pThis);
	auto const pExt_Ares = reinterpret_cast<DummyExtHere*>(pThis->align_154);
	bool const canUseAres = AresHelper::CanUseAres;
	bool const disableWeapons = pExt->AE.DisableWeapons;
	int timeLeft = 0;

	pExt->AE.DisableWeapons = false;

	if (canUseAres)
	{
		timeLeft = pExt_Ares->DisableWeaponsTimer.GetTimeLeft();
		pExt_Ares->DisableWeaponsTimer.Stop();
	}

	auto const fireError = pThis->GetFireError(pTarget, weaponIndex, ignoreRange);
	pExt->AE.DisableWeapons = disableWeapons;

	if (canUseAres && timeLeft > 0)
		pExt_Ares->DisableWeaponsTimer.Start(timeLeft);

	return fireError;
}

// =============================
// load / save

template <typename T>
void TechnoExt::Serialize(T& Stm)
{
	Stm
		.Process(this->TypeExtData)
		.Process(this->Shield)
		.Process(this->LaserTrails)
		.Process(this->AttachedEffects)
		.Process(this->AE)
		.Process(this->AnimRefCount)
		.Process(this->PassengerDeletionTimer)
		.Process(this->CurrentShieldType)
		.Process(this->ChargeTurretTimer)
		.Process(this->AutoDeathTimer)
		.Process(this->MindControlRingAnimType)
		.Process(this->DamageNumberOffset)
		.Process(this->HasBeenPlacedOnMap)
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
		.Process(this->AggressiveStance)
		.Process(this->CeaseFireStance)
		.Process(this->IsWreckage)
		.Process(this->JumpjetFromAirport)
		.Process(this->BuildingOccupying)
		.Process(this->SquadManager)
		.Process(this->ParentAttachment)
		.Process(this->ChildAttachments)
		.Process(this->ThisOccupationCell)
		.Process(this->LastOccupationCell)
		.Process(this->AltOccupation)
		.Process(this->AirstrikeTargetingMe)
		.Process(this->DelayedFireSequencePaused)
		.Process(this->DelayedFireTimer)
		.Process(this->DelayedFireWeaponIndex)
		.Process(this->CurrentDelayedFireAnim)
		.Process(this->DropCrate)
		.Process(this->DropCrateType)
		.Process(this->AttachedEffectInvokerCount)
		.Process(this->IsSelected)
		.Process(this->TintColorOwner)
		.Process(this->TintColorAllies)
		.Process(this->TintColorEnemies)
		.Process(this->TintIntensityOwner)
		.Process(this->TintIntensityAllies)
		.Process(this->TintIntensityEnemies)
		.Process(this->SpecialTracked)
		.Process(this->BulletsTargetingMeCount)
		.Process(this->FallingDownTracked)
		.Process(this->OnParachuted)
		.Process(this->HoverShutdown)
		/*.Process(this->QueuedShift)*/ // Always set and reset in one function
		.Process(this->ShiftApplier)
		.Process(this->ShiftApplierHouse)
		.Process(this->LastTargetCrd)
		.Process(this->LastTargetCrdClearTimer)
		.Process(this->ShouldBeDead)
		.Process(this->PreventCrewEscape)
		;
}

void TechnoExt::OnDetach(AirstrikeClass* pTarget, bool removed)
{
	if (removed)
		AnnounceInvalidPointer(this->AirstrikeTargetingMe, pTarget);
}

void TechnoExt::OnDetach(AbstractClass* pTarget, bool removed)
{
	if (this->HasCachedClickMission && this->CachedTarget == pTarget)
	{
		this->HasCachedClickMission = false;
		this->CachedMission = Mission::None;
		this->CachedCell = nullptr;
		this->CachedTarget = nullptr;
	}
}

void TechnoExt::OnDetach(TechnoClass* pTarget, bool removed)
{
	AnnounceInvalidPointer(this->ShiftApplier, pTarget);
}

void TechnoExt::OnDetach(HouseClass* pTarget, bool removed)
{
	AnnounceInvalidPointer(this->ShiftApplierHouse, pTarget);
}

void TechnoExt::LoadFromStream(PhobosStreamReader& Stm)
{
	RadioExt::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void TechnoExt::SaveToStream(PhobosStreamWriter& Stm)
{
	RadioExt::SaveToStream(Stm);
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
// container hooks

// The extension is allocated by the concrete leaf constructors (UnitClass/InfantryClass/
// BuildingClass/AircraftClass), not here at the abstract TechnoClass level.

// The extension is removed by the leaf destructor hooks; this only keeps the
// tech tree recheck side effect at the shared base destructor.
DEFINE_HOOK(0x6F4500, TechnoClass_DTOR, 0x5)
{
	GET(TechnoClass*, pItem, ECX);

	if (pItem->AbstractFlags & AbstractFlags::Foot)
		pItem->Owner->RecheckTechTree = true; // for SW.AuxTechons and SW.NegTechnos

	return 0;
}

DEFINE_HOOK(0x710415, TechnoClass_DetachAnim, 0x6)
{
	GET(TechnoClass*, pThis, ECX);
	GET(AbstractClass*, pTarget, EAX);

	auto const pExt = TechnoExt::Fetch(pThis);

	if (pExt->CurrentDelayedFireAnim == pTarget)
		pExt->CurrentDelayedFireAnim = nullptr;

	return 0;
}
