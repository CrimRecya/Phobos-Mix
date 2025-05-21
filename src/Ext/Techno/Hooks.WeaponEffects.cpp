#include <BuildingClass.h>
#include <CellClass.h>
#include <MapClass.h>
#include <ParticleSystemClass.h>
#include <FootClass.h>
#include <WaveClass.h>

#include <Ext/ParticleSystemType/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Utilities/Macro.h>

// Contains hooks that fix weapon graphical effects like lasers, railguns, electric bolts, beams and waves not interacting
// correctly with obstacles between firer and target, as well as railgun / railgun particles being cut off by elevation.

namespace FireAtTemp
{
	BulletClass* FireBullet = nullptr;
	CoordStruct originalTargetCoords;
	CellClass* pObstacleCell = nullptr;
	AbstractClass* pOriginalTarget = nullptr;
	AbstractClass* pWaveOwnerTarget = nullptr;
}

DEFINE_HOOK(0x6FF08B, TechnoClass_Fire_RecordBullet, 0x6)
{
	GET(BulletClass*, pBullet, EBX);
	FireAtTemp::FireBullet = pBullet;
	return 0;
}

// Set obstacle cell.
DEFINE_HOOK(0x6FF15F, TechnoClass_FireAt_ObstacleCellSet, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GET(WeaponTypeClass*, pWeapon, EBX);
	GET_BASE(AbstractClass*, pTarget, 0x8);
	LEA_STACK(CoordStruct*, pSourceCoords, STACK_OFFSET(0xB0, -0x6C));

	auto coords = pTarget->GetCenterCoords();

	if (const auto pBuilding = abstract_cast<BuildingClass*, true>(pTarget))
		coords = pBuilding->GetTargetCoords();

	// This is set to a temp variable as well, as accessing it everywhere needed from TechnoExt would be more complicated.
	FireAtTemp::pObstacleCell = TrajectoryHelper::FindFirstObstacle(*pSourceCoords, coords, pWeapon->Projectile, pThis->Owner);
	TechnoExt::ExtMap.Find(pThis)->FiringObstacleCell = FireAtTemp::pObstacleCell;

	return 0;
}

// Apply obstacle logic to fire & spark particle system targets.
DEFINE_HOOK_AGAIN(0x6FF1D7, TechnoClass_FireAt_SparkFireTargetSet, 0x5)
DEFINE_HOOK(0x6FF189, TechnoClass_FireAt_SparkFireTargetSet, 0x5)
{
	if (FireAtTemp::pObstacleCell)
	{
		if (R->Origin() == 0x6FF189)
			R->ECX(FireAtTemp::pObstacleCell);
		else
			R->EDX(FireAtTemp::pObstacleCell);
	}

	return 0;
}

// Fix fire particle target coordinates potentially differing from actual target coords.
DEFINE_HOOK(0x62FA41, ParticleSystemClass_FireAI_TargetCoords, 0x6)
{
	enum { SkipGameCode = 0x62FBAF };

	GET(ParticleSystemClass*, pThis, ESI);

	auto const pTypeExt = ParticleSystemTypeExt::ExtMap.Find(pThis->Type);

	if (!pTypeExt->AdjustTargetCoordsOnRotation)
		return SkipGameCode;

	return 0;
}

// Fix fire particles being disallowed from going upwards.
DEFINE_HOOK(0x62D685, ParticleSystemClass_Fire_Coords, 0x5)
{
	enum { SkipGameCode = 0x62D6B7 };

	// Game checks if MapClass::GetCellFloorHeight() for currentCoords is larger than for previousCoords and sets the flags on ParticleClass to
	// remove it if so. Below is an attempt to create a smarter check that allows upwards movement and does not needlessly collide with elevation
	// but removes particles when colliding with flat ground. It doesn't work perfectly and covering all edge-cases is difficult or impossible so
	// preference was to disable it. Keeping the code here commented out, however.

	/*
	GET(ParticleClass*, pThis, ESI);
	REF_STACK(CoordStruct, currentCoords, STACK_OFFSET(0x24, -0x18));
	REF_STACK(CoordStruct, previousCoords, STACK_OFFSET(0x24, -0xC));

	auto const sourceLocation = pThis->ParticleSystem ? pThis->ParticleSystem->Location : CoordStruct { INT_MAX, INT_MAX, INT_MAX };
	auto const pCell = MapClass::Instance.TryGetCellAt(currentCoords);
	int cellFloor = MapClass::Instance.GetCellFloorHeight(currentCoords);
	bool downwardTrajectory = currentCoords.Z < previousCoords.Z;
	bool isBelowSource = cellFloor < sourceLocation.Z - Unsorted::LevelHeight * 2;
	bool isRamp = pCell ? pCell->SlopeIndex : false;

	if (!isRamp && isBelowSource && downwardTrajectory && currentCoords.Z < cellFloor)
	{
		pThis->unknown_12D = 1;
		pThis->unknown_131 = 1;
	}
	*/

	return SkipGameCode;
}

