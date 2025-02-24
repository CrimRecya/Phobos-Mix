#include "Body.h"

#include <EventClass.h>
#include <SpawnManagerClass.h>
#include <JumpjetLocomotionClass.h>
#include <HoverLocomotionClass.h>

#include <Ext/Building/Body.h>
#include <Ext/WeaponType/Body.h>
#include "Ext/BulletType/Body.h"
#include "Ext/WarheadType/Body.h"
#include <Utilities/Helpers.Alex.h>
#include <Helpers/Macro.h>

#pragma region NoBurstDelay

DEFINE_HOOK(0x5209EE, InfantryClass_UpdateFiring_BurstNoDelay, 0x5)
{
	enum { SkipVanillaFire = 0x520A57 };

	GET(InfantryClass* const, pThis, EBP);
	GET(const int, wpIdx, ESI);
	GET(AbstractClass* const, pTarget, EAX);

	if (const auto pWeapon = pThis->GetWeapon(wpIdx)->WeaponType)
	{
		if (pWeapon->Burst > 1)
		{
			if (WeaponTypeExt::ExtMap.Find(pWeapon)->Burst_NoDelay)
			{
				if (pThis->Fire(pTarget, wpIdx))
				{
					if (!pThis->CurrentBurstIndex)
						return SkipVanillaFire;

					auto rof = pThis->RearmTimer.TimeLeft;
					pThis->RearmTimer.Start(0);

					for (auto i = pThis->CurrentBurstIndex; i != pWeapon->Burst && pThis->GetFireError(pTarget, wpIdx, true) == FireError::OK && pThis->Fire(pTarget, wpIdx); ++i)
					{
						rof = pThis->RearmTimer.TimeLeft;
						pThis->RearmTimer.Start(0);
					}

					pThis->RearmTimer.Start(rof);
				}

				return SkipVanillaFire;
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x736F67, UnitClass_UpdateFiring_BurstNoDelay, 0x6)
{
	enum { SkipVanillaFire = 0x737063 };

	GET(UnitClass* const, pThis, ESI);
	GET(const int, wpIdx, EDI);
	GET(AbstractClass* const, pTarget, EAX);

	if (const auto pWeapon = pThis->GetWeapon(wpIdx)->WeaponType)
	{
		if (pWeapon->Burst > 1)
		{
			if (WeaponTypeExt::ExtMap.Find(pWeapon)->Burst_NoDelay)
			{
				if (pThis->Fire(pTarget, wpIdx))
				{
					if (!pThis->CurrentBurstIndex)
						return SkipVanillaFire;

					auto rof = pThis->RearmTimer.TimeLeft;
					pThis->RearmTimer.Start(0);

					for (auto i = pThis->CurrentBurstIndex; i != pWeapon->Burst && pThis->GetFireError(pTarget, wpIdx, true) == FireError::OK && pThis->Fire(pTarget, wpIdx); ++i)
					{
						rof = pThis->RearmTimer.TimeLeft;
						pThis->RearmTimer.Start(0);
					}

					pThis->RearmTimer.Start(rof);
				}

				return SkipVanillaFire;
			}
		}
	}

	return 0;
}

#pragma endregion

#pragma region AIConstructionYard

DEFINE_HOOK(0x740A11, UnitClass_Mission_Guard_AIAutoDeployMCV, 0x6)
{
	enum { SkipGameCode = 0x740A50 };

	GET(UnitClass*, pMCV, ESI);

	return (!RulesExt::Global()->AIAutoDeployMCV && pMCV->Owner->NumConYards > 0) ? SkipGameCode : 0;
}

DEFINE_HOOK(0x739889, UnitClass_TryToDeploy_AISetBaseCenter, 0x6)
{
	enum { SkipGameCode = 0x73992B };

	GET(UnitClass*, pMCV, EBP);

	return (!RulesExt::Global()->AISetBaseCenter && pMCV->Owner->NumConYards > 1) ? SkipGameCode : 0;
}

DEFINE_HOOK(0x4FD538, HouseClass_AIHouseUpdate_CheckAIBaseCenter, 0x7)
{
	if (RulesExt::Global()->AIBiasSpawnCell && !SessionClass::IsCampaign())
	{
		GET(HouseClass*, pAI, EBX);

		if (const auto count = pAI->ConYards.Count)
		{
			const auto wayPoint = pAI->GetSpawnPosition();

			if (wayPoint != -1)
			{
				const auto center = ScenarioClass::Instance->GetWaypointCoords(wayPoint);
				auto newCenter = center;
				double distanceSquared = 131072.0;

				for (int i = 0; i < count; ++i)
				{
					if (const auto pBuilding = pAI->ConYards.GetItem(i))
					{
						if (pBuilding->IsAlive && pBuilding->Health && !pBuilding->InLimbo)
						{
							const auto newDistanceSquared = pBuilding->GetMapCoords().DistanceFromSquared(center);

							if (newDistanceSquared < distanceSquared)
							{
								distanceSquared = newDistanceSquared;
								newCenter = pBuilding->GetMapCoords();
							}
						}
					}
				}

				if (newCenter != center)
				{
					pAI->BaseSpawnCell = newCenter;
					pAI->Base.Center = newCenter;
				}
			}
		}
	}

	return 0;
}
/*
DEFINE_HOOK(0x4FE42F, HouseClass_AIBaseConstructionUpdate_SkipConYards, 0x6)
{
	enum { SkipGameCode = 0x4FE443 };

	GET(BuildingTypeClass*, pType, EAX);

	return (RulesExt::Global()->AIForbidConYard && pType->ConstructionYard) ? SkipGameCode : 0;
}

DEFINE_HOOK(0x505550, HouseClass_AIBaseConstructionUpdate_SkipConYards, 0x6)
{
	enum { SkipGameCode = 0x5056C1 };

	GET(BuildingTypeClass*, pType, ESI);

	return (RulesExt::Global()->AIForbidConYard && pType->ConstructionYard) ? SkipGameCode : 0;
}
*/
#pragma endregion

#pragma region KickOutStuckUnits

// Kick out stuck units when the factory building is not busy
DEFINE_HOOK(0x450248, BuildingClass_UpdateFactory_KickOutStuckUnits, 0x6)
{
	GET(BuildingClass*, pThis, ESI);

	if (!(Unsorted::CurrentFrame % 15))
	{
		const auto pType = pThis->Type;

		if (pType->Factory == AbstractType::UnitType && pType->WeaponsFactory && !pType->Naval && pThis->QueuedMission != Mission::Unload)
		{
			const auto mission = pThis->CurrentMission;

			if (mission == Mission::Guard || (mission == Mission::Unload && pThis->MissionStatus == 1))
				BuildingExt::KickOutStuckUnits(pThis);
		}
	}

	return 0;
}

// Should not kick out units if the factory building is in construction process
DEFINE_HOOK(0x4444A0, BuildingClass_KickOutUnit_NoKickOutInConstruction, 0xA)
{
	enum { ThisIsOK = 0x444565, ThisIsNotOK = 0x4444B3};

	GET(BuildingClass* const, pThis, ESI);

	const auto mission = pThis->GetCurrentMission();

	return (mission == Mission::Unload || mission == Mission::Construction) ? ThisIsNotOK : ThisIsOK;
}

#pragma endregion

#pragma region AirBarrier

void __fastcall FindMovingInfOrVeh(CellClass* const pCell, const AbstractType findType)
{
	const auto flag = pCell->OccupationFlags;
	pCell->OccupationFlags = 0;

	for (int i = 0; i < 8; ++i)
	{
		for (auto pObject = pCell->GetNeighbourCell(static_cast<FacingType>(i))->FirstObject; pObject; pObject = pObject->NextObject)
		{
			const auto absType = pObject->WhatAmI();

			if (absType == findType && static_cast<FootClass*>(pObject)->Locomotor->Is_Moving_Now())
			{
				pCell->OccupationFlags = flag;
				return;
			}
		}
	}
}

void __fastcall FindMovingInfAndVeh(CellClass* const pCell)
{
	const auto flag = pCell->OccupationFlags;
	pCell->OccupationFlags = 0;
	bool inf = false;
	bool veh = false;

	for (int i = 0; i < 8; ++i)
	{
		for (auto pObject = pCell->GetNeighbourCell(static_cast<FacingType>(i))->FirstObject; pObject; pObject = pObject->NextObject)
		{
			const auto absType = pObject->WhatAmI();

			if (absType == AbstractType::Infantry)
			{
				if (!inf && static_cast<InfantryClass*>(pObject)->Locomotor->Is_Moving_Now())
				{
					pCell->OccupationFlags |= (flag & 0x1F);

					if (veh)
						return;

					inf = true;
				}
			}
			else if (absType == AbstractType::Unit)
			{
				if (!veh && static_cast<UnitClass*>(pObject)->Locomotor->Is_Moving_Now())
				{
					pCell->OccupationFlags |= (flag & 0x20);

					if (inf)
						return;

					veh = true;
				}
			}
		}
	}
}

void __fastcall FindAltMovingInfOrVeh(CellClass* const pCell, const AbstractType findType)
{
	const auto flag = pCell->AltOccupationFlags;
	pCell->AltOccupationFlags = 0;

	for (int i = 0; i < 8; ++i)
	{
		for (auto pObject = pCell->GetNeighbourCell(static_cast<FacingType>(i))->AltObject; pObject; pObject = pObject->NextObject)
		{
			const auto absType = pObject->WhatAmI();

			if (absType == findType && static_cast<FootClass*>(pObject)->Locomotor->Is_Moving_Now())
			{
				pCell->AltOccupationFlags = flag;
				return;
			}
		}
	}
}

void __fastcall FindAltMovingInfAndVeh(CellClass* const pCell)
{
	const auto flag = pCell->AltOccupationFlags;
	pCell->AltOccupationFlags = 0;
	bool inf = false;
	bool veh = false;

	for (int i = 0; i < 8; ++i)
	{
		for (auto pObject = pCell->GetNeighbourCell(static_cast<FacingType>(i))->AltObject; pObject; pObject = pObject->NextObject)
		{
			const auto absType = pObject->WhatAmI();

			if (absType == AbstractType::Infantry)
			{
				if (!inf && static_cast<InfantryClass*>(pObject)->Locomotor->Is_Moving_Now())
				{
					pCell->AltOccupationFlags |= (flag & 0x1F);

					if (veh)
						return;

					inf = true;
				}
			}
			else if (absType == AbstractType::Unit)
			{
				if (!veh && static_cast<UnitClass*>(pObject)->Locomotor->Is_Moving_Now())
				{
					pCell->AltOccupationFlags |= (flag & 0x20);

					if (inf)
						return;

					veh = true;
				}
			}
		}
	}
}

DEFINE_HOOK(0x55B4E1, LogicClass_Update_UnmarkCellOccupationFlags, 0x5)
{
	const auto delay = RulesExt::Global()->CleanUpAirBarrier.Get();

	if (delay > 0 && !(Unsorted::CurrentFrame % delay))
	{
		const auto pMap = MapClass::Instance();
		pMap->CellIteratorReset();

		for (auto pCell = pMap->CellIteratorNext(); pCell; pCell = pMap->CellIteratorNext())
		{
			if ((0xFF & pCell->OccupationFlags) && !pCell->FirstObject)
			{
				pCell->OccupationFlags &= 0x3F; // ~(Aircraft | Building)

				if (pCell->OccupationFlags & 0x1F)
				{
					if (pCell->OccupationFlags & 0x20)
						FindMovingInfAndVeh(pCell);
					else
						FindMovingInfOrVeh(pCell, AbstractType::Infantry);
				}
				else if (pCell->OccupationFlags & 0x20)
				{
					FindMovingInfOrVeh(pCell, AbstractType::Unit);
				}
			}

			if ((0xFF & pCell->AltOccupationFlags) && !pCell->AltObject)
			{
				pCell->AltOccupationFlags &= 0x3F; // ~(Aircraft | Building)

				if (pCell->AltOccupationFlags & 0x1F)
				{
					if (pCell->AltOccupationFlags & 0x20)
						FindAltMovingInfAndVeh(pCell);
					else
						FindAltMovingInfOrVeh(pCell, AbstractType::Infantry);
				}
				else if (pCell->AltOccupationFlags & 0x20)
				{
					FindAltMovingInfOrVeh(pCell, AbstractType::Unit);
				}
			}
		}
	}

	return 0;
}

#pragma endregion

#pragma region NoQueueUpToEnterAndUnload

bool __fastcall CanEnterNow(UnitClass* pTransport, FootClass* pPassenger)
{
	const auto pOwner = pTransport->Owner;

	if (!pOwner || !pOwner->IsAlliedWith(pPassenger) || pTransport->IsBeingWarpedOut())
		return false;

	if (pPassenger->IsMindControlled() || pPassenger->ParasiteEatingMe)
		return false;

	const auto pManager = pPassenger->CaptureManager;

	if (pManager && pManager->IsControllingSomething())
		return false;

	const auto passengerSize = pPassenger->GetTechnoType()->Size;
	const auto pTransportType = pTransport->Type;

	if (passengerSize > pTransportType->SizeLimit)
		return false;

	const auto maxSize = pTransportType->Passengers;
	const auto predictSize = pTransport->Passengers.GetTotalSize() + static_cast<int>(passengerSize);
	const auto pLink = pTransport->GetNthLink();
	const auto needCalculate = pLink && pLink != pPassenger;

	if (needCalculate)
	{
		const auto linkCell = pLink->GetCoords();
		const auto tranCell = pTransport->GetCoords();

		// When the most important passenger is close, need to prevent overlap
		if (abs(linkCell.X - tranCell.X) <= 384 && abs(linkCell.Y - tranCell.Y) <= 384)
			return (predictSize <= (maxSize - pLink->GetTechnoType()->Size));
	}

	const auto remain = maxSize - predictSize;

	if (remain < 0)
		return false;

	if (needCalculate && remain < static_cast<int>(pLink->GetTechnoType()->Size))
	{
		// Avoid passenger moving forward, resulting in overlap with transport and create invisible barrier
		pLink->SendToFirstLink(RadioCommand::NotifyUnlink);
		pLink->EnterIdleMode(false, true);
	}

	return true;
}

DEFINE_HOOK(0x51A0D4, InfantryClass_UpdatePosition_NoQueueUpToEnter, 0x6)
{
	enum { EnteredThenReturn = 0x51A47E };

	GET(InfantryClass* const, pThis, ESI);

	if (const auto pDest = abstract_cast<UnitClass*>(pThis->CurrentMission == Mission::Enter ? pThis->Destination : pThis->QueueUpToEnter))
	{
		if (pDest->Type->Passengers > 0 && TechnoTypeExt::ExtMap.Find(pDest->Type)->NoQueueUpToEnter.Get(RulesExt::Global()->NoQueueUpToEnter))
		{
			const auto thisCell = pThis->GetCoords();
			const auto destCell = pDest->GetCoords();

			if (abs(thisCell.X - destCell.X) <= 384 && abs(thisCell.Y - destCell.Y) <= 384)
			{
				if (CanEnterNow(pDest, pThis)) // Replace send radio command: QueryCanEnter
				{
					if (const auto pTag = pDest->AttachedTag)
						pTag->RaiseEvent(TriggerEvent::EnteredBy, pThis, CellStruct::Empty);

					pThis->ArchiveTarget = nullptr;
					pThis->OnBridge = false;
					pThis->MissionAccumulateTime = 0;
					pThis->GattlingValue = 0;
					pThis->CurrentGattlingStage = 0;

					if (const auto pMind = pThis->MindControlledBy)
					{
						if (const auto pManager = pMind->CaptureManager)
							pManager->FreeUnit(pThis);
					}

					pThis->Limbo();

					if (pDest->Type->OpenTopped)
						pDest->EnteredOpenTopped(pThis);

					pThis->Transporter = pDest;
					pDest->AddPassenger(pThis);
					pThis->Undiscover();

					// Added, to prevent passengers from wanting to get on after getting off
					pThis->QueueUpToEnter = nullptr;

					// Added, to stop the passengers and let OpenTopped work normally
					pThis->SetSpeedPercentage(0.0);

					// Added, to stop hover unit's meaningless behavior
					if (const auto pHover = locomotion_cast<HoverLocomotionClass*>(pThis->Locomotor))
						pHover->MaxSpeed = 0;

					return EnteredThenReturn;
				}
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x73A5EA, UnitClass_UpdatePosition_NoQueueUpToEnter, 0x5)
{
	enum { EnteredThenReturn = 0x73A78C };

	GET(UnitClass* const, pThis, EBP);

	if (const auto pDest = abstract_cast<UnitClass*>(pThis->CurrentMission == Mission::Enter ? pThis->Destination : pThis->QueueUpToEnter))
	{
		if (pDest->Type->Passengers > 0 && TechnoTypeExt::ExtMap.Find(pDest->Type)->NoQueueUpToEnter.Get(RulesExt::Global()->NoQueueUpToEnter))
		{
			const auto thisCell = pThis->GetCoords();
			const auto destCell = pDest->GetCoords();

			if (abs(thisCell.X - destCell.X) <= 384 && abs(thisCell.Y - destCell.Y) <= 384)
			{
				if (CanEnterNow(pDest, pThis)) // Replace send radio command: QueryCanEnter
				{
					// I don't know why units have no trigger

					pThis->ArchiveTarget = nullptr;
					pThis->OnBridge = false;
					pThis->MissionAccumulateTime = 0;
					pThis->GattlingValue = 0;
					pThis->CurrentGattlingStage = 0;

					if (const auto pMind = pThis->MindControlledBy)
					{
						if (const auto pManager = pMind->CaptureManager)
							pManager->FreeUnit(pThis);
					}

					pThis->Limbo();
					pDest->AddPassenger(pThis);

					if (pDest->Type->OpenTopped)
						pDest->EnteredOpenTopped(pThis);

					pThis->Transporter = pDest;

					if (pThis->Type->OpenTopped)
						pThis->SetTargetForPassengers(nullptr);

					pThis->Undiscover();

					// Added, to prevent passengers from wanting to get on after getting off
					pThis->QueueUpToEnter = nullptr;

					// Added, to stop the passengers and let OpenTopped work normally
					pThis->SetSpeedPercentage(0.0);

					// Added, to stop hover unit's meaningless behavior
					if (const auto pHover = locomotion_cast<HoverLocomotionClass*>(pThis->Locomotor))
						pHover->MaxSpeed = 0;

					return EnteredThenReturn;
				}
			}
		}
	}

	return 0;
}

static inline void PlayUnitLeaveTransportSound(UnitClass* pThis)
{
	const int sound = pThis->Type->LeaveTransportSound;

	if (sound != -1)
		VoxClass::PlayAtPos(sound, &pThis->Location);
}

DEFINE_HOOK(0x73DC9C, UnitClass_Mission_Unload_NoQueueUpToUnloadBreak, 0xA)
{
	enum { SkipGameCode = 0x73E289 };

	GET(UnitClass* const, pThis, ESI);
	GET(FootClass* const, pPassenger, EDI);

	pPassenger->Undiscover();

	// Play the sound when interrupted for some reason
	if (TechnoTypeExt::ExtMap.Find(pThis->Type)->NoQueueUpToUnload.Get(RulesExt::Global()->NoQueueUpToUnload))
		PlayUnitLeaveTransportSound(pThis);

	return SkipGameCode;
}

DEFINE_HOOK(0x73DC1E, UnitClass_Mission_Unload_NoQueueUpToUnloadLoop, 0xA)
{
	enum { UnloadLoop = 0x73D8CB, UnloadReturn = 0x73E289 };

	GET(UnitClass* const, pThis, ESI);

	if (TechnoTypeExt::ExtMap.Find(pThis->Type)->NoQueueUpToUnload.Get(RulesExt::Global()->NoQueueUpToUnload))
	{
		if (pThis->Passengers.NumPassengers <= pThis->NonPassengerCount)
		{
			// If unloading is required within one frame, the sound will only be played when the last passenger leaves
			PlayUnitLeaveTransportSound(pThis);
			pThis->MissionStatus = 4;
			return UnloadReturn;
		}

		R->EBX(0); // Reset
		return UnloadLoop;
	}

	PlayUnitLeaveTransportSound(pThis);
	return UnloadReturn;
}
/*
static inline bool CanBuildingUnloadOccupants(BuildingClass* pThis)
{
	if (pThis->GetOccupantCount() <= 0)
		return false;

	const auto topLeftCell = pThis->GetMapCoords();
	const auto pOccupant = pThis->Occupants.GetItem(0);

	for (auto pFoundation = pThis->Type->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
	{
		if (const auto pSearchCell = MapClass::Instance->TryGetCellAt(topLeftCell + *pFoundation))
		{
			if (pOccupant->IsCellOccupied(pSearchCell, FacingType::None, -1, nullptr, true) == Move::OK)
				return true;
		}
	}

	return false;
}

DEFINE_HOOK(0x44733A, BuildingClass_MouseOverObject_BuildingCheckDeploy, 0xA)
{
	enum { OccupantsCannotLeave = 0x447348, OccupantsCanLeave = 0x4472E7 };

	GET(BuildingClass* const, pThis, ESI);

	return CanBuildingUnloadOccupants(pThis) ? OccupantsCanLeave : OccupantsCannotLeave;
}
*/
#pragma endregion

#pragma region TechnoInRangeFix

DEFINE_HOOK_AGAIN(0x4D6541, FootClass_ApproachTarget_InRangeSourceCoordsFix, 0x6)
DEFINE_HOOK(0x4D621D, FootClass_ApproachTarget_InRangeSourceCoordsFix, 0x6)
{
	GET(FootClass*, pThis, EBX);
	GET(WeaponTypeClass*, pWeapon, ECX);
	REF_STACK(CoordStruct, sourceCoords, STACK_OFFSET(0x158, -0x12C));

	bool cylinder = RulesExt::Global()->CylinderRangefinding;

	if (auto const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon))
		cylinder = pWeaponExt->CylinderRangefinding.Get(cylinder);

	if (cylinder || pThis->IsInAir())
	{
		sourceCoords.Z = pThis->Target->GetCoords().Z;
	}
	else if (pWeapon && pWeapon->CellRangefinding)
	{
		const auto pCell = MapClass::Instance->GetCellAt(sourceCoords);
		sourceCoords = pCell->GetCoords();

		if (pCell->ContainsBridge())
			sourceCoords.Z += CellClass::BridgeHeight;
	}
	else if (R->Origin() == 0x4D6541)
	{
		const auto pCell = MapClass::Instance->GetCellAt(sourceCoords);
		sourceCoords.Z = pCell->GetFloorHeight(Point2D { sourceCoords.X, sourceCoords.Y });

		if (pCell->ContainsBridge())
			sourceCoords.Z += CellClass::BridgeHeight;
	}

	return 0;
}

#pragma endregion

#pragma region DetectionLogic
/*
DEFINE_HOOK(0x5865E2, MapClass_IsLocationFogged_Check, 0x5)
{
	REF_STACK(CoordStruct*, pCoords, STACK_OFFSET(0x0, 0x4));

	const int level = pCoords->Z / Unsorted::LevelHeight;
	const int extra = (level & 1) ? ((level >> 1) + 1) : (level >> 1);
	const CellStruct cell { static_cast<short>((pCoords->X >> 8) - extra), static_cast<short>((pCoords->Y >> 8) - extra) };

	R->EAX(!(MapClass::Instance->GetCellAt(cell)->AltFlags & AltCellFlags::NoFog));
	return 0;
}
*/
// 0x655DDD
// 0x6D8FD0

#pragma endregion

#pragma region NewFactories

// Disappear
// 0x44EC3A
// BeginProduction
// 0x4FA39C
// 0x4FA553
// 0x4FA76D
// SuspendProduction
// 0x4FA942
// AbandonProduction
// 0x4FAA5C
// 0x4FABCB
// UnitFromFactory
// 0x4FB11D
// PointerExpired
// 0x4FBC75
// GetPrimaryFactory
// 0x500510
// SetPrimaryFactory
// 0x500850
// UpdateFactoriesQueues
// 0x509149
// ShouldDisableCameo
// 0x50B3A0

// FindFactory -> Ares hooks all of these away
// 0x5F7900

#pragma endregion

#pragma region EngineerAutoFire

DEFINE_HOOK(0x707E84, TechnoClass_GetGuardRange_Engineer, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return 0;
}

DEFINE_HOOK(0x6F8EF1, TechnoClass_SelectAutoTarget_Engineer, 0x6)
{
	enum { SkipGameCode = 0x6F8EF7 };

	GET(InfantryTypeClass* const, pType, EAX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	R->CL(pType->Engineer && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

DEFINE_HOOK(0x709249, TechnoClass_CanPassiveAcquireNow_Engineer1, 0xA)
{
	enum { SkipGameCode = 0x709253 };

	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

DEFINE_HOOK(0x6F8AEC, TechnoClass_TryAutoTargetObject_Engineer1, 0x6)
{
	enum { SkipGameCode = 0x6F8AF2 };

	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

DEFINE_HOOK(0x6F8BB2, TechnoClass_TryAutoTargetObject_Engineer2, 0x6)
{
	enum { SkipGameCode = 0x6F8BB8 };

	GET(TechnoClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	R->AL(pThis->IsEngineer() && !(pTypeExt && pTypeExt->Engineer_CanAutoFire));
	return SkipGameCode;
}

#pragma endregion

#pragma region AttackWall

DEFINE_HOOK(0x6F8C18, TechnoClass_ScanToAttackWall_PlayerDestroyWall, 0x6)
{
	enum { SkipIsAIChecks = 0x6F8C52, FuncRetZero = 0x6F8DE3 };

	GET(TechnoClass*, pThis, ESI);

	if (!pThis->Owner->IsControlledByHuman())
		return 0;

	return RulesExt::Global()->PlayerDestroyWalls ? SkipIsAIChecks : FuncRetZero;
}

DEFINE_HOOK(0x6F8D32, TechnoClass_ScanToAttackWall_DestroyOwnerlessWalls, 0x9)
{
	enum { GoOtherChecks = 0x6F8D58, NotOkToFire = 0x6F8DE3 };

	GET(int, OwnerIdx, EAX);
	GET(TechnoClass*, pThis, ESI);

	if (auto const pOwner = (OwnerIdx != -1) ? HouseClass::Array->Items[OwnerIdx] : nullptr)
	{
		if (pOwner->IsAlliedWith(pThis->Owner)
			&& (!RulesExt::Global()->DestroyOwnerlessWalls
			|| (pOwner != HouseClass::FindSpecial()
			&& pOwner != HouseClass::FindCivilianSide()
			&& pOwner != HouseClass::FindNeutral())))
		{
			return NotOkToFire;
		}
	}

	return GoOtherChecks;
}

DEFINE_HOOK(0x6F9B64, TechnoClass_SelectAutoTarget_RecordAttackWall, 0x7)
{
	GET(TechnoClass*, pThis, ESI);
	GET(CellClass*, pCell, EAX);

	if (auto pExt = TechnoExt::ExtMap.Find(pThis))
		pExt->AutoTargetedWallCell = pCell;

	return 0;
}

#pragma endregion

#pragma region CylinderRange

DEFINE_HOOK(0x6F7891, TechnoClass_IsCloseEnough_CylinderRangefinding, 0x7)
{
	enum { SkipGameCode = 0x6F789A };

	GET(WeaponTypeClass* const, pWeaponType, EDI);
	GET(TechnoClass* const, pThis, ESI);

	bool cylinder = RulesExt::Global()->CylinderRangefinding;

	if (auto const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeaponType))
		cylinder = pWeaponExt->CylinderRangefinding.Get(cylinder);

	R->AL(cylinder ? true : pThis->IsInAir());
	return SkipGameCode;
}

#pragma endregion

#pragma region RecycleSpawned

DEFINE_HOOK(0x6B77B4, SpawnManagerClass_Update_RecycleSpawned, 0x7)
{
	//enum { RecycleIsOk = 0x6B77FF, RecycleIsNotOk = 0x6B7838 };

	GET(SpawnManagerClass* const, pThis, ESI);
	GET(AircraftClass* const, pSpawned, EDI);
	GET(CellStruct* const, pSpawnerMapCrd, EBP);

	if (!pThis)
		return 0;

	auto const pSpawner = pThis->Owner;

	if (!pSpawner || !pSpawned)
		return 0;

	auto const pSpawnerType = pSpawner->GetTechnoType();
	auto const spawnedMapCrd = pSpawned->GetMapCoords();

	if (!pSpawnerType)
		return 0;

	auto const pSpawnerExt = TechnoTypeExt::ExtMap.Find(pSpawnerType);

	if (!pSpawnerExt)
		return 0;

	auto const spawnerCrd = pSpawner->GetCoords();
	auto const spawnedCrd = pSpawned->GetCoords();
	auto const deltaCrd = spawnedCrd - spawnerCrd;
	const int recycleRange = pSpawnerExt->Spawner_RecycleRange;

	if (recycleRange < 0)
	{
		if (pSpawner->WhatAmI() == AbstractType::Building)
		{
			if (deltaCrd.X > 182 || deltaCrd.Y > 182 || deltaCrd.Z >= 20)
				return 0;
		}
		else if (spawnedMapCrd != *pSpawnerMapCrd || deltaCrd.Z >= 20)
		{
			return 0;
		}
	}
	else if (deltaCrd.Magnitude() > recycleRange)
	{
		return 0;
	}

	if (pSpawnerExt->Spawner_RecycleAnim)
		GameCreate<AnimClass>(pSpawnerExt->Spawner_RecycleAnim, spawnedCrd);

	pSpawned->SetLocation(spawnerCrd);
	R->EAX(pSpawnerMapCrd);
	return 0;
}

#pragma endregion

#pragma region ScatterFix

DEFINE_HOOK(0x481778, CellClass_ScatterContent_Fix, 0x6)
{
	enum { Continue = 0x481797, Scatter = 0x4817C3 };

	GET(TechnoClass* const, pTechno, ESI);
	GET_STACK(bool, force, STACK_OFFSET(0x2C, 0xC));

	return ((pTechno && pTechno->Owner->IsControlledByHuman() && RulesClass::Instance()->PlayerScatter) || force) ? Scatter : Continue;
}

#pragma endregion

#pragma region PlanWaypoint

DEFINE_HOOK(0x63745D, UnknownClass_PlanWaypoint_ContinuePlanningOnEnter, 0x6)
{
	enum { SkipDeselect = 0x637468 };

	GET(const int, planResult, ESI);

	return (!planResult && !RulesExt::Global()->StopPlanningOnEnter) ? SkipDeselect : 0;
}

DEFINE_HOOK(0x637479, UnknownClass_PlanWaypoint_DisableMessage, 0x5)
{
	enum { SkipMessage = 0x637524 };
	return (!RulesExt::Global()->StopPlanningOnEnter) ? SkipMessage : 0;
}

DEFINE_HOOK(0x638D73, UnknownClass_CheckLastWaypoint_ContinuePlanningWaypoint, 0x5)
{
	enum { SkipDeselect = 0x638D8D, Deselect = 0x638D82 };

	GET(const Action, action, EAX);

	if (!RulesExt::Global()->StopPlanningOnEnter)
		return SkipDeselect;
	else if (action == Action::Select || action == Action::ToggleSelect || action == Action::Capture)
		return Deselect;

	return SkipDeselect;
}

#pragma endregion

#pragma region TargetIronCurtain

DEFINE_HOOK(0x6FC22A, TechnoClass_GetFireError_TargetingIronCurtain, 0x6)
{
	enum { CantFire = 0x6FC86A, GoOtherChecks = 0x6FC24D };

	GET(TechnoClass*, pThis, ESI);
	GET(ObjectClass*, pTarget, EBP);
	GET_STACK(int, wpIdx, STACK_OFFSET(0x20, 0x8));

	if (!pTarget->IsIronCurtained())
		return GoOtherChecks;

	auto pOwner = pThis->Owner;
	auto const pRules = RulesExt::Global();

	if ((pOwner->IsHumanPlayer || pOwner->IsInPlayerControl) ? pRules->PlayerAttackIronCurtain : pRules->AIAttackIronCurtain)
		return GoOtherChecks;

	auto pWpExt = WeaponTypeExt::ExtMap.Find(pThis->GetWeapon(wpIdx)->WeaponType);
	bool isHealing = pThis->CombatDamage(wpIdx) < 0;

	return (pWpExt && pWpExt->AttackIronCurtain.Get(isHealing)) ? GoOtherChecks : CantFire;
}

#pragma endregion

#pragma region KeepTemporal

DEFINE_HOOK(0x6F50A9, TechnoClass_UpdatePosition_TemporalLetGo, 0x7)
{
	enum { LetGo = 0x6F50B4, SkipLetGo = 0x6F50B9 };

	GET(TechnoClass* const, pThis, ESI);
	GET(TemporalClass* const, pTemporal, ECX);

	if (!pTemporal || !pTemporal->Target)
		return SkipLetGo;

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	return (!pTypeExt || !pTypeExt->KeepWarping) ? LetGo : SkipLetGo;
}

DEFINE_HOOK(0x709A43, TechnoClass_EnterIdleMode_TemporalLetGo, 0x7)
{
	enum { LetGo = 0x709A54, SkipLetGo = 0x709A59 };

	GET(TechnoClass* const, pThis, ESI);
	GET(TemporalClass* const, pTemporal, ECX);

	if (!pTemporal || !pTemporal->Target)
		return SkipLetGo;

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	return (!pTypeExt || !pTypeExt->KeepWarping) ? LetGo : SkipLetGo;
}

// This is a fix to KeepWarping.
// But I think it has no difference with vanilla behavior, so no check for KeepWarping.
DEFINE_HOOK(0x4C7643, EventClass_RespondToEvent_StopTemporal, 0x6)
{
	GET(TechnoClass*, pTechno, ESI);

	auto const pTemporal = pTechno->TemporalImUsing;

	if (pTemporal && pTemporal->Target)
		pTemporal->LetGo();

	return 0;
}

DEFINE_HOOK(0x71A7A8, TemporalClass_Update_CheckRange, 0x6)
{
	enum { DontCheckRange = 0x71A84E, CheckRange = 0x71A7B4 };

	GET(TechnoClass*, pTechno, EAX);

	if (pTechno->InOpenToppedTransport)
		return CheckRange;

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pTechno->GetTechnoType());

	return (pTypeExt && pTypeExt->KeepWarping) ? CheckRange : DontCheckRange;
}

#pragma endregion

#pragma region AttackUnderGround

DEFINE_HOOK(0x70023B, TechnoClass_MouseOverObject_AttackUnderGround, 0x5)
{
	enum { FireIsOK = 0x700246, FireIsNotOK = 0x70056C };

	GET(ObjectClass*, pObject, EDI);
	GET(TechnoClass*, pThis, ESI);
	GET(int, wpIdx, EAX);

	if (pObject->IsSurfaced())
		return FireIsOK;

	auto const pWeapon = pThis->GetWeapon(wpIdx)->WeaponType;
	auto const pProjExt = pWeapon ? BulletTypeExt::ExtMap.Find(pWeapon->Projectile) : nullptr;

	return (!pProjExt || !pProjExt->AU) ? FireIsNotOK : FireIsOK;
}

#pragma endregion

#pragma region GuardRange

DEFINE_HOOK(0x4D6E83, FootClass_MissionAreaGuard_FollowStray, 0x6)
{
	enum { SkipGameCode = 0x4D6E8F };

	GET(FootClass* const, pThis, ESI);

	int range = RulesClass::Instance()->GuardModeStray;

	if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType()))
		range = pThis->Owner->IsControlledByHuman() ? pTypeExt->PlayerGuardModeStray.Get(Leptons(range)) : pTypeExt->AIGuardModeStray.Get(Leptons(range));

	R->EDI(range);
	return SkipGameCode;
}

DEFINE_HOOK(0x4D6E97, FootClass_MissionAreaGuard_Pursuit, 0x6)
{
	enum { KeepTarget = 0x4D6ED1, RemoveTarget = 0x4D6EB3 };

	GET(FootClass* const, pThis, ESI);
	GET(int, range, EDI);
	GET(AbstractClass* const, pFocus, EAX);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	bool isPlayer = pThis->Owner->IsControlledByHuman();
	bool pursuit = true;

	if (pTypeExt)
		pursuit = isPlayer ? pTypeExt->PlayerGuardModePursuit.Get(RulesExt::Global()->PlayerGuardModePursuit) : pTypeExt->AIGuardModePursuit.Get(RulesExt::Global()->AIGuardModePursuit);

	if ((pFocus->AbstractFlags & AbstractFlags::Foot) == AbstractFlags::None && pTypeExt)
	{
		Leptons stationaryStray = isPlayer ? pTypeExt->PlayerGuardStationaryStray.Get(RulesExt::Global()->PlayerGuardStationaryStray) : pTypeExt->AIGuardStationaryStray.Get(RulesExt::Global()->AIGuardStationaryStray);

		if (stationaryStray != Leptons(-256))
			range = stationaryStray;
	}

	if (pursuit)
	{
		if (!pThis->IsFiring && !pThis->Destination && pThis->DistanceFrom(pFocus) > range)
			return RemoveTarget;
	}
	else if (pThis->DistanceFrom(pFocus) > range)
	{
		return RemoveTarget;
	}

	return KeepTarget;
}

DEFINE_HOOK(0x707F08, TechnoClass_GetGuardRange_AreaGuardRange, 0x5)
{
	enum { SkipGameCode = 0x707E70 };

	GET(Leptons, guardRange, EAX);
	GET(int, mode, EDI);
	GET(TechnoClass* const, pThis, ESI);

	const bool isPlayer = pThis->Owner->IsControlledByHuman();
	auto const pRulesExt = RulesExt::Global();

	double multiplier = pRulesExt->PlayerGuardModeGuardRangeMultiplier;
	Leptons addend = pRulesExt->PlayerGuardModeGuardRangeAddend;

	if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType()))
	{
		multiplier = isPlayer ? pTypeExt->PlayerGuardModeGuardRangeMultiplier.Get(pRulesExt->PlayerGuardModeGuardRangeMultiplier) : pTypeExt->AIGuardModeGuardRangeMultiplier.Get(pRulesExt->AIGuardModeGuardRangeMultiplier);
		addend = isPlayer ? pTypeExt->PlayerGuardModeGuardRangeAddend.Get(pRulesExt->PlayerGuardModeGuardRangeAddend) : pTypeExt->AIGuardModeGuardRangeAddend.Get(pRulesExt->AIGuardModeGuardRangeAddend);
	}

	const Leptons areaGuardRange = Leptons(static_cast<int>(static_cast<int>(guardRange) * multiplier + static_cast<int>(addend)));
	const Leptons min = Leptons((mode == 2) ? 1792 : 0);
	const Leptons max = isPlayer ? pRulesExt->PlayerGuardModeGuardRangeMax : pRulesExt->AIGuardModeGuardRangeMax;

	R->EAX(Math::clamp(areaGuardRange, min, max));

	return SkipGameCode;
}

DEFINE_HOOK(0x4D6D34, FootClass_MissionAreaGuard_Miner, 0x5)
{
	enum { GuardArea = 0x4D6D69 };

	GET(FootClass*, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	return (pThis->Owner->IsControlledByHuman() && pTypeExt && pTypeExt->Harvester_CanGuardArea) ? GuardArea : 0;
}

#pragma endregion

#pragma region MissileSpawnFLH

DEFINE_HOOK(0x6B73EA, SpawnManagerClass_Update_MissileSpawnFLH, 0x5)
{
	enum { SkipCurrentBurstReset = 0x6B73FC };

	GET(SpawnManagerClass* const, pThis, ESI);
	GET(WeaponTypeClass* const, pWeaponType, EAX);
	GET(int, idx, EBX);

	auto const pSpawner = pThis->Owner;
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pSpawner->GetTechnoType());

	if (pTypeExt && pTypeExt->MissileSpawnUseOtherFLHs)
	{
		int burst = pWeaponType->Burst;
		pSpawner->CurrentBurstIndex = idx % burst;
		return SkipCurrentBurstReset;
	}

	return 0;
}

#pragma endregion

#pragma region RallyPointEnhancement

DEFINE_HOOK(0x44368D, BuildingClass_ObjectClickedAction_RallyPoint, 0x7)
{
	enum { OnTechno = 0x44363C };

	return RulesExt::Global()->RallyPointOnTechno ? OnTechno : 0;
}

DEFINE_HOOK(0x4473F4, BuildingClass_MouseOverObject_JustHasRallyPoint, 0x6)
{
	enum { JustRally = 0x447413 };

	GET(BuildingClass* const, pThis, ESI);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->JustHasRallyPoint) ? JustRally : 0;
}

DEFINE_HOOK(0x447413, BuildingClass_MouseOverObject_RallyPointForceMove, 0x5)
{
	enum { AlwaysAlt = 0x44744E };

	return RulesExt::Global()->RallyPointForceMove ? AlwaysAlt : 0;
}

DEFINE_HOOK(0x70000E, TechnoClass_MouseOverObject_RallyPointForceMove, 0x5)
{
	enum { AlwaysAlt = 0x700038 };

	GET(TechnoClass* const, pThis, ESI);

	if (pThis->WhatAmI() == AbstractType::Building && RulesExt::Global()->RallyPointForceMove)
	{
		auto const pType = static_cast<BuildingClass*>(pThis)->Type;
		auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pType);
		bool HasRallyPoint = (pTypeExt ? pTypeExt->JustHasRallyPoint : false) || pType->Factory == AbstractType::UnitType || pType->Factory == AbstractType::InfantryType || pType->Factory == AbstractType::AircraftType;
		return HasRallyPoint ? AlwaysAlt : 0;
	}

	return 0;
}

DEFINE_HOOK(0x44748E, BuildingClass_MouseOverObject_JustHasRallyPointAircraft, 0x6)
{
	enum { JustRally = 0x44749D };

	GET(BuildingClass* const, pThis, ESI);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->JustHasRallyPoint) ? JustRally : 0;
}

DEFINE_HOOK(0x447674, BuildingClass_MouseOverCell_JustHasRallyPoint, 0x6)
{
	enum { JustRally = 0x447683 };

	GET(BuildingClass* const, pThis, ESI);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->JustHasRallyPoint) ? JustRally : 0;
}

DEFINE_HOOK(0x447643, BuildingClass_MouseOverCell_RallyPointForceMove, 0x5)
{
	enum { AlwaysAlt = 0x447674 };

	return RulesExt::Global()->RallyPointForceMove ? AlwaysAlt : 0;
}

DEFINE_HOOK(0x700B28, TechnoClass_MouseOverCell_RallyPointForceMove, 0x6)
{
	enum { AlwaysAlt = 0x700B30 };

	GET(TechnoClass* const, pThis, ESI);

	if (pThis->WhatAmI() == AbstractType::Building && RulesExt::Global()->RallyPointForceMove)
	{
		auto const pType = static_cast<BuildingClass*>(pThis)->Type;
		auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pType);
		bool HasRallyPoint = (pTypeExt ? pTypeExt->JustHasRallyPoint : false) || pType->Factory == AbstractType::UnitType || pType->Factory == AbstractType::InfantryType || pType->Factory == AbstractType::AircraftType;

		return HasRallyPoint ? AlwaysAlt : 0;
	}

	return 0;
}

