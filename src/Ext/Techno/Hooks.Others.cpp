#include "Body.h"

#include <EventClass.h>
#include <SpawnManagerClass.h>
#include <OverlayClass.h>
#include <TerrainClass.h>
#include <JumpjetLocomotionClass.h>
#include <DriveLocomotionClass.h>
#include <ShipLocomotionClass.h>
#include <HoverLocomotionClass.h>

#include <Ext/Building/Body.h>
#include "Ext/BulletType/Body.h"
#include <Ext/WeaponType/Body.h>
#include "Ext/WarheadType/Body.h"
#include <Ext/OverlayType/Body.h>
#include <Ext/TerrainType/Body.h>
#include <Utilities/AresHelper.h>
#include <Utilities/Helpers.Alex.h>
#include <Helpers/Macro.h>

#include <TacticalClass.h>
#include <Commands/FrameByFrame.h>

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

DEFINE_JUMP(LJMP, 0x445249, 0x445253);
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
		auto& pMap = MapClass::Instance;
		pMap.CellIteratorReset();

		for (auto pCell = pMap.CellIteratorNext(); pCell; pCell = pMap.CellIteratorNext())
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
/*
#pragma region HardLoco

DEFINE_HOOK_AGAIN(0x742A8C, UnitClass_SetDestination_PiggyBack, 0x8)
DEFINE_HOOK(0x742691, UnitClass_SetDestination_PiggyBack, 0x8)
{
	REF_STACK(const _GUID*, ID, STACK_OFFSET(0x9C, -0x9C));
	ID = &__uuidof(AdvancedDriveLocomotionClass);//&LocomotionClass::CLSIDs::Jumpjet;
	return 0;
}
*/
#pragma endregion