// Fix railgun target coordinates potentially differing from actual target coords.
DEFINE_HOOK(0x70C6B5, TechnoClass_Railgun_TargetCoords, 0x5)
{
	GET(AbstractClass*, pTarget, EBX);

	auto coords = pTarget->GetCenterCoords();

	if (const auto pBuilding = abstract_cast<BuildingClass*, true>(pTarget))
		coords = pBuilding->GetTargetCoords();
	else if (const auto pCell = abstract_cast<CellClass*, true>(pTarget))
		coords = pCell->GetCoordsWithBridge();

	R->EAX(&coords);
	return 0;
}

// Cut railgun logic off at obstacle coordinates.
DEFINE_HOOK(0x70CA64, TechnoClass_Railgun_Obstacles, 0x5)
{
	enum { Continue = 0x70CA79, Stop = 0x70CAD8 };

	REF_STACK(CoordStruct const, coords, STACK_OFFSET(0xC0, -0x80));

	auto pCell = MapClass::Instance.GetCellAt(coords);

	if (pCell == FireAtTemp::pObstacleCell)
		return Stop;

	return Continue;
}

DEFINE_HOOK(0x70C862, TechnoClass_Railgun_AmbientDamageIgnoreTarget1, 0x5)
{
	enum { IgnoreTarget = 0x70CA59 };

	GET_BASE(WeaponTypeClass*, pWeapon, 0x14);

	if (WeaponTypeExt::ExtMap.Find(pWeapon)->AmbientDamage_IgnoreTarget)
		return IgnoreTarget;

	return 0;
}

DEFINE_HOOK(0x70CA8B, TechnoClass_Railgun_AmbientDamageIgnoreTarget2, 0x6)
{
	enum { IgnoreTarget = 0x70CBB0 };

	GET_BASE(WeaponTypeClass*, pWeapon, 0x14);

	if (WeaponTypeExt::ExtMap.Find(pWeapon)->AmbientDamage_IgnoreTarget)
	{
		REF_STACK(DynamicVectorClass<ObjectClass*>, objects, STACK_OFFSET(0xC0, -0xAC));
		R->EAX(objects.Count);
		return IgnoreTarget;
	}

	return 0;
}

DEFINE_HOOK(0x70CBDA, TechnoClass_Railgun_AmbientDamageWarhead, 0x6)
{
	enum { SkipGameCode = 0x70CBE0 };

	GET(WeaponTypeClass*, pWeapon, EDI);

	R->EDX(WeaponTypeExt::ExtMap.Find(pWeapon)->AmbientDamage_Warhead.Get(pWeapon->Warhead));

	return SkipGameCode;
}

// Do not adjust map coordinates for railgun or fire stream particles that are below cell coordinates.
DEFINE_HOOK(0x62B8BC, ParticleClass_CTOR_CoordAdjust, 0x6)
{
	enum { SkipCoordAdjust = 0x62B8CB };

	GET(ParticleClass*, pThis, ESI);

	if (pThis->ParticleSystem
		&& (pThis->ParticleSystem->Type->BehavesLike == BehavesLike::Railgun
			|| pThis->ParticleSystem->Type->BehavesLike == BehavesLike::Fire))
	{
		return SkipCoordAdjust;
	}

	return 0;
}