DEFINE_HOOK(0x455DA0, BuildingClass_IsUnitFactory_JustHasRallyPoint, 0x6)
{
	enum { SkipGameCode = 0x455DCC };

	GET(BuildingClass* const, pThis, ECX);

	auto const pTypeExt = BuildingTypeExt::ExtMap.Find(pThis->Type);

	return (pTypeExt && pTypeExt->JustHasRallyPoint) ? SkipGameCode : 0;
}

// Handle the rally of infantry.
DEFINE_HOOK(0x444CA3, BuildingClass_KickOutUnit_RallyPointAreaGuard1, 0x6)
{
	enum { SkipQueueMove = 0x444D11 };

	GET(BuildingClass*, pThis, ESI);
	GET(FootClass*, pProduct, EDI);

	if (!pThis->Owner->IsControlledByHuman() || !pProduct->Owner->IsControlledByHuman())
		return 0;

	if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetArchiveTarget(pThis->ArchiveTarget);
		pProduct->SetDestination(pThis->ArchiveTarget, true);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return 0;
}

// Vehicle but without BuildingClass::Unload calling, e.g. the building has WeaponsFactory = no set.
// Also fix the bug that WeaponsFactory = no will make the product ignore the rally point.
// Also fix the bug that WeaponsFactory = no will make the Jumpjet product park on the ground.
DEFINE_HOOK(0x4448CE, BuildingClass_KickOutUnit_RallyPointAreaGuard2, 0x6)
{
	enum { SkipGameCode = 0x4448F8 };

	GET(FootClass*, pProduct, EDI);
	GET(BuildingClass*, pThis, ESI);

	if (!pThis->Owner->IsControlledByHuman() || !pProduct->Owner->IsControlledByHuman())
		return 0;

	auto const pFocus = pThis->ArchiveTarget;
	auto const pUnit = abstract_cast<UnitClass*>(pProduct);
	bool isHarvester = pUnit ? pUnit->Type->Harvester : false;

	if (isHarvester)
	{
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Harvest, true);
	}
	else if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetArchiveTarget(pFocus);
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Area_Guard, true);
	}
	else
	{
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Move, true);
	}

	if (!pFocus && pProduct->GetTechnoType()->Locomotor == LocomotionClass::CLSIDs::Jumpjet)
		pProduct->Scatter(CoordStruct::Empty, false, false);

	return SkipGameCode;
}

