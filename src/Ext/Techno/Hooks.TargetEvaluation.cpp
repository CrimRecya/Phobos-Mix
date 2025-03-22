#include "Body.h"

#include <Ext/BuildingType/Body.h>
#include <EventClass.h>

// Cursor & target acquisition stuff not directly tied to other features can go here.

#pragma region TargetAcquisition

DEFINE_HOOK(0x7098B9, TechnoClass_TargetSomethingNearby_AutoFire, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);

	if (auto pExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType()))
	{
		if (pExt->AutoFire)
		{
			if (pExt->AutoFire_TargetSelf)
				pThis->SetTarget(pThis);
			else
				pThis->SetTarget(pThis->GetCell());

			return 0x7099B8;
		}
	}

	return 0;
}

FireError __fastcall TechnoClass_TargetSomethingNearby_CanFire_Wrapper(TechnoClass* pThis, void* _, AbstractClass* pTarget, int weaponIndex, bool ignoreRange)
{
	auto const pExt = TechnoExt::ExtMap.Find(pThis);
	bool disableWeapons = pExt->AE.DisableWeapons;
	pExt->AE.DisableWeapons = false;
	auto const fireError = pThis->GetFireError(pTarget, weaponIndex, ignoreRange);
	pExt->AE.DisableWeapons = disableWeapons;
	return fireError;
}

DEFINE_FUNCTION_JUMP(CALL6, 0x7098E6, TechnoClass_TargetSomethingNearby_CanFire_Wrapper);

DEFINE_HOOK(0x4C73B0, EventClass_RespondToEvent_RecordTarget, 0x6)
{
	GET(EventClass*, pThis, ESI);
	GET(TechnoClass*, pTechno, EDI);
	GET(Mission, mission, EAX);

	if (mission == Mission::Attack)
	{
		auto pExt = TechnoExt::ExtMap.Find(pTechno);
		pExt->PlayerAssignedLastTarget = pThis->MegaMission.Target;
	}

	return 0;
}

bool IsAThreatToMe(TechnoClass* pTechno, AbstractClass* pTarget)
{
	auto pTechnoTarget = abstract_cast<TechnoClass*>(pTarget);
	bool targetThreat = false;

	if (pTechnoTarget)
	{
		auto error = pTechnoTarget->GetFireError(pTechno, pTechnoTarget->SelectWeapon(pTechno), false);
		targetThreat = pTechnoTarget->WhatAmI() == AbstractType::Building ? (error != FireError::ILLEGAL) && (error != FireError::RANGE) : (error != FireError::ILLEGAL);
	}

	return targetThreat;
}

bool IsAssignedTarget(TechnoClass* pTechno, AbstractClass* pTarget)
{
	auto pExt = TechnoExt::ExtMap.Find(pTechno);
	return pTarget && pExt->PlayerAssignedLastTarget == pTarget;
}

DEFINE_HOOK(0x702B31, TechnoClass_ReceiveDamage_ReturnFireCheck, 0x7)
{
	enum { SkipReturnFire = 0x702B47 };

	GET(TechnoClass* const, pThis, ESI);
	GET_STACK(TechnoClass*, pAttacker, STACK_OFFSET(0xC4, 0x10));

	auto const pOwner = pThis->Owner;

	// Vanilla behavior.
	if (!pOwner->IsControlledByHuman() || !RulesExt::Global()->PlayerReturnFire_Smarter)
		return 0;

	// Too far. Can't attack.
	if (!pThis->IsCloseEnoughToAttack(pAttacker))
		return SkipReturnFire;

	auto pCurrentTarget = pThis->Target;

	// Target is assigned by player. I must kill it first.
	if (IsAssignedTarget(pThis, pCurrentTarget))
		return SkipReturnFire;

	// Current target may hurt me. I must kill it first.
	if (IsAThreatToMe(pThis, pCurrentTarget) && pThis->IsCloseEnoughToAttack(pCurrentTarget))
		return SkipReturnFire;

	// OK I will attack it, but no override mission.
	pThis->Target = pAttacker; // 如果使用 Settter，当 pAttacker 为建筑时单位会停下。我不知道这是为什么。
	// pThis->SetTarget(pAttacker);
	// pThis->QueueMission(Mission::Attack, false);
	return SkipReturnFire;
}

DEFINE_HOOK(0x708859, TechnoClass_CanRetaliate_SmarterReturnFire, 0x6)
{
	enum { CanRetaliate = 0x708867 };

	GET(TechnoClass*, pThis, ESI);

	auto pTarget = pThis->Target;

	return RulesClass::Instance->PlayerReturnFire
		&& RulesExt::Global()->PlayerReturnFire_Smarter
		&& !IsAssignedTarget(pThis, pTarget)
		&& !(IsAThreatToMe(pThis, pTarget) && pThis->IsCloseEnoughToAttack(pTarget))
		? CanRetaliate : 0;
}

