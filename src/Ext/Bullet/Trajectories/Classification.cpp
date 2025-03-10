#include "Classification.h"

#include <OverlayTypeClass.h>

#include <Ext/Bullet/Body.h>
#include <Ext/Techno/Body.h>

void PhobosTrajectory::OnUnlimbo(BulletClass* pBullet)
{
	// Anyway, first reset the useless velocity
	pBullet->Velocity = BulletVelocity::Empty;

	// Without a target, the game will inevitably crash before, so no need to check here
	const auto pTarget = pBullet->Target;

	// Due to various ways of firing weapons, the true firer may have already died
	const auto pFirer = pBullet->Owner;

	// Just like GetTechnoType()
	const auto pType = this->GetType();

	// Record the status of the target
	this->TargetIsTechno = (pTarget->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None;
	this->TargetInTheAir = (pTarget->AbstractFlags & AbstractFlags::Object) ? (static_cast<ObjectClass*>(pTarget)->GetHeight() > Unsorted::CellHeight) : false;

	// Record some information of weapon
	if (const auto pWeapon = pBullet->WeaponType)
	{
		this->AttenuationRange = pWeapon->Range;
		this->CountOfBurst = pWeapon->Burst;

		if (pType->ApplyRangeModifiers && pFirer)
			this->AttenuationRange = WeaponTypeExt::GetRangeWithModifiers(pWeapon, pFirer);
	}

	// Record some information of firer
	if (pFirer)
	{
		this->CurrentBurst = pFirer->CurrentBurstIndex;
		this->FirepowerMult = pFirer->FirepowerMultiplier * TechnoExt::ExtMap.Find(pFirer)->AE.FirepowerMultiplier;

		const auto flag = this->Flag();

		if (pType->DisperseWeapons.size() && pType->RecordSourceCoord || flag == TrajectoryFlag::Engrave || flag == TrajectoryFlag::Tracing)
			this->GetTechnoFLHCoord(pBullet, pFirer);
		else
			this->NotMainWeapon = true;
	}
	else
	{
		this->NotMainWeapon = true;
	}

	// Initialize additional warheads
	if (pType->PassDetonate)
		this->PassDetonateTimer.Start(pType->PassDetonateInitialDelay > 0 ? pType->PassDetonateInitialDelay : 0);

	// Initialize additional weapons
	if (!pType->DisperseWeapons.empty() && !pType->DisperseCounts.empty() && this->DisperseCycle)
	{
		this->DisperseCount = pType->DisperseCounts[0];
		this->DisperseTimer.Start(pType->DisperseInitialDelay);
	}
}

bool PhobosTrajectory::OnAI(BulletClass* pBullet)
{
	const auto pFirer = pBullet->Owner;
	const auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;
	const auto pType = this->GetType();

	// Detonate the warhead at the current location
	if (pType->PassDetonate)
		this->PassWithDetonateAt(pBullet, pOwner);

	// Detonate the warhead on the technos passing through
	if (this->ProximityImpact != 0 && pType->ProximityRadius.Get() > 0)
		this->PrepareForDetonateAt(pBullet, pOwner);

	// Launch additional weapons towards the target
	if (this->DisperseTimer.Completed() && this->CheckFireFacing(pBullet) && this->PrepareDisperseWeapon(pBullet))
		return true;

	return false;
}

// Check if it should be detonated immediately
bool PhobosTrajectory::OnAIPreCheck(BulletClass* pBullet, HouseClass* pOwner)
{
	// If this value is not empty, it means that the projectile should be directly detonated at this time. This cannot be taken out here for use.
	if (this->ExtraCheck)
		return true;

	// Check the remaining existence time
	if (this->DurationTimer.Completed())
		return true;

	// Check the remaining travel distance of the bullet
	this->RemainingDistance -= static_cast<int>(this->GetType()->Speed);

	if (this->RemainingDistance < 0)
		return true;

	// Below ground level? (16 ->error range)
	return BulletTypeExt::ExtMap.Find(pBullet->Type)->SubjectToGround && MapClass::Instance->GetCellFloorHeight(pBullet->Location) >= (pBullet->Location.Z + 16);
}

// If there is an obstacle on the route, the bullet should need to reduce its speed so it will not penetrate the obstacle.
void PhobosTrajectory::OnAIVelocityCheck(BulletClass* pBullet, HouseClass* pOwner)
{
	double locationDistance = 0.0;
	bool velocityCheck = false;

	const auto pType = this->GetType();
	const bool checkThrough = (!pType->ThroughBuilding || !pType->ThroughVehicles);

	// Low speed with checkSubject was already done well
	if (pType->Speed < 256.0)
	{
		// Blocked by obstacles?
		if (checkThrough && this->CheckThroughAndSubjectInCell(pBullet, MapClass::Instance->GetCellAt(pBullet->Location), pOwner))
			velocityCheck = true;
	}
	else
	{
		// When in high speed, it's necessary to check each cell on the path that the next frame will pass through
		const bool subjectToGround = this->GetCanHitGround();
		const bool subjectToWalls = pBullet->Type->SubjectToWalls;
		const bool checkSubject = (subjectToGround || subjectToWalls);

		if (checkThrough || checkSubject)
		{
			const auto& theSourceCoords = pBullet->Location;
			const CoordStruct theTargetCoords
			{
				pBullet->Location.X + static_cast<int>(pBullet->Velocity.X),
				pBullet->Location.Y + static_cast<int>(pBullet->Velocity.Y),
				pBullet->Location.Z + static_cast<int>(pBullet->Velocity.Z)
			};

			const auto sourceCell = CellClass::Coord2Cell(theSourceCoords);
			const auto targetCell = CellClass::Coord2Cell(theTargetCoords);
			const auto cellDist = sourceCell - targetCell;
			const auto cellPace = CellStruct { static_cast<short>(std::abs(cellDist.X)), static_cast<short>(std::abs(cellDist.Y)) };

			auto largePace = static_cast<size_t>(std::max(cellPace.X, cellPace.Y));
			const auto stepCoord = !largePace ? CoordStruct::Empty : (theTargetCoords - theSourceCoords) * (1.0 / largePace);
			auto curCoord = theSourceCoords;
			auto pCurCell = MapClass::Instance->GetCellAt(sourceCell);

			for (size_t i = 0; i < largePace; ++i)
			{
				if ((subjectToGround && (curCoord.Z + 16) < MapClass::Instance->GetCellFloorHeight(curCoord)) // Below ground level? (16 ->error range)
					|| (subjectToWalls && pCurCell->OverlayTypeIndex != -1 && OverlayTypeClass::Array->GetItem(pCurCell->OverlayTypeIndex)->Wall) // Impact on the wall?
					|| (checkThrough && this->CheckThroughAndSubjectInCell(pBullet, pCurCell, pOwner))) // Blocked by obstacles?
				{
					locationDistance = PhobosTrajectory::Get2DDistance(curCoord, theSourceCoords);
					velocityCheck = true;
					break;
				}

				curCoord += stepCoord;
				pCurCell = MapClass::Instance->GetCellAt(curCoord);
			}

			const auto fireStormCoords = MapClass::Instance->FindFirstFirestorm(theSourceCoords, theTargetCoords, pOwner);

			if (fireStormCoords != CoordStruct::Empty)
			{
				const auto distance = PhobosTrajectory::Get2DDistance(fireStormCoords, theSourceCoords);

				if (!velocityCheck || distance < locationDistance)
					locationDistance = distance;

				velocityCheck = true;
			}
		}
	}

	// Check if the bullet needs to slow down the speed
	if (velocityCheck)
	{
		// Let the distance slightly exceed
		locationDistance += 32.0;
		this->RemainingDistance = 0;
		const auto velocity = PhobosTrajectory::Get2DVelocity(pBullet->Velocity);

		// It may not be necessary to compare them again, but still do so
		if (locationDistance < velocity)
			pBullet->Velocity *= (locationDistance / velocity);
	}
	else if (this->RemainingDistance < pType->Speed)
	{
		// Check if the distance to the destination exceeds the speed limit
		pBullet->Velocity *= this->RemainingDistance / pType->Speed;
	}
}

// If the check result here is true, it only needs to be detonated in the next frame, without returning.
void PhobosTrajectory::OnAILastCheck(BulletClass* pBullet, HouseClass* pOwner)
{
	const auto pType = this->GetType();

	// Obstacles were detected in the current frame here
	const auto pDetonateAt = this->ExtraCheck;

	if (!pDetonateAt)
		return;

	// Slow down and reset the target
	const auto distance = pDetonateAt->GetCoords().DistanceFrom(pBullet->Location);
	const auto velocity = pBullet->Velocity.Magnitude();

	// Set the new target so that the snap function can take effect
	this->SetBulletNewTarget(pBullet, pDetonateAt);

	if (std::abs(velocity) > 1e-10 && distance < velocity)
		pBullet->Velocity *= distance / velocity;

	// Need to cause additional damage?
	if (!this->ProximityImpact || !pType->ProximityWarhead)
		return;

	this->ProximityDetonateAt(pBullet, pOwner, pDetonateAt);
}

void PhobosTrajectory::OnAIPreDetonate(BulletClass* pBullet)
{
	const auto flag = this->Flag();

	// No damage, no anims...
	if (this->GetType()->PeacefulVanish.Get(flag == TrajectoryFlag::Engrave || flag == TrajectoryFlag::Tracing))
	{
		pBullet->Health = 0;
		pBullet->Limbo();
		pBullet->UnInit();
	}
}

// ------------------------------------------------------------------------------ //

template<typename T>
void LiveShellTrajectoryType::Serialize(T& Stm)
{
	Stm
		.Process(this->RotateCoord)
		.Process(this->OffsetCoord)
		.Process(this->AxisOfRotation)
		.Process(this->LeadTimeCalculate)
		.Process(this->SubjectToGround)
		.Process(this->EarlyDetonation)
		.Process(this->DetonationHeight)
		.Process(this->DetonationDistance)
		.Process(this->TargetSnapDistance)
		;
}

bool LiveShellTrajectoryType::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->PhobosTrajectoryType::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool LiveShellTrajectoryType::Save(PhobosStreamWriter& Stm) const
{
	this->PhobosTrajectoryType::Save(Stm);
	const_cast<LiveShellTrajectoryType*>(this)->Serialize(Stm);
	return true;
}
/*
void LiveShellTrajectoryType::Read(CCINIClass* const pINI, const char* pSection) // Read separately
{
	this->PhobosTrajectoryType::Read(pINI, pSection);
	INI_EX exINI(pINI);

	this->RotateCoord.Read(exINI, pSection, "Trajectory.RotateCoord");
	this->OffsetCoord.Read(exINI, pSection, "Trajectory.OffsetCoord");
	this->AxisOfRotation.Read(exINI, pSection, "Trajectory.AxisOfRotation");
	this->LeadTimeCalculate.Read(exINI, pSection, "Trajectory.LeadTimeCalculate");
	this->EarlyDetonation.Read(exINI, pSection, "Trajectory.EarlyDetonation");
	this->DetonationHeight.Read(exINI, pSection, "Trajectory.DetonationHeight");
	this->DetonationDistance.Read(exINI, pSection, "Trajectory.DetonationDistance");
	this->TargetSnapDistance.Read(exINI, pSection, "Trajectory.TargetSnapDistance");
}
*/
template<typename T>
void LiveShellTrajectory::Serialize(T& Stm)
{
	Stm
		.Process(this->LastTargetCoord)
		.Process(this->WaitOneFrame)
		;
}

bool LiveShellTrajectory::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->PhobosTrajectory::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool LiveShellTrajectory::Save(PhobosStreamWriter& Stm) const
{
	this->PhobosTrajectory::Save(Stm);
	const_cast<LiveShellTrajectory*>(this)->Serialize(Stm);
	return true;
}

void LiveShellTrajectory::OnUnlimbo(BulletClass* pBullet)
{
	this->PhobosTrajectory::OnUnlimbo(pBullet);

	this->LastTargetCoord = pBullet->TargetCoords;
}

template<typename T>
void VirtualTrajectoryType::Serialize(T& Stm)
{
	Stm
		.Process(this->VirtualSourceCoord)
		.Process(this->VirtualTargetCoord)
		.Process(this->AllowFirerTurning)
		.Process(this->IgnoresFirestorm)
		;
}

bool VirtualTrajectoryType::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->PhobosTrajectoryType::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool VirtualTrajectoryType::Save(PhobosStreamWriter& Stm) const
{
	this->PhobosTrajectoryType::Save(Stm);
	const_cast<VirtualTrajectoryType*>(this)->Serialize(Stm);
	return true;
}
/*
void VirtualTrajectoryType::Read(CCINIClass* const pINI, const char* pSection) // Read separately
{
	this->PhobosTrajectoryType::Read(pINI, pSection);
	INI_EX exINI(pINI);

	this->VirtualSourceCoord.Read(exINI, pSection, "Trajectory.VirtualSourceCoord");
	this->VirtualTargetCoord.Read(exINI, pSection, "Trajectory.VirtualTargetCoord");
	this->AllowFirerTurning.Read(exINI, pSection, "Trajectory.AllowFirerTurning");
}
*/
template<typename T>
void VirtualTrajectory::Serialize(T& Stm)
{
	Stm
		.Process(this->SurfaceFirerID)
		;
}

bool VirtualTrajectory::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->PhobosTrajectory::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool VirtualTrajectory::Save(PhobosStreamWriter& Stm) const
{
	this->PhobosTrajectory::Save(Stm);
	const_cast<VirtualTrajectory*>(this)->Serialize(Stm);
	return true;
}

void VirtualTrajectory::OnUnlimbo(BulletClass* pBullet)
{
	this->PhobosTrajectory::OnUnlimbo(pBullet);

	for (auto pTrans = pBullet->Owner; pTrans; pTrans = pTrans->Transporter)
		this->SurfaceFirerID = pTrans->UniqueID;
}