// This makes the building has WeaponsFactory = no to kick out units in the cell same as WeaponsFactory = yes.
// Also enhanced the ExitCoord.
DEFINE_HOOK(0x4448B0, BuildingClass_KickOutUnit_ExitCoords, 0x6)
{
	GET(FootClass*, pProduct, EDI);
	GET(BuildingClass*, pThis, ESI);
	GET(CoordStruct*, pCrd, ECX);
	REF_STACK(DirType, dir, STACK_OFFSET(0x144,-0x100));

	auto const isJJ = pProduct->GetTechnoType()->Locomotor == LocomotionClass::CLSIDs::Jumpjet;
	auto const pProductType = pProduct->GetTechnoType();
	auto const buildingExitCrd = isJJ ? BuildingTypeExt::ExtMap.Find(pThis->Type)->JumpjetExitCoord.Get(pThis->Type->ExitCoord)
		: TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType())->ExitCoord.Get(pThis->Type->ExitCoord);
	auto const exitCrd = TechnoTypeExt::ExtMap.Find(pProductType)->ExitCoord.Get(buildingExitCrd);

	pCrd->X += exitCrd.X;
	pCrd->Y += exitCrd.Y;
	pCrd->Z += exitCrd.Z;

	if (!isJJ)
	{
		auto nCell = CellClass::Coord2Cell(*pCrd);
		auto const pCell = MapClass::Instance->GetCellAt(nCell);
		bool isBridge = pCell->ContainsBridge();
		nCell = MapClass::Instance->NearByLocation(CellClass::Coord2Cell(*pCrd),
			pProductType->SpeedType, -1, pProductType->MovementZone, isBridge, 1, 1, false,
			false, false, isBridge, nCell, false, false);
		*pCrd = CellClass::Cell2Coord(nCell, pCrd->Z);
	}

	dir = DirType::East;
	return 0;
}