DEFINE_HOOK(0x6FA697, TechnoClass_Update_AutoTargetMissionCheck, 0x6)
{
	enum { CanTargeting = 0x6FA6AC };

	GET(TechnoClass*, pThis, ESI);

	return pThis->CurrentMission == Mission::Attack && RulesExt::Global()->ExtraTargeting_OnNoTargetAssigned ? CanTargeting : 0;
}

DEFINE_HOOK(0x7093E9, TechnoClass_CanPassiveAquireNow_MissionCheck, 0x7)
{
	enum { CanTargeting = 0x7093F8 };

	GET(TechnoClass*, pThis, ESI);

	auto pTarget = pThis->Target;

	return pThis->CurrentMission == Mission::Attack
		&& RulesExt::Global()->ExtraTargeting_OnNoTargetAssigned
		&& !IsAssignedTarget(pThis, pTarget)
		&& !(IsAThreatToMe(pThis, pTarget) && pThis->IsCloseEnoughToAttack(pTarget))
		? CanTargeting : 0;
}

DEFINE_HOOK(0x70CF1D, TechnoClass_ThreatCoefficient_CanAttackMeThreatBonus, 0x6)
{
	REF_STACK(double, totalThreat, STACK_OFFSET(0x58, -0x48));

	totalThreat += RulesExt::Global()->CanAttackMeThreatBonus;
	return 0;
}

DEFINE_HOOK(0x709918, TechnoClass_TargetAndEstimateDamage_CheckTarget, 0x6)
{
	enum { CanTargeting = 0x709926 };

	GET(TechnoClass*, pThis, ESI);

	auto pTarget = pThis->Target;

	return pTarget
		&& RulesExt::Global()->ExtraTargeting_OnNoTargetAssigned
		&& !IsAssignedTarget(pThis, pTarget)
		&& !(IsAThreatToMe(pThis, pTarget) && pThis->IsCloseEnoughToAttack(pTarget))
		? CanTargeting : 0;
}

#pragma endregion

#pragma region MapZone

namespace MapZoneTemp
{
	TargetZoneScanType zoneScanType;
}

DEFINE_HOOK(0x6F9C67, TechnoClass_GreatestThreat_MapZoneSetContext, 0x5)
{
	GET(TechnoClass*, pThis, ESI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->GetTechnoType());
	MapZoneTemp::zoneScanType = pTypeExt->TargetZoneScanType;

	return 0;
}

DEFINE_HOOK(0x6F7E47, TechnoClass_EvaluateObject_MapZone, 0x7)
{
	enum { AllowedObject = 0x6F7EA2, DisallowedObject = 0x6F894F };

	GET(TechnoClass*, pThis, EDI);
	GET(ObjectClass*, pObject, ESI);
	GET(int, zone, EBP);

	if (auto const pTechno = abstract_cast<TechnoClass*>(pObject))
	{
		if (!TechnoExt::AllowedTargetByZone(pThis, pTechno, MapZoneTemp::zoneScanType, nullptr, true, zone))
			return DisallowedObject;
	}

	return AllowedObject;
}

#pragma endregion

#pragma region Walls

DEFINE_HOOK(0x70095A, TechnoClass_WhatAction_WallWeapon, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET_STACK(OverlayTypeClass*, pOverlayTypeClass, STACK_OFFSET(0x2C, -0x18));

	int weaponIndex = TechnoExt::GetWeaponIndexAgainstWall(pThis, pOverlayTypeClass);
	R->EAX(pThis->GetWeapon(weaponIndex));

	return 0;
}

DEFINE_HOOK(0x51C1F1, InfantryClass_CanEnterCell_WallWeapon, 0x5)
{
	enum { SkipGameCode = 0x51C1FE };

	GET(InfantryClass*, pThis, EBP);
	GET(OverlayTypeClass*, pOverlayTypeClass, ESI);

	R->EAX(pThis->GetWeapon(TechnoExt::GetWeaponIndexAgainstWall(pThis, pOverlayTypeClass)));

	return SkipGameCode;
}

DEFINE_HOOK(0x73F495, UnitClass_CanEnterCell_WallWeapon, 0x6)
{
	enum { SkipGameCode = 0x73F4A1 };

	GET(UnitClass*, pThis, EBX);
	GET(OverlayTypeClass*, pOverlayTypeClass, ESI);

	R->EAX(pThis->GetWeapon(TechnoExt::GetWeaponIndexAgainstWall(pThis, pOverlayTypeClass)));

	return SkipGameCode;
}

namespace CellEvalTemp
{
	int weaponIndex;
}