#pragma region BuildingUnloadFix
/*
static inline bool CanBuildingUnloadOccupants(BuildingClass* pThis)
{
	if (pThis->GetOccupantCount() <= 0)
		return false;

	const auto topLeftCell = pThis->GetMapCoords();
	const auto pOccupant = pThis->Occupants.GetItem(0);

	for (auto pFoundation = pThis->Type->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
	{
		if (const auto pSearchCell = MapClass::Instance.TryGetCellAt(topLeftCell + *pFoundation))
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

#pragma region DetectionLogic
/*
DEFINE_HOOK(0x5865E2, MapClass_IsLocationFogged_Check, 0x5)
{
	REF_STACK(CoordStruct*, pCoords, STACK_OFFSET(0x0, 0x4));

	const int level = pCoords->Z / Unsorted::LevelHeight;
	const int extra = (level & 1) ? ((level >> 1) + 1) : (level >> 1);
	const CellStruct cell { static_cast<short>((pCoords->X >> 8) - extra), static_cast<short>((pCoords->Y >> 8) - extra) };

	R->EAX(!(MapClass::Instance.GetCellAt(cell)->AltFlags & AltCellFlags::NoFog));
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

#pragma region SetHealthPercentageFix

DEFINE_HOOK(0x5F5C80, ObjectClass_SetHealthPercentage_Round, 0xA)
{
	enum { SkipGameCode = 0x5F5CBA };

	GET(ObjectClass* const, pThis, ECX);
	GET_STACK(double, percentage, STACK_OFFSET(0x0, 0x4));

	pThis->Health = (percentage <= 0.0) ? 0 : Math::max(1, Game::F2I(pThis->GetType()->Strength * percentage + 0.5));

	R->EAX(0);
	return SkipGameCode;
}

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

	if (auto const pOwner = (OwnerIdx != -1) ? HouseClass::Array.Items[OwnerIdx] : nullptr)
	{
		if (pOwner->IsAlliedWith(pThis->Owner)
			&& (!RulesExt::Global()->DestroyOwnerlessWalls
			|| !pOwner->IsNeutral()))
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

DEFINE_HOOK(0x6F755A, TechnoClass_IsCloseEnough_CylinderRangefinding, 0x7)
{
	GET_BASE(WeaponTypeClass* const, pWeaponType, 0x10);
	GET(CoordStruct* const, pCoord, ESI);
	GET(TechnoClass* const, pThis, EDI);
	const bool cylinder = WeaponTypeExt::ExtMap.Find(pWeaponType)->CylinderRangefinding.Get(RulesExt::Global()->CylinderRangefinding.Get());
	R->EAX(pCoord->X);
	return (cylinder || pThis->WhatAmI() == AbstractType::Aircraft) ? 0x6F75B2 : 0x6F7568;
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

	int range = RulesClass::Instance->GuardModeStray;

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

#pragma endregion

#pragma region MissileSpawnFLH

DEFINE_HOOK(0x6B73D2, SpawnManagerClass_Update_MissileSpawnFLH1, 0xA)
{
	GET(TechnoClass*, pOwner, ECX);

	auto pPrimary = pOwner->GetWeapon(0)->WeaponType;
	R->EAX(pPrimary->Spawner ? pPrimary : pOwner->GetWeapon(1)->WeaponType);
	return 0x6B73DE;
}

DEFINE_HOOK(0x6B73EA, SpawnManagerClass_Update_MissileSpawnFLH2, 0x5)
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

	return BuildingTypeExt::ExtMap.Find(pThis->Type)->JustHasRallyPoint ? JustRally : 0;
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
		bool HasRallyPoint = pTypeExt->JustHasRallyPoint || pType->Factory == AbstractType::UnitType || pType->Factory == AbstractType::InfantryType || pType->Factory == AbstractType::AircraftType;
		return HasRallyPoint ? AlwaysAlt : 0;
	}

	return 0;
}

DEFINE_HOOK(0x44748E, BuildingClass_MouseOverObject_JustHasRallyPointAircraft, 0x6)
{
	enum { JustRally = 0x44749D };

	GET(BuildingClass* const, pThis, ESI);

	return BuildingTypeExt::ExtMap.Find(pThis->Type)->JustHasRallyPoint ? JustRally : 0;
}

DEFINE_HOOK(0x447674, BuildingClass_MouseOverCell_JustHasRallyPoint1, 0x6)
{
	enum { JustRally = 0x447683 };

	GET(BuildingClass* const, pThis, ESI);

	return BuildingTypeExt::ExtMap.Find(pThis->Type)->JustHasRallyPoint ? JustRally : 0;
}

DEFINE_HOOK(0x447643, BuildingClass_MouseOverCell_JustHasRallyPoint2, 0x5)
{
	enum { JustRally = 0x447674 };

	GET(BuildingClass* const, pThis, ESI);

	return BuildingTypeExt::ExtMap.Find(pThis->Type)->JustHasRallyPoint ? JustRally : 0;
}

DEFINE_HOOK(0x700B28, TechnoClass_MouseOverCell_JustHasRallyPoint, 0x6)
{
	enum { JustRally = 0x700B30 };

	GET(TechnoClass* const, pThis, ESI);

	if (pThis->WhatAmI() == AbstractType::Building)
		return BuildingTypeExt::ExtMap.Find(static_cast<BuildingClass*>(pThis)->Type)->JustHasRallyPoint ? JustRally : 0;

	return 0;
}

DEFINE_HOOK(0x455DA0, BuildingClass_IsUnitFactory_JustHasRallyPoint, 0x6)
{
	enum { SkipGameCode = 0x455DCC };

	GET(BuildingClass* const, pThis, ECX);

	return BuildingTypeExt::ExtMap.Find(pThis->Type)->JustHasRallyPoint ? SkipGameCode : 0;
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
	auto const pUnit = abstract_cast<UnitClass*, true>(pProduct);
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
		auto const pCell = MapClass::Instance.GetCellAt(nCell);
		bool isBridge = pCell->ContainsBridge();
		nCell = MapClass::Instance.NearByLocation(CellClass::Coord2Cell(*pCrd),
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

	auto const pUnit = abstract_cast<UnitClass*, true>(pProduct);
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

void __fastcall KickOutClones(const BuildingExt::ExtData* const pThis, const TechnoClass* const pProduction)
{
	auto const pFactory = pThis->OwnerObject();
	auto const pFactoryType = pFactory->Type;

	if (pFactoryType->Cloning || (pFactoryType->Factory != InfantryTypeClass::AbsID && pFactoryType->Factory != UnitTypeClass::AbsID))
		return;

	auto pProductionType = pProduction->GetTechnoType();
	auto pProductionTypeExt = TechnoTypeExt::ExtMap.Find(pProductionType);

	if (!pProductionTypeExt->Cloneable)
		return;

	if (const auto clonedAs = pProductionTypeExt->ClonedAs)
	{
		pProductionType = clonedAs;
		pProductionTypeExt = TechnoTypeExt::ExtMap.Find(pProductionType);
	}

	auto const pFactoryOwner = pFactory->Owner;
	auto const& pCloningSources = pProductionTypeExt->ClonedAt;
	auto kickOutClone = [pProductionType, pFactoryOwner](BuildingClass* pBuilding) -> void
		{
			auto pClone = static_cast<TechnoClass*>(pProductionType->CreateObject(pFactoryOwner));

			if (pBuilding->KickOutUnit(pClone, CellStruct::Empty) != KickOutResult::Succeeded)
				pClone->UnInit();
		};

	auto const isUnit = (pFactoryType->Factory != InfantryTypeClass::AbsID);
	// keep cloning vats for backward compat, unless explicit sources are defined
	if (!isUnit && pCloningSources.empty())
	{
		for (auto const pCloningVat : pFactoryOwner->CloningVats)
			kickOutClone(pCloningVat);
	}

	// and clone from new sources
	if (!pCloningSources.empty() || isUnit)
	{
		for (auto const pBuilding : pFactoryOwner->Buildings)
		{
			if (pBuilding->InLimbo)
				continue;

			auto const pBuildingType = pBuilding->Type;
			auto shouldClone = false;

			if (!pCloningSources.empty())
				shouldClone = pCloningSources.Contains(pBuildingType);
			else if (isUnit)
				shouldClone = BuildingTypeExt::ExtMap.Find(pBuildingType)->CloningFacility && (pBuildingType->Naval == pFactoryType->Naval);

			if (shouldClone)
				kickOutClone(pBuilding);
		}
	}
}

DEFINE_HOOK(0x4448F8, BuildingClass_KickOutUnit_CloningFacilityFix, 0x6)
{
	GET(const BuildingClass* const, pThis, ESI);
	GET(const UnitClass* const, pUnit, EDI);

	--Unsorted::ScenarioInit;
	KickOutClones(BuildingExt::ExtMap.Find(pThis), pUnit);
	++Unsorted::ScenarioInit;

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

#pragma region JumpjetSpeedType

DEFINE_HOOK(0x54B36A, JumpjetLocomotionClass_MoveTo_JumpjetSpeedType, 0x5)
{
	GET(ILocomotion* const, iloco, ESI);
	REF_STACK(SpeedType, speedType, STACK_OFFSET(0x5C, -0x54));

	__assume(iloco != nullptr);
	const auto pLoco = static_cast<JumpjetLocomotionClass*>(iloco);
	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pLoco->LinkedTo->GetTechnoType());
	speedType = static_cast<SpeedType>(pTypeExt->JumpjetSpeedType.Get());

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
				pJumpjetLoco->State = JumpjetLocomotionClass::State::Hovering;
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
			if ((pTechno->AbstractFlags & AbstractFlags::Foot) != AbstractFlags::None
				&& pTechno != pThis && pTechno->Owner == pThis->Owner
				&& pTechno->MegaMissionIsAttackMove())
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

	if (EventClass::OutList.Count >= 128)
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

	if (EventClass::OutList.Count >= 128)
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

#pragma region UpdateReload

DEFINE_HOOK(0x51BDCF, InfantryClass_Update_Reload, 0x7)
{
	enum { SkipGameCode = 0x51BDD6 };

	GET(InfantryClass*, pThis, ESI);

	R->EAX(pThis->InWhichLayer());

	if (RulesExt::Global()->InTransportInfantryAmmoFix && AresHelper::CanUseAres && pThis->InLimbo && !TechnoTypeExt::ExtMap.Find(pThis->Type)->ReloadInTransport)
		pThis->Reload();

	return SkipGameCode;
}

#pragma endregion

#pragma region UpdateInLimbo

DEFINE_HOOK(0x522937, InfantryClass_EnterOccupyBuilding_KeepUpdate, 0xA)
{
	GET(InfantryClass*, pThis, ESI);
	GET_STACK(BuildingClass*, pBuilding, STACK_OFFSET(0x1C, 0x4));

	if (RulesExt::Global()->UpdateInLimbo_Occupier)
		LogicClass::Instance.AddObject(pThis, false);

	TechnoExt::ExtMap.Find(pThis)->BuildingOccupying = pBuilding;

	return 0;
}

DEFINE_HOOK(0x4733B6, PassengerClass_AddPassenger_KeepUpdate, 0x5)
{
	GET(InfantryClass*, pThis, ESI);

	if (RulesExt::Global()->UpdateInLimbo_NormalPassenger)
		LogicClass::Instance.AddObject(pThis, false);

	return 0;
}

DEFINE_HOOK(0x4DB87E, FootClass_SetLocation_UpdatePassengerLocation, 0x6)
{
	return RulesExt::Global()->UpdateInLimbo_NormalPassenger ? 0x4DB888 : 0;
}

DEFINE_HOOK(0x62A0A0, ParasiteClass_Update_UpdateParasiteLocation, 0x7)
{
	GET(ParasiteClass*, pThis, ESI);

	if (RulesExt::Global()->UpdateInLimbo_LimboLaunch)
		pThis->Owner->SetLocation(pThis->Victim->Location);

	return 0;
}

DEFINE_HOOK(0x6FF7F9, SITechnoClass_Fire_LimboLaunch, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	if (RulesExt::Global()->UpdateInLimbo_LimboLaunch)
		LogicClass::Instance.AddObject(pThis, false);

	return 0;
}

DEFINE_HOOK(0x4580B4, BuildingClass_OccupantLeaveAll_UpdateState, 0x5)
{
	GET(InfantryClass*, pOccupant, EDI);

	TechnoExt::ExtMap.Find(pOccupant)->BuildingOccupying = nullptr;

	return 0;
}

DEFINE_HOOK(0x6FC5B3, TechnoClass_GetFireError_InLimbo, 0x6)
{
	enum { Illegal = 0x6FC86A };

	GET(TechnoClass*, pThis, ESI);

	return !pThis->InLimbo || pThis->InOpenToppedTransport
		|| (RulesExt::Global()->UpdateInLimbo_Occupier && TechnoExt::ExtMap.Find(pThis)->BuildingOccupying)
		? 0 : Illegal;
}

#pragma endregion

#pragma region RadarDrawing

DEFINE_HOOK(0x47C329, CellClass_GetRadarColor_UnifiedRadarColor, 0x7)
{
	REF_STACK(ColorStruct, rgb1, STACK_OFFSET(0x14, -0x8));
	REF_STACK(ColorStruct, rgb2, STACK_OFFSET(0x14, -0xC));
	GET(CellClass*, pThis, ESI);

	const auto pRulesExt = RulesExt::Global();

	if (!pRulesExt->UnifiedRadarColor)
		return 0;

	if (pThis->Tile_Is_Cliff())
		rgb1 = pRulesExt->UnifiedRadarColor_Cliff;
	else if (pThis->Tile_Is_Water())
		rgb1 = pRulesExt->UnifiedRadarColor_Water;
	else
		rgb1 = pRulesExt->UnifiedRadarColor_Land;

	rgb2 = rgb1;

	return 0;
}

#pragma endregion

#pragma region UnifiedTechnoColor

DEFINE_HOOK(0x655F80, RadarClass_ProcessPoint_UnifiedRadarColor, 0x6)
{
	enum { SkipGameCode = 0x655FEB };

	GET_STACK(HouseClass*, pOwner, STACK_OFFSET(0x40, 0x4));

	if (!Phobos::Config::UnifiedTechnoColor)
		return 0;

	const auto pRulesExt = RulesExt::Global();
	int colorCode = 0;

	if (pOwner->Type->MultiplayPassive)
		colorCode = Drawing::RGB_To_Int(pRulesExt->UnifiedRadarColor_Neutral);
	else if (pOwner->IsControlledByCurrentPlayer())
		colorCode = Drawing::RGB_To_Int(pRulesExt->UnifiedRadarColor_Self);
	else if (HouseClass::CurrentPlayer->IsAlliedWith(pOwner))
		colorCode = Drawing::RGB_To_Int(pRulesExt->UnifiedRadarColor_Ally);
	else
		colorCode = Drawing::RGB_To_Int(pRulesExt->UnifiedRadarColor_Enemy);

	R->EBX(colorCode);
	return SkipGameCode;
}

DEFINE_HOOK(0x705D88, TechnoClass_GetRemapColour_UnifiedColor, 0x8)
{
	enum { SkipGameCode = 0x705DF1 };

	GET(TechnoClass*, pThis, ESI);
	GET(DynamicVectorClass<ColorScheme*>*, pPalette, EAX);

	if (!Phobos::Config::UnifiedTechnoColor)
		return 0;

	auto getOwner = [pThis]()
	{
		if (pThis->IsClearlyVisibleTo(HouseClass::CurrentPlayer))
			return pThis->Owner;

		if (const auto pDisguiseHouse = pThis->GetDisguiseHouse(true))
			return pDisguiseHouse;

		return pThis->Owner;
	};
	const auto pOwner = getOwner();

	auto getSchemeIdx = [pOwner]()
	{
		const auto pRulesExt = RulesExt::Global();

		if (pOwner->Type->MultiplayPassive)
			return pRulesExt->UnifiedTechnoColor_NeutralColorIdx == -1 ? ColorScheme::FindIndex("LightGrey") : pRulesExt->UnifiedTechnoColor_NeutralColorIdx;
		else if (pOwner->IsControlledByCurrentPlayer())
			return pRulesExt->UnifiedTechnoColor_SelfColorIdx == -1 ? ColorScheme::FindIndex("Green") : pRulesExt->UnifiedTechnoColor_SelfColorIdx;
		else if (HouseClass::CurrentPlayer->IsAlliedWith(pOwner))
			return pRulesExt->UnifiedTechnoColor_AllyColorIdx == -1 ? ColorScheme::FindIndex("Gold") : pRulesExt->UnifiedTechnoColor_AllyColorIdx;

		return pRulesExt->UnifiedTechnoColor_EnemyColorIdx == -1 ? ColorScheme::FindIndex("Red") : pRulesExt->UnifiedTechnoColor_EnemyColorIdx;
	};
	const int unifiedColorScheme = getSchemeIdx();
	const int colorSchemeIdx = unifiedColorScheme != -1 ? unifiedColorScheme : pOwner->ColorSchemeIndex;

	R->ECX(pPalette ? pPalette->Items[colorSchemeIdx] : ColorScheme::Array.Items[colorSchemeIdx]);
	return SkipGameCode;
}

#pragma endregion

#pragma region VisualCharacter

namespace VisualCharacterContext
{
	TechnoClass* pThis = nullptr;
	bool specificOwner = false;
	HouseClass* pWatcher = nullptr;
}

// Set context
DEFINE_HOOK(0x703860, TechnoClass_VisualCharacter_Start, 0x6)
{
	GET(TechnoClass* const, pThis, ECX);
	GET_STACK(const bool, specificOwner, STACK_OFFSET(0, 0x4));
	GET_STACK(HouseClass* const, pWatcher, STACK_OFFSET(0, 0x8));

	VisualCharacterContext::pThis = pThis;
	VisualCharacterContext::specificOwner = specificOwner;
	VisualCharacterContext::pWatcher = pWatcher;

	return 0;
}

DEFINE_HOOK(0x703B0B, TechnoClass_VisualCharacter_Normal, 0x5)
{
	if (const HouseClass* const pWatcher = VisualCharacterContext::specificOwner ? VisualCharacterContext::pWatcher : HouseClass::CurrentPlayer)
	{
		const auto pThis = VisualCharacterContext::pThis;
		const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
		const auto pOwner = pThis->Owner;
		const auto defaultValue = pTypeExt->DefaultVisualCharacter;

		if (pOwner == pWatcher)
			R->EAX(static_cast<VisualType>(pTypeExt->DefaultVisualCharacterToSelf.Get(defaultValue)));
		else if (pOwner->IsAlliedWith(pWatcher))
			R->EAX(static_cast<VisualType>(pTypeExt->DefaultVisualCharacterToAlly.Get(defaultValue)));
		else
			R->EAX(static_cast<VisualType>(pTypeExt->DefaultVisualCharacterToEnemy.Get(defaultValue)));
	}
	else
	{
		R->EAX(VisualType::Normal);
	}

	return 0;
}

#pragma endregion

#pragma region IgnoreByMouse

static inline bool ShouldIgnoreByMouse(ObjectClass* pObject)
{
	const auto pType = pObject->GetType();

	if (!pType)
		return true;

	if (pObject->AbstractFlags & AbstractFlags::Techno)
	{
		const auto pTechno = static_cast<TechnoClass*>(pObject);
		const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pTechno->GetTechnoType());
		const auto pOwner = pTechno->Owner;
		const auto defaultValue = pTypeExt->IgnoredByMouse;

		if (pOwner == HouseClass::CurrentPlayer)
			return pTypeExt->IgnoredByMouse_ToSelf.Get(defaultValue);
		else if (pOwner->IsAlliedWith(HouseClass::CurrentPlayer))
			return pTypeExt->IgnoredByMouse_ToAlly.Get(defaultValue);

		return pTypeExt->IgnoredByMouse_ToEnemy.Get(defaultValue);
	}

	const auto absType = pObject->WhatAmI();

	if (absType == OverlayClass::AbsID)
	{
		const auto pOverlay = static_cast<OverlayClass*>(pObject);
		const auto pTypeExt = OverlayTypeExt::ExtMap.Find(pOverlay->Type);
		return pTypeExt->IgnoredByMouse.Get();
	}
	else if (absType == TerrainClass::AbsID)
	{
		const auto pTerrain = static_cast<TerrainClass*>(pObject);
		const auto pTypeExt = TerrainTypeExt::ExtMap.Find(pTerrain->Type);
		return pTypeExt->IgnoredByMouse.Get();
	}

	return false;
}

DEFINE_HOOK(0x6DA3D2, TacticalClass_GetObjectOnCrd_IgnoredByMouse1, 0x8)
{
	enum { Ignore = 0x6DA491, DontIgnore = 0x6DA3DA };

	GET(ObjectClass* const, pObject, ESI);

	return !pObject || ShouldIgnoreByMouse(pObject) ? Ignore : DontIgnore;
}

DEFINE_HOOK(0x6DA4FB, TacticalClass_GetObjectOnCrd_IgnoredByMouse2, 0x6)
{
	enum { SkipGameCode = 0x6DA501 };

	GET(CellClass* const, pCell, EAX);

	ObjectClass* pFoundObject = nullptr;

	for (auto pOccupier = pCell->FirstObject; pOccupier; pOccupier = pOccupier->NextObject)
	{
		if (ShouldIgnoreByMouse(pOccupier))
			continue;

		// find first non-transparent to mouse techno and return it
		if (const auto pExt = TechnoExt::ExtMap.Find(abstract_cast<TechnoClass*, true>(pOccupier)))
		{
			if (pExt->ParentAttachment && pExt->ParentAttachment->GetType()->TransparentToMouse)
				continue;
		}

		pFoundObject = pOccupier;
		break;
	}

	R->EAX(pFoundObject);
	return SkipGameCode;
}

#pragma endregion

#pragma region HealingWeaponFix

// Skip the hardcode of healing weapon auto target range.
DEFINE_JUMP(LJMP, 0x6F9024, 0x6F9042);

DEFINE_HOOK(0x6FA9D8, TechnoClass_Update_FixRepairWeapon, 0x6)
{
	enum { SkipGameCode = 0x6FAA6F };

	GET(TechnoClass*, pThis, ESI);

	if (pThis->Target && pThis->CombatDamage(-1) < 0)
	{
		if ((SessionClass::IsCampaign() ? pThis->Owner->IsControlledByCurrentPlayer() : pThis->Owner->IsHumanPlayer) && !pThis->Owner->IsAlliedWith(pThis->Target))
			pThis->SetTarget(nullptr);
		else if (pThis->CurrentMission == Mission::Guard && !pThis->IsCloseEnoughToAttack(pThis->Target))
			pThis->SetTarget(nullptr);
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x707ED0, TechnoClass_GetGuardRange_FixForIFV, 0x6)
{
	enum { SkipGameCode = 0x707F08 };

	GET(TechnoClass*, pThis, ESI);

	auto pType = pThis->GetTechnoType();

	if (!pType->HasMultipleTurrets() || pType->IsGattling)
		return 0;

	R->EAX(pThis->GetWeaponRange(pThis->CurrentWeaponNumber));
	return SkipGameCode;
}

#pragma endregion

#pragma region SharedControl

DEFINE_HOOK(0x50B716, HouseClass_IsCurrentPlayer_SharedControl, 0x6)
{
	GET(HouseClass*, pThis, ECX);
	R->EAX(RulesExt::Global()->AllyShareControl ? pThis->IsAlliedWith(HouseClass::CurrentPlayer) : pThis == HouseClass::CurrentPlayer);
	return 0x50B723;
}

#pragma endregion

#pragma region ExtraTargeting

static inline bool ExtraTargeting_Range(TechnoClass* pThis)
{
	if (!RulesExt::Global()->ExtraTargeting || pThis->Spawned || pThis->SpawnOwner)
		return false;

	auto coord = pThis->GetCoords();

	pThis->ShouldLoseTargetNow = true;
	bool HasTarget = pThis->TargetAndEstimateDamage(coord, ThreatType::Range);
	pThis->ShouldLoseTargetNow = HasTarget;

	return HasTarget;
}

DEFINE_HOOK(0x4C7655, EventClass_RespondToEvent_ExtraTargeting_Idle, 0x7)
{
	enum { SkipGameCode = 0x4C765C };

	GET(TechnoClass*, pTechno, ESI);

	ExtraTargeting_Range(pTechno);

	R->EAX(pTechno->WhatAmI());
	return SkipGameCode;
}

DEFINE_HOOK(0x4D4E72, FootClass_MissionAttack_ExtraTargeting, 0x6)
{
	enum { ApproachTarget = 0x4D4E64 };

	GET(FootClass*, pThis, ESI);

	return pThis->MegaMissionIsAttackMove() && ExtraTargeting_Range(pThis) ? ApproachTarget : 0;
}

DEFINE_HOOK(0x44AF90, BuildingClass_MissionAttack_ExtraTargeting, 0x5)
{
	enum { AttackTarget = 0x44AFED };

	GET(BuildingClass*, pThis, ESI);

	return ExtraTargeting_Range(pThis) ? AttackTarget : 0;
}

DEFINE_HOOK(0x417FE0, AircraftClass_MissionAttack_ExtraTargeting, 0x6)
{
	GET(AircraftClass*, pThis, ECX);

	if (!pThis->Target && pThis->MegaMissionIsAttackMove())
		ExtraTargeting_Range(pThis);

	return 0;
}

DEFINE_HOOK(0x4C7462, EventClass_RespondToEvent_ExtraTargeting_MegaMission, 0x5)
{
	enum { SkipGameCode = 0x4C74C0, SkipSetTarget = 0x4C746D };

	GET(TechnoClass*, pTechno, EDI);
	GET(EventClass*, pThis, ESI);

	auto const mission = static_cast<Mission>(pThis->MegaMission.Mission);

	if (pTechno->WhatAmI() == AbstractType::Unit)
	{
		auto const pExt = TechnoExt::ExtMap.Find(pTechno);

		if (mission == Mission::Move && pExt->TypeExtData->KeepTargetOnMove && pTechno->Target)
		{
			GET(AbstractClass*, pTarget, EBX);

			if (!pTarget && pTechno->IsCloseEnoughToAttack(pTechno->Target))
			{
				auto const pDestination = pThis->MegaMission.Destination.As_Abstract();
				pTechno->SetDestination(pDestination, true);
				pExt->KeepTargetOnMove = true;

				return SkipGameCode;
			}
		}

		pExt->KeepTargetOnMove = false;
	}

	return ((mission == Mission::Move || mission == Mission::Harvest)
		&& pTechno->GetTechnoType()->OpportunityFire
		&& ExtraTargeting_Range(pTechno))
		? SkipSetTarget
		: 0;
}

// Current target may hurt me.
static inline bool IsAThreatToMe(TechnoClass* const pTechno, AbstractClass* const pTarget, int weaponIndex = -1)
{
	if (const auto pTechnoTarget = abstract_cast<TechnoClass*>(pTarget))
	{
		if (weaponIndex < 0)
			weaponIndex = pTechnoTarget->SelectWeapon(pTechno);

		if (!pTechnoTarget->GetWeapon(weaponIndex)->WeaponType)
			return false;

		const auto error = pTechnoTarget->GetFireError(pTechno, weaponIndex, true);
		return pTechnoTarget->WhatAmI() == AbstractType::Building ? (error != FireError::ILLEGAL) && (error != FireError::RANGE) : (error != FireError::ILLEGAL);
	}

	return false;
}

DEFINE_HOOK(0x70CE85, TechnoClass_ThreatCoefficient_CanAttackMeThreatBonus, 0x5)
{
	GET(TechnoClass* const, pThis, EDI);
	GET(TechnoClass* const, pTarget, ESI);
	GET(const int, weaponIndex, EAX);
	REF_STACK(double, totalThreat, STACK_OFFSET(0x58, -0x48));

	if (IsAThreatToMe(pThis, pTarget, weaponIndex))
		totalThreat += RulesExt::Global()->CanAttackMeThreatBonus;

	return 0;
}

DEFINE_HOOK(0x709918, TechnoClass_TargetAndEstimateDamage_CheckTarget, 0x6)
{
	enum { CanTargeting = 0x709926 };
	// GET(TechnoClass* const, pThis, ESI);
	return RulesExt::Global()->ExtraTargeting ? CanTargeting : 0;
}

DEFINE_HOOK(0x709957, TechnoClass_TargetAndEstimateDamage_SetTarget, 0x6)
{
	enum { SkipSetTarget = 0x709966, SkipSetTargetAndEstimateHealth = 0x7099B8 };

	GET(AbstractClass*, pTarget, EDI);
	GET(TechnoClass*, pThis, ESI);

	if (RulesExt::Global()->ExtraTargeting ? (pThis->QueuedMission != Mission::Attack) : (pTarget != nullptr))
		pThis->SetTarget(pTarget);

	return RulesExt::Global()->VHPScan_Enhanced ? SkipSetTargetAndEstimateHealth : SkipSetTarget;
}

DEFINE_HOOK(0x7079D1, TechnoClass_PointerExpired_TargetExpired, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	if(RulesExt::Global()->ExtraTargeting)
		pThis->TargetingTimer.TimeLeft = 0;

	return 0;
}

#pragma endregion

#pragma region VHPScan

static inline void CheckVHPScanAndRetarget(TechnoClass* pThis)
{
	if (!RulesExt::Global()->VHPScan_Enhanced)
		return;

	const auto pType = pThis->GetTechnoType();

	if (pType->VHPScan != 2)
		return;

	const auto pTargetTechno = abstract_cast<TechnoClass*>(pThis->Target);

	if (!pTargetTechno || pTargetTechno->EstimatedHealth > 0)
		return;

	pThis->SetTarget(nullptr);

	if (!RulesExt::Global()->ExtraTargeting)
		return;

	if (pThis->CurrentMission == Mission::Attack)
		pThis->QueueMission(pThis->MegaMissionIsAttackMove() ? Mission::Attack : (pType->DefaultToGuardArea ? Mission::Area_Guard : Mission::Guard), true);

	ExtraTargeting_Range(pThis);
}

DEFINE_HOOK(0x5206B7, InfantryClass_UpdateFiring_Start, 0x6)
{
	GET(InfantryClass*, pThis, EBP);
	CheckVHPScanAndRetarget(pThis);
	return 0;
}

DEFINE_HOOK(0x736DF8, UnitClass_UpdateFiring_Start, 0x6)
{
	GET(UnitClass*, pThis, ESI);
	CheckVHPScanAndRetarget(pThis);
	return 0;
}

DEFINE_HOOK(0x44ACF0, BuildingClass_MissionAttack_Start, 0x6)
{
	GET(BuildingClass*, pThis, ECX);
	CheckVHPScanAndRetarget(pThis);
	return 0;
}

DEFINE_HOOK(0x417FF1, AircraftClass_MissionAttack_Start, 0x6)
{
	GET(AircraftClass*, pThis, ESI);
	CheckVHPScanAndRetarget(pThis);
	return 0;
}

DEFINE_HOOK(0x6F7D0D, TechnoClass_CanAutoTargetObject_VHPScanStrong, 0x6)
{
	enum { SkipGameCode = 0x6F7D19 };
	return RulesExt::Global()->VHPScan_Enhanced ? SkipGameCode : 0;
}

DEFINE_HOOK(0x6F8721, TechnoClass_CanAutoTargetObject_VHPScanThreat, 0x7)
{
	enum { SkipGameCode = 0x6F875F };

	GET(TechnoClass*, pThis, EDI);
	GET(ObjectClass*, pTarget, ESI);
	GET_STACK(int*, pThreat, STACK_OFFSET(0x3C, 0x14));

	if (RulesExt::Global()->VHPScan_Enhanced && pThis->GetTechnoType()->VHPScan == 2)
	{
		if (pTarget->EstimatedHealth <= 0)
			*pThreat /= 10;

		return SkipGameCode;
	}

	return 0;
}

#pragma endregion

#pragma region Activate

DEFINE_HOOK(0x70FC85, TechnoClass_Activate_End, 0x5)
{
	GET(TechnoClass*, pThis, ECX);

	if (!pThis->Deactivated && TechnoExt::ExtMap.Find(pThis)->IsWreckage)
		pThis->Deactivate();

	return 0;
}

#pragma region

#pragma region TunnelDist

DEFINE_HOOK(0x74608F, UnitClass_AStarAttempt_SimpleTooFar, 0x5)
{
	enum { GoUnderground = 0x7460F4, GoSurface = 0x746094 };
	GET(const int, simpleDist, EAX);
	return simpleDist >= RulesExt::Global()->TunnelSimpleDistTooFar ? GoUnderground : GoSurface;
}

DEFINE_HOOK(0x7460EC, UnitClass_AStarAttempt_PathingTooFar, 0x5)
{
	GET(const int, pathingDist, EAX);
	R->DL(pathingDist > RulesExt::Global()->TunnelPathingDistTooFar);
	return 0;
}

#pragma region

// TODO Self-made impl


















































#pragma region NoManualEject

DEFINE_HOOK(0x73D6EC, UnitClass_Unload_NoManualEject, 0x6)
{
	enum { NoEject = 0x73DCD3 };
	GET(TechnoTypeClass* const, pType, EAX);
	return TechnoTypeExt::ExtMap.Find(pType)->NoManualEject.Get() ? NoEject : 0;
}

DEFINE_HOOK(0x740015, UnitClass_WhatAction_NoManualEject, 0x6)
{
	enum { NoEject = 0x7400F0 };
	GET(TechnoTypeClass* const, pType, EAX);
	return TechnoTypeExt::ExtMap.Find(pType)->NoManualEject.Get() ? NoEject : 0;
}

#pragma endregion

#pragma region NoManualEject

static inline InfantryClass* CreateInfantryFromFactory(InfantryTypeClass* pType, HouseClass* pOwner)
{
	// BuildLimit check goes before creation
	if (pType->BuildLimit > 0)
	{
		int sum = pOwner->CountOwnedNow(pType);

		// copy Ares' deployable units x build limit fix
		if (auto const pUndeploy = pType->UndeploysInto)
			sum += pOwner->CountOwnedNow(pUndeploy);

		if (sum >= pType->BuildLimit)
			return nullptr;
	}

	return static_cast<InfantryClass*>(pType->CreateObject(pOwner));
}

DEFINE_HOOK(0x444DDF, BuildingClass_KickOutUnit_InfantrySquad, 0x5)
{
	GET(TechnoClass*, pTechno, EDI);

	if (const auto pInfantry = abstract_cast<InfantryClass*, true>(pTechno))
	{
		const auto pExt = TechnoExt::ExtMap.Find(pInfantry);
		const auto pTypeExt = pExt->TypeExtData;

		if (pTypeExt->Squad_IsInitAsTeam)
		{
			pExt->SquadManager = SquadManagerClass::Allocate();
			pExt->SquadManager->AddMember(pInfantry);
		}

		const int size = pTypeExt->Squad_Members.size();

		if (size > 0)
		{
			for (int i = 0; i < size; ++i)
			{
				if (const auto pMember = CreateInfantryFromFactory(pTypeExt->Squad_Members[i], pInfantry->Owner))
				{
					++Unsorted::ScenarioInit;
					pMember->Unlimbo(pInfantry->Location, DirType::North);
					pMember->SetDestination(pInfantry, true);
					pMember->SetArchiveTarget(pInfantry);
					pMember->QueueMission(Mission::Area_Guard, 0);
					--Unsorted::ScenarioInit;

					if (pExt->SquadManager)
					{
						TechnoExt::ExtMap.Find(pMember)->SquadManager = pExt->SquadManager;
						pExt->SquadManager->AddMember(pMember);
					}
				}
			}
		}
	}

	return 0x444971;
}

DEFINE_HOOK(0x6FBFD0, TechnoClass_Select_SquadSelect, 0x5)
{
	GET(TechnoClass*, pTechno, ESI);

	if (const auto pSquadManager = TechnoExt::ExtMap.Find(pTechno)->SquadManager)
	{
		if (!pSquadManager->Select)
		{
			pSquadManager->Select = true;

			for (auto& pMember : pSquadManager->Members)
			{
				if (pMember != pTechno)
					pMember->Select();
			}

			pSquadManager->Select = false;
		}
	}

	return 0;
}

DEFINE_HOOK(0x737BBE, UnitClass_Unlimbo_CreatePassengerSquad, 0x6)
{
	GET(UnitClass*, pThis, ESI);

	const auto pExt = TechnoExt::ExtMap.Find(pThis);

	if (pExt->TypeExtData->Squad_IsInitAsTeam)
	{
		if (pThis->Passengers.NumPassengers > 0)
		{
			pExt->SquadManager = SquadManagerClass::Allocate();
			pExt->SquadManager->AddMember(pThis);

			for (auto pPassenger = pThis->Passengers.GetFirstPassenger(); pPassenger; pPassenger = abstract_cast<FootClass*>(pPassenger->NextObject))
			{
				TechnoExt::ExtMap.Find(pPassenger)->SquadManager = pExt->SquadManager;
				pExt->SquadManager->AddMember(pPassenger);
			}
		}
	}

	return 0;
}

#pragma endregion

// TODO Other contributors' impl











#pragma region DebugLogInGreatestThreat

DEFINE_HOOK(0x6F9C80, TechnoClass_GreatestThreat_LogDeadInTechnoArray, 0x9)
{
	enum { Continue = 0x6F9C89, NextOne = 0x6F9D93 };

	GET(TechnoClass* const, pThis, ESI);
	GET(int, index, EBX);

	auto pTechno = TechnoClass::Array.Items[index];

	if (pTechno->IsDead())
	{
		if (VTable::Get(pTechno) == 0x7E1F50) // AbstractClass::AbsVTable
		{
			Debug::LogAndMessage("TechnoClass::GreatestThreat: Found DeadTechno(0x%08X) with dirty vtable in TechnoArray!\n",
				reinterpret_cast<DWORD>(pTechno));

			TechnoClass::Array.RemoveItem(index);
			R->EBX(index - 1);

			if (SessionClass::IsSingleplayer() && Phobos::Config::DevelopmentCommands)
			{
				Debug::LogAndMessage("Skip processing. Entering Stepping Mode...\n");
				FrameByFrameCommandClass::FrameStep = true;
				auto coords = pThis->GetCoords();
				TacticalClass::Instance->SetTacticalPosition(&coords);
			}
		}
		else
		{
			Debug::Log("TechnoClass::GreatestThreat: Found DeadTechno(0x%08X)[%s]at(%d,%d) in TechnoArray.\n",
				reinterpret_cast<DWORD>(pTechno), pTechno->get_ID(), (pThis->Location.X / 256), (pThis->Location.Y / 256));
		}

		return NextOne;
	}

	R->ECX(pThis->Owner);
	R->EDI(pTechno);
	return Continue;
}

DEFINE_HOOK(0x6F91EC, TechnoClass_GreatestThreat_LogDeadInAircraftTracker, 0x6)
{
	enum { NextOne = 0x6F9377 };

	GET(TechnoClass* const, pThis, ESI);
	GET(TechnoClass* const, pTechno, EBP);

	if (pTechno->IsDead())
	{
		if (VTable::Get(pTechno) == 0x7E1F50) // AbstractClass::AbsVTable
		{
			Debug::LogAndMessage("TechnoClass::GreatestThreat: Found DeadTechno(0x%08X) with dirty vtable in AircraftTracker!\n",
				reinterpret_cast<DWORD>(pTechno));

			for (int i = 0; i < 20; ++i)
			{
				for (int j = 0; j < 20; ++j)
				{
					if (AircraftTrackerClass::Instance.TrackerVectors[i][j].Remove(pTechno))
						return NextOne;
				}
			}

			if (SessionClass::IsSingleplayer() && Phobos::Config::DevelopmentCommands)
			{
				Debug::LogAndMessage("Skip processing. Entering Stepping Mode...\n");
				FrameByFrameCommandClass::FrameStep = true;
				auto coords = pThis->GetCoords();
				TacticalClass::Instance->SetTacticalPosition(&coords);
			}
		}
		else
		{
			Debug::Log("TechnoClass::GreatestThreat: Found DeadTechno(0x%08X)[%s]at(%d,%d) in AircraftTracker.\n",
				reinterpret_cast<DWORD>(pTechno), pTechno->get_ID(), (pThis->Location.X / 256), (pThis->Location.Y / 256));
		}

		return NextOne;
	}

	return 0;
}

#pragma endregion

#pragma region DebugLogInTetherCheck

DEFINE_HOOK(0x7043B9, TechnoClass_GetZAdjustment_LogTetherButNoLink, 0x6)
{
	enum { SkipGameCode = 0x7043E1 };

	GET(TechnoClass* const, pLink, EAX);

	if (pLink)
		return 0;

	GET(TechnoClass* const, pThis, ESI);

	Debug::LogAndMessage("TechnoClass::GetZAdjustment: Found a Techno [%s] in Tether state without any RadioLink!\n", pThis->get_ID());

	if (SessionClass::IsSingleplayer() && Phobos::Config::DevelopmentCommands)
	{
		Debug::LogAndMessage("Skip processing. Entering Stepping Mode...\n");
		FrameByFrameCommandClass::FrameStep = true;
		auto coords = pThis->GetCoords();
		TacticalClass::Instance->SetTacticalPosition(&coords);
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x73B0C5, UnitClass_DrawIfVisible_LogTetherButNoLink, 0x6)
{
	enum { SkipGameCode = 0x73B124 };

	GET(TechnoClass* const, pLink, EAX);

	if (pLink)
		return 0;

	GET(TechnoClass* const, pThis, EDI);

	Debug::LogAndMessage("UnitClass::DrawIfVisible: Found a Unit [%s] in Tether state without any RadioLink!\n", pThis->get_ID());

	if (SessionClass::IsSingleplayer() && Phobos::Config::DevelopmentCommands)
	{
		Debug::LogAndMessage("Skip processing. Entering Stepping Mode...\n");
		FrameByFrameCommandClass::FrameStep = true;
		auto coords = pThis->GetCoords();
		TacticalClass::Instance->SetTacticalPosition(&coords);
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x7410D6, UnitClass_GetFireError_LogTetherButNoLink, 0x7)
{
	enum { SkipGameCode = 0x7410EC };

	GET(TechnoClass* const, pLink, EAX);

	if (pLink)
		return 0;

	GET(TechnoClass* const, pThis, ESI);

	Debug::LogAndMessage("UnitClass::GetFireError: Found a Unit [%s] in Tether state without any RadioLink!\n", pThis->get_ID());

	if (SessionClass::IsSingleplayer() && Phobos::Config::DevelopmentCommands)
	{
		Debug::LogAndMessage("Skip processing. Entering Stepping Mode...\n");
		FrameByFrameCommandClass::FrameStep = true;
		auto coords = pThis->GetCoords();
		TacticalClass::Instance->SetTacticalPosition(&coords);
	}

	return SkipGameCode;
}

#pragma endregion

// TODO Debug hooks