// Ships.
DEFINE_HOOK(0x444424, BuildingClass_KickOutUnit_RallyPointAreaGuard3, 0x5)
{
	enum { SkipQueueMove = 0x44443F };

	GET(FootClass*, pProduct, EDI);
	GET(AbstractClass*, pFocus, ESI);

	if (!pProduct->Owner->IsControlledByHuman())
		return 0;

	auto const pUnit = abstract_cast<UnitClass*>(pProduct);
	const bool isHarvester = pUnit ? pUnit->Type->Harvester : false;

	if (RulesExt::Global()->RallyPointAreaGuard && !isHarvester)
	{
		pProduct->SetArchiveTarget(pFocus);
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return 0;
}

// For common aircrafts.
// Also make AirportBound aircraft not ignore the rally point.
DEFINE_HOOK(0x444061, BuildingClass_KickOutUnit_RallyPointAreaGuard4, 0x6)
{
	enum { SkipQueueMove = 0x444091, NotSkip = 0x444075 };

	GET(FootClass*, pProduct, EBP);
	GET(AbstractClass*, pFocus, ESI);

	if (!pProduct->Owner->IsControlledByHuman())
		return 0;

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pProduct->GetTechnoType());

	if (pTypeExt && pTypeExt->IgnoreRallyPoint)
		return SkipQueueMove;

	if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetArchiveTarget(pFocus);
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return NotSkip;
}