DEFINE_HOOK(0x6F8C9D, TechnoClass_EvaluateCell_SetContext, 0x7)
{
	GET(int, weaponIndex, EAX);

	CellEvalTemp::weaponIndex = weaponIndex;

	return 0;
}

WeaponStruct* __fastcall TechnoClass_EvaluateCellGetWeaponWrapper(TechnoClass* pThis)
{
	return pThis->GetWeapon(CellEvalTemp::weaponIndex);
}

int __fastcall TechnoClass_EvaluateCellGetWeaponRangeWrapper(TechnoClass* pThis, void* _, int weaponIndex)
{
	return pThis->GetWeaponRange(CellEvalTemp::weaponIndex);
}

DEFINE_FUNCTION_JUMP(CALL6, 0x6F8CE3, TechnoClass_EvaluateCellGetWeaponWrapper);
DEFINE_FUNCTION_JUMP(CALL6, 0x6F8DD2, TechnoClass_EvaluateCellGetWeaponRangeWrapper);

#pragma endregion

#pragma region AggressiveAttackMove

static inline bool CheckAttackMoveCanResetTarget(FootClass* pThis)
{
	const auto pTarget = pThis->Target;

	if (!pTarget || pTarget == pThis->MegaTarget)
		return false;

	const auto pTargetTechno = abstract_cast<TechnoClass*>(pTarget);

	if (!pTargetTechno || pTargetTechno->IsArmed())
		return false;

	if (pThis->TargetingTimer.InProgress())
		return false;

	const auto pPrimaryWeapon = pThis->GetWeapon(0)->WeaponType;

	if (!pPrimaryWeapon)
		return false;

	const auto pNewTarget = abstract_cast<TechnoClass*>(pThis->GreatestThreat(ThreatType::Range, &pThis->Location, false));

	if (!pNewTarget || pNewTarget->GetTechnoType() == pTargetTechno->GetTechnoType())
		return false;

	const auto pSecondaryWeapon = pThis->GetWeapon(1)->WeaponType;

	if (!pSecondaryWeapon || !pSecondaryWeapon->NeverUse) // Melee unit's virtual scanner
		return true;

	return pSecondaryWeapon->Range <= pPrimaryWeapon->Range;
}

DEFINE_HOOK(0x4DF3A0, FootClass_UpdateAttackMove_SelectNewTarget, 0x6)
{
	GET(FootClass* const, pThis, ECX);

	const auto pExt = TechnoExt::ExtMap.Find(pThis);

	if (pExt->TypeExtData->AttackMove_UpdateTarget.Get(RulesExt::Global()->AttackMove_UpdateTarget) && CheckAttackMoveCanResetTarget(pThis))
	{
		pThis->Target = nullptr;
		pThis->HaveAttackMoveTarget = false;
	}

	return 0;
}

DEFINE_HOOK(0x6F85AB, TechnoClass_CanAutoTargetObject_AggressiveAttackMove, 0x6)
{
	enum { ContinueCheck = 0x6F85BA, CanTarget = 0x6F8604 };

	GET(TechnoClass* const, pThis, EDI);

	if (!pThis->Owner->IsControlledByHuman())
		return CanTarget;

	GET(TechnoClass*, pTarget, ESI);

	if (pTarget->WhatAmI() == AbstractType::Building)
	{
		// Fallback to unmodded behavior if the building is an exempt of aggressive stance.
		if (BuildingTypeExt::ExtMap.Find(static_cast<BuildingClass*>(pTarget)->Type)->AggressiveStance_Exempt)
			return ContinueCheck;

		if (TechnoExt::ExtMap.Find(pThis)->GetAggressiveStance())
			return CanTarget;
	}

	if (pThis->MegaMissionIsAttackMove())
	{
		if (TechnoExt::ExtMap.Find(pThis)->TypeExtData->AttackMove_Aggressive.Get(RulesExt::Global()->AttackMove_Aggressive))
			return CanTarget;
	}

	return ContinueCheck;
}

#pragma endregion

#pragma region HealingWeapons

#pragma region TechnoClass_EvaluateObject

namespace EvaluateObjectTemp
{
	WeaponTypeClass* PickedWeapon = nullptr;
}

DEFINE_HOOK(0x6F7E24, TechnoClass_EvaluateObject_SetContext, 0x6)
{
	GET(WeaponTypeClass*, pWeapon, EBP);

	EvaluateObjectTemp::PickedWeapon = pWeapon;

	return 0;
}

double __fastcall HealthRatio_Wrapper(TechnoClass* pTechno)
{
	double result = pTechno->GetHealthPercentage();

	if (result >= 1.0)
	{
		if (const auto pExt = TechnoExt::ExtMap.Find(pTechno))
		{
			if (const auto pShieldData = pExt->Shield.get())
			{
				if (pShieldData->IsActive())
				{
					const auto pWH = EvaluateObjectTemp::PickedWeapon ? EvaluateObjectTemp::PickedWeapon->Warhead : nullptr;
					const auto pFoot = abstract_cast<FootClass*>(pTechno);

					if (!pShieldData->CanBePenetrated(pWH) || ((pFoot && pFoot->ParasiteEatingMe)))
						result = pExt->Shield->GetHealthRatio();
				}
			}
		}
	}

	return result;
}

