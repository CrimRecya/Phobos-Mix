#include "PhobosTrajectory.h"

#include <OverlayTypeClass.h>

#include <Ext/Bullet/Body.h>
#include <Ext/Techno/Body.h>

void PhobosTrajectory::OnUnlimbo()
{
	const auto pBullet = this->Bullet;

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
	int damage = pBullet->Health;

	// Record some information of weapon
	if (const auto pWeapon = pBullet->WeaponType)
	{
		this->AttenuationRange = pWeapon->Range;
		this->CountOfBurst = pWeapon->Burst;

		if (pType->ApplyRangeModifiers && pFirer)
			this->AttenuationRange = WeaponTypeExt::GetRangeWithModifiers(pWeapon, pFirer);

		damage = pWeapon->Damage;
	}

	this->ProximityDamage = pType->ProximityDamage.Get(damage);
	this->PassDetonateDamage = pType->PassDetonateDamage.Get(damage);

	// Record some information of firer
	if (pFirer)
	{
		this->CurrentBurst = pFirer->CurrentBurstIndex;
		this->FirepowerMult = pFirer->FirepowerMultiplier * TechnoExt::ExtMap.Find(pFirer)->AE.FirepowerMultiplier;

		const auto flag = this->Flag();

		if (pType->DisperseWeapons.size() && pType->RecordSourceCoord || flag == TrajectoryFlag::Engrave || flag == TrajectoryFlag::Tracing)
			this->GetTechnoFLHCoord();
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

bool PhobosTrajectory::OnAI()
{
	const auto pType = this->GetType();

	// Detonate the warhead at the current location
	if (pType->PassDetonate)
		this->PassWithDetonateAt();

	// Detonate the warhead on the technos passing through
	if (this->ProximityImpact != 0 && pType->ProximityRadius.Get() > 0)
		this->PrepareForDetonateAt();

	// Launch additional weapons towards the target
	if (!this->DisperseTimer.Completed())
		return false;

	const auto pBullet = this->Bullet;
	const auto range = pType->DisperseEffectiveRange.Get();

	if (range && pBullet->TargetCoords.DistanceFrom(pBullet->Location) > range)
		return false;

	return this->CheckFireFacing() && this->PrepareDisperseWeapon();
}

// Check if it should be detonated immediately
bool PhobosTrajectory::OnAIDetonateCheck()
{
	// The previous frame requires detonation at this time
	if (this->ShouldDetonate)
		return true;

	// Check the remaining existence time
	if (this->DurationTimer.Completed())
		return true;

	const auto pBullet = this->Bullet;

	// Below ground level? (16 ->error range)
	if (this->GetCanHitGround() && MapClass::Instance->GetCellFloorHeight(pBullet->Location) >= (pBullet->Location.Z + 16))
		return true;

	// Check if the tolerance time has ended or if the firer's target can be synchronized
	if (this->CheckTolerantAndSynchronize())
		return true;

	// Check if the target needs to be changed
	return (std::abs(this->GetType()->RetargetRadius) > 1e-10 && this->BulletRetargetTechno());
}

// If there is an obstacle on the route, the bullet should need to reduce its speed so it will not penetrate the obstacle.
void PhobosTrajectory::OnAIVelocityCheck()
{
	const auto pBullet = this->Bullet;
	double locationDistance = 0.0;
	bool velocityCheck = false;

	const auto pType = this->GetType();
	const bool checkThrough = (!pType->ThroughBuilding || !pType->ThroughVehicles);
	const auto velocity = PhobosTrajectory::Get2DVelocity(this->MovingVelocity);

	// Low speed with checkSubject was already done well
	if (velocity < 256.0)
	{
		// Blocked by obstacles?
		if (checkThrough)
		{
			const auto pFirer = pBullet->Owner;
			const auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;

			if (this->CheckThroughAndSubjectInCell(MapClass::Instance->GetCellAt(pBullet->Location), pOwner))
				velocityCheck = true;
		}
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
				theSourceCoords.X + static_cast<int>(this->MovingVelocity.X),
				theSourceCoords.Y + static_cast<int>(this->MovingVelocity.Y),
				theSourceCoords.Z + static_cast<int>(this->MovingVelocity.Z)
			};

			const auto sourceCell = CellClass::Coord2Cell(theSourceCoords);
			const auto targetCell = CellClass::Coord2Cell(theTargetCoords);
			const auto cellDist = sourceCell - targetCell;
			const auto cellPace = CellStruct { static_cast<short>(std::abs(cellDist.X)), static_cast<short>(std::abs(cellDist.Y)) };

			auto largePace = static_cast<size_t>(std::max(cellPace.X, cellPace.Y));
			const auto stepCoord = !largePace ? CoordStruct::Empty : (theTargetCoords - theSourceCoords) * (1.0 / largePace);
			auto curCoord = theSourceCoords;
			auto pCurCell = MapClass::Instance->GetCellAt(sourceCell);

			const auto pFirer = pBullet->Owner;
			const auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;

			for (size_t i = 0; i < largePace; ++i)
			{
				if ((subjectToGround && (curCoord.Z + 16) < MapClass::Instance->GetCellFloorHeight(curCoord)) // Below ground level? (16 ->error range)
					|| (subjectToWalls && pCurCell->OverlayTypeIndex != -1 && OverlayTypeClass::Array->GetItem(pCurCell->OverlayTypeIndex)->Wall) // Impact on the wall?
					|| (checkThrough && this->CheckThroughAndSubjectInCell(pCurCell, pOwner))) // Blocked by obstacles?
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

	double ratio = 1.0;

	// Check if the bullet needs to slow down the speed
	if (velocityCheck)
	{
		// Let the distance slightly exceed
		locationDistance += 32.0;
		this->ShouldDetonate = true;

		// It may not be necessary to compare them again, but still do so
		if (locationDistance < velocity)
			ratio = (locationDistance / velocity);
	}

	// Check if the distance to the destination exceeds the speed limit
	if (this->RemainingDistance < this->MovingSpeed)
	{
		const auto newRatio = this->RemainingDistance / this->MovingSpeed;

		if (ratio > newRatio)
			ratio = newRatio;
	}

	if (ratio < 1.0)
		this->MultiplyBulletVelocity(ratio, true);
}

// If the check result here is true, it only needs to be detonated in the next frame, without returning.
void PhobosTrajectory::OnAINextFrameCheck()
{
	// Obstacles were detected in the current frame here
	const auto pDetonateAt = this->ExtraCheck;

	if (!pDetonateAt)
		return;

	// Slow down and reset the target
	const auto pBullet = this->Bullet;
	const auto distance = pDetonateAt->GetCoords().DistanceFrom(pBullet->Location);

	// Set the new target so that the snap function can take effect
	this->SetBulletNewTarget(pDetonateAt);
	const auto speed = this->MovingSpeed;

	if (speed && distance < speed)
		this->MultiplyBulletVelocity(distance / speed, true);
	else
		this->ShouldDetonate = true;

	// Need to cause additional damage?
	if (!this->ProximityImpact || !this->GetType()->ProximityWarhead)
		return;

	const auto pFirer = pBullet->Owner;
	const auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;
	this->ProximityDetonateAt(pOwner, pDetonateAt);
}

void PhobosTrajectory::OnAIPreDetonate()
{
	const auto pType = this->GetType();

	// Special circumstances, similar to airburst behavior
	if (pType->DisperseEffectiveRange.Get() < 0)
		this->PrepareDisperseWeapon();

	const auto pBullet = this->Bullet;
	const auto flag = this->Flag();

	// No damage, no anims...
	if (pType->PeacefulVanish.Get(flag == TrajectoryFlag::Engrave || flag == TrajectoryFlag::Tracing))
	{
		pBullet->Health = 0;
		pBullet->Limbo();
		pBullet->UnInit();
	}
	else
	{
		// Calculate the current damage
		pBullet->Health = this->GetTheTrueDamage(pBullet->Health, true);
	}
}

void PhobosTrajectory::OpenFire()
{
	const auto pBullet = this->Bullet;
	const auto pFirer = pBullet->Owner;
	const auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;
	auto fireStormCoords = MapClass::Instance->FindFirstFirestorm(pBullet->SourceCoords, pBullet->TargetCoords, pOwner);

	if (fireStormCoords != CoordStruct::Empty)
	{
		fireStormCoords.Z = MapClass::Instance->GetCellFloorHeight(fireStormCoords);
		pBullet->Data.Location = fireStormCoords;
	}
}

void PhobosTrajectory::SetBulletNewTarget(AbstractClass* const pTarget)
{
	const auto pBullet = this->Bullet;
	pBullet->SetTarget(pTarget);
	pBullet->TargetCoords = pTarget->GetCoords();
}

bool PhobosTrajectory::CalculateBulletVelocity(const double speed)
{
	const auto velocityLength = this->MovingVelocity.Magnitude();

	if (velocityLength < 1e-10)
		return true;

	this->MovingVelocity *= speed / velocityLength;
	this->MovingSpeed = speed;

	return false;
}

void PhobosTrajectory::MultiplyBulletVelocity(const double ratio, const bool shouldDetonate)
{
	this->MovingVelocity *= ratio;
	this->MovingSpeed = this->MovingSpeed * ratio;

	if (shouldDetonate)
		this->ShouldDetonate = true;
}

void PhobosTrajectory::ChangeBulletFacing()
{
	const auto pBullet = this->Bullet;
	const auto pType = this->GetType();
	constexpr double ratio = Math::TwoPi / 256;

	if (!pType->BulletSpin)
	{
		if (pType->BulletROT < 0)
			return;

		BulletVelocity desiredFacing
		{
			static_cast<double>(pBullet->TargetCoords.X - pBullet->Location.X),
			static_cast<double>(pBullet->TargetCoords.Y - pBullet->Location.Y),
			0
		};

		if (!pType->BulletROT)
		{
			this->MovingVelocity = desiredFacing * (1 / desiredFacing.Magnitude());
			return;
		}

		const auto current = Math::atan2(this->MovingVelocity.Y, this->MovingVelocity.X);
		const auto desired = Math::atan2(desiredFacing.Y, desiredFacing.X);
		const auto rotate = pType->BulletROT * ratio;

		const auto differenceP = desired - current;
		const bool dir1 = differenceP > 0;
		const auto delta = dir1 ? differenceP : -differenceP;

		const bool dir2 = delta > Math::Pi;
		const auto differenceR = dir2 ? (Math::TwoPi - delta) : delta;
		const bool dirR = dir1 ^ dir2;

		if (differenceR <= rotate)
		{
			this->MovingVelocity = desiredFacing * (1 / desiredFacing.Magnitude());
			return;
		}

		const auto facing = current + (dirR ? rotate : -rotate);
		this->MovingVelocity.X = Math::cos(facing);
		this->MovingVelocity.Y = Math::sin(facing);
		this->MovingVelocity.Z = 0;
	}
	else
	{
		const auto radian = Math::atan2(this->MovingVelocity.Y, this->MovingVelocity.X) + (pType->BulletROT * ratio);
		this->MovingVelocity.X = Math::cos(radian);
		this->MovingVelocity.Y = Math::sin(radian);
		this->MovingVelocity.Z = 0;
	}
}

bool PhobosTrajectory::CheckTolerantAndSynchronize()
{
	const auto pBullet = this->Bullet;
	const auto pType = this->GetType();
	auto pFirer = pBullet->Owner;

	if (!pBullet->Target && !pType->TolerantTime)
		return true;

	for (auto pTrans = pFirer->Transporter; pTrans; pTrans = pTrans->Transporter)
		pFirer = pTrans;

	if (pType->Synchronize)
	{
		if (pFirer)
		{
			auto pTarget = pFirer->Target;

			if (pBullet->Target != pTarget && !pType->TolerantTime)
				return true;

			if (pTarget && (pTarget->IsInAir() != this->TargetInTheAir))
				pTarget = nullptr;

			pBullet->SetTarget(pTarget);
		}
	}

	if (const auto pTarget = pBullet->Target)
	{
		pBullet->TargetCoords = pTarget->GetCoords();
		this->TolerantTimer.Stop();
	}
	else if (pType->TolerantTime > 0)
	{
		if (this->TolerantTimer.Completed())
			return true;
		else if (!this->TolerantTimer.IsTicking())
			this->TolerantTimer.Start(pType->TolerantTime);
	}

	return false;
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

void LiveShellTrajectory::OnUnlimbo()
{
	this->PhobosTrajectory::OnUnlimbo();

	const auto pType = this->GetType();
	this->LastTargetCoord = this->Bullet->TargetCoords;

	if (pType->Duration > 0)
		this->DurationTimer.Start(pType->Duration);
}

bool LiveShellTrajectory::OnAI()
{
	if (this->WaitOneFrame && this->BulletPrepareCheck())
		return false;

	if (this->OnAIDetonateCheck())
		return true;

	this->OnAIVelocityCheck();

	if (this->PhobosTrajectory::OnAI())
		return true;

	this->OnAINextFrameCheck();

	return false;
}

void LiveShellTrajectory::OnAIPreDetonate()
{
	// Can snap to target?
	const auto targetSnapDistance = static_cast<const LiveShellTrajectoryType*>(this->GetType())->TargetSnapDistance.Get();

	if (targetSnapDistance <= 0)
		return;

	const auto pBullet = this->Bullet;
	const auto pTarget = abstract_cast<ObjectClass*>(pBullet->Target);
	const auto coords = pTarget ? pTarget->GetCoords() : pBullet->TargetCoords;

	// Whether to snap to target?
	if (coords.DistanceFrom(pBullet->Location) <= targetSnapDistance)
	{
		const auto pExt = BulletExt::ExtMap.Find(pBullet);
		pExt->SnappedToTarget = true;
		pBullet->SetLocation(coords);
	}

	this->PhobosTrajectory::OnAIPreDetonate();
}

BulletVelocity LiveShellTrajectory::RotateAboutTheAxis(const BulletVelocity& theSpeed, BulletVelocity& theAxis, double theRadian)
{
	const auto theAxisLengthSquared = theAxis.MagnitudeSquared();

	// Zero vector is not acceptable
	if (std::abs(theAxisLengthSquared) < 1e-10)
		return theSpeed;

	// Rotate around the axis of rotation
	theAxis *= 1 / sqrt(theAxisLengthSquared);
	const auto cosRotate = Math::cos(theRadian);

	return ((theSpeed * cosRotate) + (theAxis * ((1 - cosRotate) * (theSpeed * theAxis))) + (theAxis.CrossProduct(theSpeed) * Math::sin(theRadian)));
}

bool LiveShellTrajectory::BulletPrepareCheck()
{
	// The time between bullets' Unlimbo() and Update() is completely uncertain.
	// Target will update location after techno firing, which may result in inaccurate
	// target position recorded by the LastTargetCoord in Unlimbo(). Therefore, it's
	// necessary to record the position during the first Update(). - CrimRecya
	if (this->WaitOneFrame == 2)
	{
		if (const auto pTarget = this->Bullet->Target)
		{
			this->LastTargetCoord = pTarget->GetCoords();
			this->WaitOneFrame = 1;
			return true;
		}
	}

	this->WaitOneFrame = 0;
	this->FireTrajectory();

	return false;
}

CoordStruct LiveShellTrajectory::GetOnlyStableOffsetCoords(double rotateRadian)
{
	const auto pType = static_cast<const LiveShellTrajectoryType*>(this->GetType());
	auto offsetCoord = pType->OffsetCoord.Get();

	if (pType->MirrorCoord && this->CurrentBurst & 1)
		offsetCoord.Y = -(offsetCoord.Y);

	return CoordStruct
		{
			static_cast<int>(offsetCoord.X * Math::cos(rotateRadian) + offsetCoord.Y * Math::sin(rotateRadian)),
			static_cast<int>(offsetCoord.X * Math::sin(rotateRadian) - offsetCoord.Y * Math::cos(rotateRadian)),
			offsetCoord.Z
		};
}

CoordStruct LiveShellTrajectory::GetInaccurateTargetCoords(const CoordStruct& baseCoord, double distance)
{
	const auto pTypeExt = BulletTypeExt::ExtMap.Find(this->Bullet->Type);
	const auto offsetMult = 0.0004 * distance;
	const auto offsetMin = static_cast<int>(offsetMult * pTypeExt->BallisticScatter_Min.Get(Leptons(0)));
	const auto offsetMax = static_cast<int>(offsetMult * pTypeExt->BallisticScatter_Max.Get(Leptons(RulesClass::Instance->BallisticScatter)));
	const auto offsetDistance = ScenarioClass::Instance->Random.RandomRanged(offsetMin, offsetMax);
	return MapClass::GetRandomCoordsNear(baseCoord, offsetDistance, false);
}

void LiveShellTrajectory::DisperseBurstSubstitution(double baseRadian)
{
	const auto pType = static_cast<const LiveShellTrajectoryType*>(this->GetType());
	const auto axis = pType->AxisOfRotation.Get();

	// Calculate the actual rotation axis
	BulletVelocity rotationAxis
	{
		axis.X * Math::cos(baseRadian) + axis.Y * Math::sin(baseRadian),
		axis.X * Math::sin(baseRadian) - axis.Y * Math::cos(baseRadian),
		static_cast<double>(axis.Z)
	};

	double extraRotate = 0.0;

	if (pType->MirrorCoord)
	{
		if (this->CurrentBurst & 1)
			rotationAxis *= -1;

		extraRotate = Math::Pi * (pType->RotateCoord * ((this->CurrentBurst / 2) / (this->CountOfBurst - 1.0) - 0.5)) / 180;
	}
	else
	{
		extraRotate = Math::Pi * (pType->RotateCoord * (this->CurrentBurst / (this->CountOfBurst - 1.0) - 0.5)) / 180;
	}

	// Rotate the selected angle
	this->MovingVelocity = LiveShellTrajectory::RotateAboutTheAxis(this->MovingVelocity, rotationAxis, extraRotate);
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

void VirtualTrajectory::OnUnlimbo()
{
	this->PhobosTrajectory::OnUnlimbo();

	this->RemainingDistance = INT_MAX;

	for (auto pTrans = this->Bullet->Owner; pTrans; pTrans = pTrans->Transporter)
		this->SurfaceFirerID = pTrans->UniqueID;
}

bool VirtualTrajectory::OnAI()
{
	if (this->OnAIDetonateCheck())
		return true;

	this->OnAIVelocityCheck();

	if (this->PhobosTrajectory::OnAI())
		return true;

	this->OnAINextFrameCheck();

	return false;
}

bool VirtualTrajectory::OnAIDetonateCheck()
{
	if (!this->NotMainWeapon && this->InvalidFireCondition(this->Bullet->Owner))
		return true;

	return this->PhobosTrajectory::OnAIDetonateCheck();
}

bool VirtualTrajectory::InvalidFireCondition(TechnoClass* pTechno)
{
	if (!pTechno)
		return true;

	for (auto pTrans = pTechno->Transporter; pTrans; pTrans = pTrans->Transporter)
		pTechno = pTrans;

	if (!TechnoExt::IsActive(pTechno) || this->SurfaceFirerID != pTechno->UniqueID)
		return true;

	if (static_cast<const VirtualTrajectoryType*>(this->GetType())->AllowFirerTurning)
		return false;

	const auto SourceCrd = pTechno->GetCoords();
	const auto TargetCrd = this->Bullet->TargetCoords;

	const auto rotateAngle = Math::atan2(TargetCrd.Y - SourceCrd.Y , TargetCrd.X - SourceCrd.X);
	const auto tgtDir = DirStruct(-rotateAngle);

	const auto& face = pTechno->HasTurret() && pTechno->WhatAmI() == AbstractType::Unit ? pTechno->SecondaryFacing : pTechno->PrimaryFacing;
	const auto curDir = face.Current();

	// Similar to the vanilla 45 degree turret facing check design
	return (std::abs(static_cast<short>(static_cast<short>(tgtDir.Raw) - static_cast<short>(curDir.Raw))) >= 4096);
}