// For aircrafts with AirportBound = no and the airport is full.
// Still some other bug in it.
DEFINE_HOOK(0x443EB8, BuildingClass_KickOutUnit_RallyPointAreaGuard5, 0x5)
{
	enum { SkipQueueMove = 0x443ED3 };

	GET(FootClass*, pProduct, EBP);
	GET(AbstractClass*, pFocus, EAX);

	if (!pProduct->Owner->IsControlledByHuman())
		return 0;

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pProduct->GetTechnoType());

	if (pTypeExt && pTypeExt->IgnoreRallyPoint)
		return SkipQueueMove;

	if (RulesExt::Global()->RallyPointAreaGuard)
	{
		pProduct->SetArchiveTarget(pFocus);
		pProduct->SetDestination(pFocus, true);
		pProduct->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return 0;
}

// For unloaded units.
DEFINE_HOOK(0x73AAB3, UnitClass_UpdateMoving_RallyPointAreaGuard, 0x5)
{
	enum { SkipQueueMove = 0x73AAC1 };

	GET(UnitClass*, pThis, EBP);
	GET(AbstractClass*, pFocus, EAX);

	if (!pThis->Owner->IsControlledByHuman())
		return 0;

	if (RulesExt::Global()->RallyPointAreaGuard && !pThis->Type->Harvester)
	{
		pThis->SetArchiveTarget(pFocus);
		pThis->SetDestination(pFocus, true);
		pThis->QueueMission(Mission::Area_Guard, true);
		return SkipQueueMove;
	}

	return 0;
}

