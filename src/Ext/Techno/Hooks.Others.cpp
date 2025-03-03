#include "Body.h"

#include <EventClass.h>
#include <SpawnManagerClass.h>
#include <JumpjetLocomotionClass.h>
#include <HoverLocomotionClass.h>

#include <Ext/Building/Body.h>
#include <Ext/WeaponType/Body.h>
#include "Ext/BulletType/Body.h"
#include "Ext/WarheadType/Body.h"

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

	if (pThis->IsInAir())
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