DEFINE_HOOK_AGAIN(0x6FD70D, TechnoClass_DrawSth_DrawToInvisoFlakScatterLocation, 0x6) // CreateRBeam
DEFINE_HOOK_AGAIN(0x6FD514, TechnoClass_DrawSth_DrawToInvisoFlakScatterLocation, 0x7) // CreateEBolt
DEFINE_HOOK(0x6FD38D, TechnoClass_DrawSth_DrawToInvisoFlakScatterLocation, 0x7) // CreateLaser
{
	GET(CoordStruct*, pTargetCoords, EAX);

	if (const auto pBullet = FireAtTemp::FireBullet)
	{
		// The weapon may not have been set up
		const auto pWeaponExt = WeaponTypeExt::ExtMap.Find(pBullet->WeaponType);

		if (pWeaponExt && pWeaponExt->VisualScatter)
		{
			const auto& pRulesExt = RulesExt::Global();
			const auto radius = ScenarioClass::Instance->Random.RandomRanged(pRulesExt->VisualScatter_Min.Get(), pRulesExt->VisualScatter_Max.Get());
			*pTargetCoords = MapClass::GetRandomCoordsNear((pBullet->Type->Inviso ? pBullet->Location : pBullet->TargetCoords), radius, false);
		}
		else
		{
			*pTargetCoords = (pBullet->Type->Inviso ? pBullet->Location : pBullet->TargetCoords);
		}
	}
	else if (const auto pObstacleCell = FireAtTemp::pObstacleCell)
	{
		*pTargetCoords = pObstacleCell->GetCoordsWithBridge();
	}

	R->EAX(pTargetCoords);
	return 0;
}

// Adjust target for bolt / beam / wave drawing.
DEFINE_HOOK(0x6FF43F, TechnoClass_FireAt_TargetSet, 0x6)
{
	LEA_STACK(CoordStruct*, pTargetCoords, STACK_OFFSET(0xB0, -0x28));
	GET_BASE(AbstractClass*, pOriginalTarget, 0x8);

	// Store original target & coords
	FireAtTemp::originalTargetCoords = *pTargetCoords;
	FireAtTemp::pOriginalTarget = pOriginalTarget;

	if (FireAtTemp::pObstacleCell)
	{
		*pTargetCoords = FireAtTemp::pObstacleCell->GetCoordsWithBridge();
		R->Base(8, FireAtTemp::pObstacleCell); // Replace original target so it gets used by Ares sonic wave stuff etc. as well.
	}

	return 0;
}

// Restore original target values and unset obstacle cell.
DEFINE_HOOK(0x6FF660, TechnoClass_FireAt_ObstacleCellUnset, 0x6)
{
	LEA_STACK(CoordStruct*, pTargetCoords, STACK_OFFSET(0xB0, -0x28));

	// Restore original target & coords
	*pTargetCoords = FireAtTemp::originalTargetCoords;
	R->Base(8, FireAtTemp::pOriginalTarget);
	R->EDI(FireAtTemp::pOriginalTarget);

	// Reset temp values
	FireAtTemp::FireBullet = nullptr;
	FireAtTemp::originalTargetCoords = CoordStruct::Empty;
	FireAtTemp::pObstacleCell = nullptr;
	FireAtTemp::pOriginalTarget = nullptr;

	return 0;
}

namespace LaserZapContext
{
	TechnoClass* pThis = nullptr;
}

DEFINE_HOOK(0x6FD210, TechnoClass_LaserZap_SetContext, 0x7)
{
	GET(TechnoClass*, pThis, ECX);
	LaserZapContext::pThis = pThis;
	return 0;
}