DEFINE_HOOK(0x4438C9, BuildingClass_SetRallyPoint_PathFinding, 0x6)
{
	GET(BuildingClass* const, pThis, EBP);
	GET(int, movementzone, ESI);
	GET_STACK(int, speedtype, STACK_OFFSET(0xA4, -0x84));

	auto const pExt = BuildingTypeExt::ExtMap.Find(pThis->Type);
	R->ESI(pExt->RallyMovementZone.Get(movementzone));
	R->Stack(STACK_OFFSET(0xA4, -0x84), pExt->RallySpeedType.Get(speedtype));

	return 0;
}

#pragma endregion

#pragma region CrushBuildingOnAnyCell

namespace CrushBuildingOnAnyCell
{
	CellClass* pCell;
}

DEFINE_HOOK(0x741733, UnitClass_CrushCell_SetContext, 0x6)
{
	GET(CellClass*, pCell, ESI);

	CrushBuildingOnAnyCell::pCell = pCell;

	return 0;
}

DEFINE_HOOK(0x741925, UnitClass_CrushCell_CrushBuilding, 0x5)
{
	GET(UnitClass*, pThis, EDI);

	if (RulesExt::Global()->CrushBuildingOnAnyCell)
	{
		if (auto const pBuilding = CrushBuildingOnAnyCell::pCell->GetBuilding())
		{
			if (reinterpret_cast<bool(__thiscall*)(BuildingClass*, TechnoClass*)>(0x5F6CD0)(pBuilding, pThis)) // IsCrushable
			{
				VocClass::PlayAt(pBuilding->Type->CrushSound, pThis->Location, 0);
				pBuilding->Destroy();
				pBuilding->RegisterDestruction(pThis);
				pBuilding->Mark(MarkType::Up);
				// pBuilding->Limbo(); // Vanilla do this. May be not necessary?
				pBuilding->UnInit();

				R->AL(true);
			}
		}
	}

	return 0;
}