DEFINE_FUNCTION_JUMP(CALL, 0x6F7F51, HealthRatio_Wrapper)

#pragma endregion

class AresScheme
{
	static inline ObjectClass* LinkedObj = nullptr;
public:
	static void __cdecl Prefix(TechnoClass* pThis, ObjectClass* pObj, int nWeaponIndex, bool considerEngineers)
	{
		if (LinkedObj)
			return;

		if (considerEngineers && CanApplyEngineerActions(pThis, pObj))
			return;

		if (nWeaponIndex < 0)
			nWeaponIndex = pThis->SelectWeapon(pObj);

		if (const auto pTechno = abstract_cast<TechnoClass*>(pObj))
		{
			if (const auto pExt = TechnoExt::ExtMap.Find(pTechno))
			{
				if (const auto pShieldData = pExt->Shield.get())
				{
					if (pShieldData->IsActive())
					{
						const auto pWeapon = pThis->GetWeapon(nWeaponIndex)->WeaponType;
						const auto pFoot = abstract_cast<FootClass*>(pObj);

						if (pWeapon && (!pShieldData->CanBePenetrated(pWeapon->Warhead) || (pFoot && pFoot->ParasiteEatingMe)))
						{
							const auto shieldRatio = pExt->Shield->GetHealthRatio();

							if (shieldRatio < 1.0)
							{
								LinkedObj = pObj;
								--LinkedObj->Health;
							}
						}
					}
				}
			}
		}
	}

	static void __cdecl Suffix()
	{
		if (LinkedObj)
		{
			++LinkedObj->Health;
			LinkedObj = nullptr;
		}
	}

private:
	static bool CanApplyEngineerActions(TechnoClass* pThis, ObjectClass* pTarget)
	{
		const auto pInf = abstract_cast<InfantryClass*>(pThis);
		const auto pBuilding = abstract_cast<BuildingClass*>(pTarget);

		if (!pInf || !pBuilding)
			return false;

		bool allied = HouseClass::CurrentPlayer->IsAlliedWith(pBuilding);

		if (allied && pBuilding->Type->Repairable)
			return true;

		if (!allied && pBuilding->Type->Capturable &&
			(!pBuilding->Owner->Type->MultiplayPassive || !pBuilding->Type->CanBeOccupied || pBuilding->IsBeingWarpedOut()))
		{
			return true;
		}

		return false;
	}
};

FireError __fastcall UnitClass__GetFireError_Wrapper(UnitClass* pThis, void* _, ObjectClass* pObj, int nWeaponIndex, bool ignoreRange)
{
	AresScheme::Prefix(pThis, pObj, nWeaponIndex, false);
	auto const result = pThis->UnitClass::GetFireError(pObj, nWeaponIndex, ignoreRange);
	AresScheme::Suffix();
	return result;
}
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F6030, UnitClass__GetFireError_Wrapper)

FireError __fastcall InfantryClass__GetFireError_Wrapper(InfantryClass* pThis, void* _, ObjectClass* pObj, int nWeaponIndex, bool ignoreRange)
{
	AresScheme::Prefix(pThis, pObj, nWeaponIndex, false);
	auto const result = pThis->InfantryClass::GetFireError(pObj, nWeaponIndex, ignoreRange);
	AresScheme::Suffix();
	return result;
}
DEFINE_FUNCTION_JUMP(VTABLE, 0x7EB418, InfantryClass__GetFireError_Wrapper)

Action __fastcall UnitClass__WhatAction_Wrapper(UnitClass* pThis, void* _, ObjectClass* pObj, bool ignoreForce)
{
	AresScheme::Prefix(pThis, pObj, -1, false);
	auto const result = pThis->UnitClass::MouseOverObject(pObj, ignoreForce);
	AresScheme::Suffix();
	return result;
}
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F5CE4, UnitClass__WhatAction_Wrapper)

Action __fastcall InfantryClass__WhatAction_Wrapper(InfantryClass* pThis, void* _, ObjectClass* pObj, bool ignoreForce)
{
	AresScheme::Prefix(pThis, pObj, -1, pThis->Type->Engineer);
	auto const result = pThis->InfantryClass::MouseOverObject(pObj, ignoreForce);
	AresScheme::Suffix();
	return result;
}
DEFINE_FUNCTION_JUMP(VTABLE, 0x7EB0CC, InfantryClass__WhatAction_Wrapper)

#pragma endregion