static void __fastcall AttachTrackingLaser(WeaponTypeClass* pWeapon, LaserDrawClass* pLaser, ObjectClass* pTarget, int nWpIdx)
{
	if (auto const pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon))
	{
		if (!pLaser->IsHouseColor && pWeaponExt->Laser_IsSingleColor)
			pLaser->IsHouseColor = true;

		if (pWeaponExt->Laser_IsTracking)
		{
			auto const pThis = LaserZapContext::pThis;
			auto const flh = pThis->GetFLH(nWpIdx, CoordStruct { 0,0,0 }) - TechnoExt::GetFLHAbsoluteCoords(pThis, CoordStruct { 0,0,0 }, pThis->HasTurret());
			auto const pExt = TechnoExt::ExtMap.Find(pThis);
			auto const pTargetExt = TechnoExt::ExtMap.Find(abstract_cast<TechnoClass*>(pTarget));

			// This laser must be fired from techno, not bullet.
			// if (*pCrd != CoordStruct { 0,0,0 })
			//	return;;

			// Target changed. Stop tracking current lasers.
			if (pExt->MyTrackingLasersTarget && pExt->MyTrackingLasersTarget != pTarget)
			{
				if (auto pOldTargetExt = TechnoExt::ExtMap.Find(abstract_cast<TechnoClass*>(pExt->MyTrackingLasersTarget)))
				{
					for (int i = 0; i != pExt->MyTrackingLasers.Count; ++i)
					{
						for (int j = 0; j != pOldTargetExt->TrackingLasersTargetingMe.Count; ++j)
						{
							if (pExt->MyTrackingLasers[i] == pOldTargetExt->TrackingLasersTargetingMe[j])
							{
								pOldTargetExt->TrackingLasersTargetingMe.RemoveItem(j);
								break;
							}
						}
					}
				}

				pExt->MyTrackingLasers.Clear();
				pExt->MyTrackingLasers_WeaponIdx.Clear();
				pExt->MyTrackingLasers_BurstIdx.Clear();
				pExt->MyTrackingLasers_CreatorWeapon.Clear();
			}

			for (int idx = 0; idx != pExt->MyTrackingLasers.Count; ++idx)
			{
				if (pExt->MyTrackingLasers_WeaponIdx[idx] == nWpIdx && pExt->MyTrackingLasers_BurstIdx[idx] == pThis->CurrentBurstIndex)
				{
					if (pExt->MyTrackingLasers_CreatorWeapon[idx] == pWeapon)
					{
						// Same flh and same weapon, delete the new laser.
						// R->EAX(nullptr);
						// GameDelete(pLaser);
						pLaser->Duration = 0;
						return;
					}
					else
					{
						// Same flh different weapon, delete the old laser.
						if (auto pOldTargetExt = TechnoExt::ExtMap.Find(abstract_cast<TechnoClass*>(pTargetExt->MyTrackingLasersTarget)))
						{
							for (int j = 0; j != pOldTargetExt->TrackingLasersTargetingMe.Count; ++j)
							{
								if (pExt->MyTrackingLasers[idx] == pOldTargetExt->TrackingLasersTargetingMe[j])
								{
									pOldTargetExt->TrackingLasersTargetingMe.RemoveItem(j);
									break;
								}
							}
						}

						pExt->MyTrackingLasers[idx]->Duration = 0;
						pExt->MyTrackingLasers[idx] = pLaser;
						pExt->MyTrackingLasers_CreatorWeapon[idx] = pWeapon;
						break;
					}
				}
			}

			// Track the firer.
			pExt->MyTrackingLasers.AddItem(pLaser);
			pExt->MyTrackingLasers_WeaponIdx.AddItem(nWpIdx);
			pExt->MyTrackingLasers_BurstIdx.AddItem(pThis->CurrentBurstIndex);
			pExt->MyTrackingLasers_CreatorWeapon.AddItem(pWeapon);
			pExt->MyTrackingLasersTarget = pTarget;

			// Hardcoded these properties for tracking lasers.
			pLaser->Fades = false;
			pLaser->Duration = 2;
			pLaser->Progress.Value = 0;

			// Only track the target if it is a techno.
			if (!pTargetExt)
				return;

			pTargetExt->TrackingLasersTargetingMe.AddItem(pLaser);
		}
	}
}

// Allow drawing single color lasers with thickness.
DEFINE_HOOK(0x6FD446, TechnoClass_LaserZap_IsSingleColor, 0x7)
{
	GET(WeaponTypeClass* const, pWeapon, ECX);
	GET(LaserDrawClass* const, pLaser, EAX);
	GET_STACK(ObjectClass* const, pTarget, STACK_OFFSET(0x48, 0x4));
	GET_STACK(const int, nWpIdx, STACK_OFFSET(0x48, 0x8));
//	GET_STACK(CoordStruct*, pCrd, STACK_OFFSET(0x48, 0x10));

	// Fixes drawing thick lasers for non-PrismSupport building-fired lasers.
	pLaser->IsSupported = pLaser->Thickness > 3;
	AttachTrackingLaser(pWeapon, pLaser, pTarget, nWpIdx);

	return 0;
}

// WaveClass requires the firer's target and wave's target to match so it needs bit of extra handling here for obstacle cell targets.
DEFINE_HOOK(0x762AFF, WaveClass_AI_TargetSet, 0x6)
{
	GET(WaveClass*, pThis, ESI);

	if (pThis->Target && pThis->Owner)
	{
		auto const pObstacleCell = TechnoExt::ExtMap.Find(pThis->Owner)->FiringObstacleCell;

		if (pObstacleCell == pThis->Target && pThis->Owner->Target)
		{
			FireAtTemp::pWaveOwnerTarget = pThis->Owner->Target;
			pThis->Owner->Target = pThis->Target;
		}
	}

	return 0;
}

DEFINE_HOOK(0x762D57, WaveClass_AI_TargetUnset, 0x6)
{
	GET(WaveClass*, pThis, ESI);

	if (FireAtTemp::pWaveOwnerTarget)
	{
		if (pThis->Owner->Target)
			pThis->Owner->Target = FireAtTemp::pWaveOwnerTarget;

		FireAtTemp::pWaveOwnerTarget = nullptr;
	}

	return 0;
}