#pragma endregion

#pragma region FollowTargetSelf

DEFINE_HOOK(0x4D9620, FootClass_SetDestination_FollowTargetSelf, 0x5)
{
	enum { SkipGameCode = 0x4D962B };

	GET(AbstractClass*, pDestination, ECX);

	if (RulesExt::Global()->FollowTargetSelf && pDestination->WhatAmI() != AbstractType::Building)
	{
		auto crd = pDestination->GetCoords();
		R->EAX(&crd);
		return SkipGameCode;
	}

	return 0;
}

#pragma endregion

#pragma region Sink

DEFINE_HOOK(0x7364DC, UnitClass_Update_SinkSpeed, 0x7)
{
	GET(UnitClass* const, pThis, ESI);

	GET(int, CoordZ, EDX);

	if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type))
		R->EDX(CoordZ - (pTypeExt->SinkSpeed - 5));

	return 0;
}

DEFINE_HOOK(0x737DE2, UnitClass_ReceiveDamage_Sinkable, 0x6)
{
	enum { GoOtherChecks = 0x737E18, NoSink = 0x737E63 };

	GET(UnitTypeClass*, pType, EAX);

	bool ShouldSink = pType->Weight > RulesClass::Instance->ShipSinkingWeight && pType->Naval && !pType->Underwater && !pType->Organic;

	if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType))
		ShouldSink = pTypeExt->Sinkable.Get(ShouldSink);

	return ShouldSink ? GoOtherChecks : NoSink;
}

#pragma endregion

#pragma region JumpjetSpeedType

namespace JumpjetSpeedType
{
	int speedType;
}

DEFINE_HOOK(0x54B255, JumpjetLocomotionClass_MoveTo_JumpjetSpeedType, 0x5)
{
	GET(ILocomotionPtr, pThis, ESI);

	if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(locomotion_cast<JumpjetLocomotionClass*>(pThis)->LinkedTo->GetTechnoType()))
		JumpjetSpeedType::speedType = pTypeExt->JumpjetSpeedType;

	return 0;
}

DEFINE_HOOK(0x56DC20, MapClass_NearByLocation_JumpjetSpeedType, 0x6)
{
	if (*R->ESP<int*>() == 0x54B374) // Ret_in_JJLoco_MoveTo
		R->Stack(STACK_OFFSET(0, 0xC), JumpjetSpeedType::speedType);

	return 0;
}

#pragma endregion

#pragma region AttackMove

DEFINE_HOOK(0x4DF410, FootClass_UpdateAttackMove_TargetAcquired, 0x6)
{
	GET(FootClass* const, pThis, ESI);

	auto const pType = pThis->GetTechnoType();

	if (auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType))
	{
		if (pThis->IsCloseEnoughToAttack(pThis->Target)
			&& pTypeExt->AttackMove_StopWhenTargetAcquired.Get(RulesExt::Global()->AttackMove_StopWhenTargetAcquired.Get(!pType->OpportunityFire)))
		{
			if (auto const pJumpjetLoco = locomotion_cast<JumpjetLocomotionClass*>(pThis->Locomotor))
			{
				auto crd = pThis->GetCoords();
				pJumpjetLoco->DestinationCoords.X = crd.X;
				pJumpjetLoco->DestinationCoords.Y = crd.Y;
				pJumpjetLoco->CurrentSpeed = 0;
				pJumpjetLoco->MaxSpeed = 0;
				pThis->AbortMotion();
			}
			else
			{
				pThis->StopMoving();
				pThis->AbortMotion();
			}
		}

		if (pTypeExt->AttackMove_PursuitTarget)
			pThis->SetDestination(pThis->Target, true);
	}

	return 0;
}

DEFINE_HOOK(0x711E90, TechnoTypeClass_CanAttackMove_IgnoreWeapon, 0x6)
{
	enum { SkipGameCode = 0x711E9A };
	return RulesExt::Global()->AttackMove_IgnoreWeaponCheck ? SkipGameCode : 0;
}

DEFINE_HOOK(0x4DF3A6, FootClass_UpdateAttackMove_Follow, 0x6)
{
	enum { FuncRet = 0x4DF425 };

	GET(FootClass* const, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());

	if (pTypeExt && pTypeExt->AttackMove_Follow)
	{
		auto pTechnoVectors = Helpers::Alex::getCellSpreadItems(pThis->GetCoords(), pThis->GetGuardRange(2) / 256.0, pTypeExt->AttackMove_Follow_IncludeAir);
		TechnoClass* pClosestTarget = nullptr;
		int closestRange = 65536;

		for (auto pTechno : pTechnoVectors)
		{
			if ((pTechno->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None &&
				pTechno != pThis && pTechno->Owner == pThis->Owner &&
				pTechno->MegaMissionIsAttackMove())
			{
				auto const pTargetExt = TechnoExt::ExtMap.Find(pTechno);

				// Check this to prevent the followed techno from being surrounded
				if (!pTargetExt || pTargetExt->AttackMoveFollowerTempCount >= 6)
					continue;

				auto const pTargetTypeExt = pTargetExt->TypeExtData;

				if (pTargetTypeExt && !pTargetTypeExt->AttackMove_Follow)
				{
					auto const dist = pTechno->DistanceFrom(pThis);

					if (dist < closestRange)
					{
						pClosestTarget = pTechno;
						closestRange = dist;
					}
				}
			}
		}

		if (pClosestTarget)
		{
			auto const pTargetExt = TechnoExt::ExtMap.Find(pClosestTarget);
			pTargetExt->AttackMoveFollowerTempCount += pThis->WhatAmI() == AbstractType::Infantry ? 1 : 3;
			pThis->SetDestination(pClosestTarget, false);
			pThis->SetArchiveTarget(pClosestTarget);
			pThis->QueueMission(Mission::Area_Guard, true);
		}
		else
		{
			if (pThis->MegaTarget)
				pThis->SetDestination(pThis->MegaTarget, false);
			else if (pThis->MegaDestination)
				pThis->SetDestination(pThis->MegaDestination, false);
			else
				pThis->SetDestination(nullptr, false);
		}

		pThis->ClearMegaMissionData();

		R->EAX(pClosestTarget);
		return FuncRet;
	}

	return 0;
}

#pragma endregion

#pragma region EventListOverflow

DEFINE_HOOK(0x6FFE00, TechnoClass_ClickedEvent_CacheClickedEvent, 0x5)
{
	GET(TechnoClass*, pThis, ECX);
	GET_STACK(EventType, event, 0x4);

	if (EventClass::OutList->Count >= 128)
	{
		auto const pExt = TechnoExt::ExtMap.Find(pThis);
		pExt->HasCachedClickEvent = true;
		pExt->CachedEventType = event;
		// one cache at a time
		pExt->HasCachedClickMission = false;
		pExt->CachedMission = Mission::None;
		pExt->CachedCell = nullptr;
		pExt->CachedTarget = nullptr;
	}

	return 0;
}

DEFINE_HOOK(0x6FFDA5, TechnoClass_ClickedMission_CacheClickedMission, 0x7)
{
	GET_STACK(AbstractClass* const, pCell, STACK_OFFSET(0x98, 0xC));
	GET(TechnoClass* const, pThis, ESI);
	GET(AbstractClass* const, pTarget, EBP);
	GET(Mission const, mission, EDI);

	if (EventClass::OutList->Count >= 128)
	{
		auto const pExt = TechnoExt::ExtMap.Find(pThis);
		pExt->HasCachedClickMission = true;
		pExt->CachedMission = mission;
		pExt->CachedCell = pCell;
		pExt->CachedTarget = pTarget;
		// one cache at a time
		pExt->HasCachedClickEvent = false;
		pExt->CachedEventType = EventType::LAST_EVENT;
	}

	return 0;
}

#pragma endregion

// TODO Self-made impl


















































#pragma region FallingDownDamage

DEFINE_HOOK(0x5F4032, ObjectClass_FallingDown_ToDead, 0x6)
{
	GET(ObjectClass*, pThis, ESI);

	if (const auto pTechno = abstract_cast<TechnoClass*>(pThis))
	{
		const auto pCell = pTechno->GetCell();

		if (!pCell || !pCell->IsClearToMove(pTechno->GetTechnoType()->SpeedType, true, true, -1, pTechno->GetTechnoType()->MovementZone, pCell->GetLevel(), pCell->ContainsBridge()))
			return 0;

		if (const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pTechno->GetTechnoType()))
		{
			double ratio = 0.0;

			if (pCell->Tile_Is_Water() && !pTechno->OnBridge)
				ratio = pTypeExt->FallingDownDamage_Water.Get(pTypeExt->FallingDownDamage.Get());
			else
				ratio = pTypeExt->FallingDownDamage.Get();

			int damage = 0;

			if (ratio < 0.0)
				damage = static_cast<int>(pThis->Health * abs(ratio));
			else if (ratio >= 0.0 && ratio <= 1.0)
				damage = static_cast<int>(pThis->GetTechnoType()->Strength * ratio);
			else
				damage = static_cast<int>(ratio);

			pThis->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, true, nullptr);

			if (pThis->Health > 0 && pThis->IsAlive)
			{
				pThis->IsABomb = false;

				if (pThis->WhatAmI() == AbstractType::Infantry)
				{
					const auto pInf = abstract_cast<InfantryClass*>(pTechno);

					if (pCell->Tile_Is_Water())
					{
						if (pInf->SequenceAnim != Sequence::Swim)
							pInf->PlayAnim(Sequence::Swim, true, false);
					}
					else if (pInf->SequenceAnim != Sequence::Guard)
					{
						pInf->PlayAnim(Sequence::Guard, true, false);
					}
				}
			}
			else
			{
				pTechno->UpdatePosition(PCPType::During);
			}

			return 0x5F405B;
		}
	}

	return 0;
}

#pragma endregion

// TODO Other contributors' impl
