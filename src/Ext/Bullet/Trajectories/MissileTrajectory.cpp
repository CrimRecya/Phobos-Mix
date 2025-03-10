#include "MissileTrajectory.h"
#include "StraightTrajectory.h"
#include "BombardTrajectory.h"
#include "EngraveTrajectory.h"
#include "ParabolaTrajectory.h"
#include "TracingTrajectory.h"

#include <AnimClass.h>
#include <LaserDrawClass.h>
#include <EBolt.h>
#include <RadBeam.h>
#include <ParticleSystemClass.h>
#include <ScenarioClass.h>
#include <AircraftTrackerClass.h>

#include <Ext/Bullet/Body.h>
#include <Ext/BulletType/Body.h>
#include <Ext/Techno/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/Anim/Body.h>
#include <Utilities/EnumFunctions.h>
#include <Utilities/Helpers.Alex.h>

std::unique_ptr<PhobosTrajectory> MissileTrajectoryType::CreateInstance(BulletClass* pBullet) const
{
	return std::make_unique<MissileTrajectory>(this, pBullet);
}

template<typename T>
void MissileTrajectoryType::Serialize(T& Stm)
{
	Stm
		.Process(this->UniqueCurve)
		.Process(this->PreAimCoord)
		.Process(this->RotateCoord)
		.Process(this->MirrorCoord)
		.Process(this->FacingCoord)
		.Process(this->ReduceCoord)
		.Process(this->UseDisperseBurst)
		.Process(this->AxisOfRotation)
		.Process(this->LaunchSpeed)
		.Process(this->Acceleration)
		.Process(this->TurningSpeed)
		.Process(this->LockDirection)
		.Process(this->CruiseEnable)
		.Process(this->CruiseUnableRange)
		.Process(this->CruiseAltitude)
		.Process(this->CruiseAlongLevel)
		.Process(this->LeadTimeCalculate)
		.Process(this->RecordSourceCoord)
		.Process(this->RetargetRadius)
		.Process(this->RetargetAllies)
		.Process(this->TargetSnapDistance)
		.Process(this->SuicideAboveRange)
		.Process(this->SuicideShortOfROT)
		.Process(this->SuicideIfNoWeapon)
		.Process(this->Weapons)
		.Process(this->WeaponBurst)
		.Process(this->WeaponCount)
		.Process(this->WeaponDelay)
		.Process(this->WeaponInitialDelay)
		.Process(this->WeaponEffectiveRange)
		.Process(this->WeaponSeparate)
		.Process(this->WeaponRetarget)
		.Process(this->WeaponLocation)
		.Process(this->WeaponTendency)
		.Process(this->WeaponHolistic)
		.Process(this->WeaponMarginal)
		.Process(this->WeaponToAllies)
		.Process(this->WeaponDoRepeat)
		;
}

bool MissileTrajectoryType::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->LiveShellTrajectoryType::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool MissileTrajectoryType::Save(PhobosStreamWriter& Stm) const
{
	this->LiveShellTrajectoryType::Save(Stm);
	const_cast<MissileTrajectoryType*>(this)->Serialize(Stm);
	return true;
}

void MissileTrajectoryType::Read(CCINIClass* const pINI, const char* pSection)
{
	this->PhobosTrajectoryType::Read(pINI, pSection);
	INI_EX exINI(pINI);

	this->UniqueCurve.Read(exINI, pSection, "Trajectory.Missile.UniqueCurve");
	this->PreAimCoord.Read(exINI, pSection, "Trajectory.Missile.PreAimCoord");
	this->RotateCoord.Read(exINI, pSection, "Trajectory.Missile.RotateCoord");
	this->MirrorCoord.Read(exINI, pSection, "Trajectory.Missile.MirrorCoord");
	this->FacingCoord.Read(exINI, pSection, "Trajectory.Missile.FacingCoord");
	this->ReduceCoord.Read(exINI, pSection, "Trajectory.Missile.ReduceCoord");
	this->UseDisperseBurst.Read(exINI, pSection, "Trajectory.Missile.UseDisperseBurst");
	this->AxisOfRotation.Read(exINI, pSection, "Trajectory.Missile.AxisOfRotation");
	this->LaunchSpeed.Read(exINI, pSection, "Trajectory.Missile.LaunchSpeed");
	this->LaunchSpeed = Math::max(0.001, this->LaunchSpeed);
	this->Acceleration.Read(exINI, pSection, "Trajectory.Missile.Acceleration");
	this->TurningSpeed.Read(exINI, pSection, "Trajectory.Missile.TurningSpeed");
	this->TurningSpeed = Math::max(0.0, this->TurningSpeed);
	this->LockDirection.Read(exINI, pSection, "Trajectory.Missile.LockDirection");
	this->CruiseEnable.Read(exINI, pSection, "Trajectory.Missile.CruiseEnable");
	this->CruiseUnableRange.Read(exINI, pSection, "Trajectory.Missile.CruiseUnableRange");
	this->CruiseUnableRange = Leptons(Math::max(128, this->CruiseUnableRange.Get()));
	this->CruiseAltitude.Read(exINI, pSection, "Trajectory.Missile.CruiseAltitude");
	this->CruiseAlongLevel.Read(exINI, pSection, "Trajectory.Missile.CruiseAlongLevel");
	this->LeadTimeCalculate.Read(exINI, pSection, "Trajectory.Missile.LeadTimeCalculate");
	this->RecordSourceCoord.Read(exINI, pSection, "Trajectory.Missile.RecordSourceCoord");
	this->RetargetAllies.Read(exINI, pSection, "Trajectory.Missile.RetargetAllies");
	this->RetargetRadius.Read(exINI, pSection, "Trajectory.Missile.RetargetRadius");
	this->TargetSnapDistance.Read(exINI, pSection, "Trajectory.Missile.TargetSnapDistance");
	this->SuicideAboveRange.Read(exINI, pSection, "Trajectory.Missile.SuicideAboveRange");
	this->SuicideShortOfROT.Read(exINI, pSection, "Trajectory.Missile.SuicideShortOfROT");
	this->SuicideIfNoWeapon.Read(exINI, pSection, "Trajectory.Missile.SuicideIfNoWeapon");
	this->Weapons.Read(exINI, pSection, "Trajectory.Missile.Weapons");
	this->WeaponBurst.Read(exINI, pSection, "Trajectory.Missile.WeaponBurst");
	this->WeaponCount.Read(exINI, pSection, "Trajectory.Missile.WeaponCount");
	this->WeaponDelay.Read(exINI, pSection, "Trajectory.Missile.WeaponDelay");
	this->WeaponDelay = Math::max(1, this->WeaponDelay);
	this->WeaponInitialDelay.Read(exINI, pSection, "Trajectory.Missile.WeaponInitialDelay");
	this->WeaponEffectiveRange.Read(exINI, pSection, "Trajectory.Missile.WeaponEffectiveRange");
	this->WeaponSeparate.Read(exINI, pSection, "Trajectory.Missile.WeaponSeparate");
	this->WeaponRetarget.Read(exINI, pSection, "Trajectory.Missile.WeaponRetarget");
	this->WeaponLocation.Read(exINI, pSection, "Trajectory.Missile.WeaponLocation");
	this->WeaponTendency.Read(exINI, pSection, "Trajectory.Missile.WeaponTendency");
	this->WeaponHolistic.Read(exINI, pSection, "Trajectory.Missile.WeaponHolistic");
	this->WeaponMarginal.Read(exINI, pSection, "Trajectory.Missile.WeaponMarginal");
	this->WeaponToAllies.Read(exINI, pSection, "Trajectory.Missile.WeaponToAllies");
	this->WeaponDoRepeat.Read(exINI, pSection, "Trajectory.Missile.WeaponDoRepeat");
}

template<typename T>
void MissileTrajectory::Serialize(T& Stm)
{
	Stm
		.Process(this->Type)
		.Process(this->Speed)
		.Process(this->PreAimCoord)
		.Process(this->UseDisperseBurst)
		.Process(this->CruiseEnable)
		.Process(this->SuicideAboveRange)
		.Process(this->WeaponCount)
		.Process(this->WeaponTimer)
		.Process(this->InStraight)
		.Process(this->Accelerate)
		.Process(this->TargetInTheAir)
		.Process(this->TargetIsTechno)
		.Process(this->OriginalDistance)
		.Process(this->CurrentBurst)
		.Process(this->ThisWeaponIndex)
		.Process(this->LastTargetCoord)
		.Process(this->PreAimDistance)
		.Process(this->LastDotProduct)
		.Process(this->FLHCoord)
		.Process(this->BuildingCoord)
		.Process(this->FirepowerMult)
		;
}

bool MissileTrajectory::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->LiveShellTrajectory::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool MissileTrajectory::Save(PhobosStreamWriter& Stm) const
{
	this->LiveShellTrajectory::Save(Stm);
	const_cast<MissileTrajectory*>(this)->Serialize(Stm);
	return true;
}

void MissileTrajectory::OnUnlimbo(BulletClass* pBullet)
{
	this->LiveShellTrajectory::OnUnlimbo(pBullet);

	// Record the initial distance
	this->OriginalDistance = static_cast<int>(pBullet->TargetCoords.DistanceFrom(pBullet->SourceCoords));

	if (!BulletExt::ExtMap.Find(pBullet)->DispersedTrajectory)
		this->OpenFire(pBullet);
}

bool MissileTrajectory::OnAI(BulletClass* pBullet)
{
	// Immediately detonate below ground level
	if (MapClass::Instance->GetCellFloorHeight(pBullet->Location) > pBullet->Location.Z)
		return true;

	const auto pType = this->Type;

	// 154 -> 0.6 * Unsorted::LeptonsPerCell (Used to ensure correct hit at the fixed speed)
	if (pBullet->TargetCoords.DistanceFrom(pBullet->Location) < (pType->UniqueCurve ? 154 : pType->TargetSnapDistance.Get()))
		return true;

	// TODO

	// Calculate new speed
	return (pType->UniqueCurve ? this->CurveVelocityChange(pBullet) : this->NotCurveVelocityChange(pBullet));
}

void MissileTrajectory::OnAIPreDetonate(BulletClass* pBullet)
{
	const auto pTarget = abstract_cast<ObjectClass*>(pBullet->Target);
	const auto coords = pTarget ? pTarget->GetCoords() : pBullet->TargetCoords;
	const auto pType = this->Type;

	// Whether to snap to target?
	if (coords.DistanceFrom(pBullet->Location) <= pType->TargetSnapDistance.Get())
	{
		const auto pExt = BulletExt::ExtMap.Find(pBullet);
		pExt->SnappedToTarget = true;
		pBullet->SetLocation(coords);
	}
}

void MissileTrajectory::OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition)
{
	// We don't want to take the gravity into account
	pSpeed->Z += BulletTypeExt::GetAdjustedGravity(pBullet->Type);

	// Check if the bullet needs to slow down the speed since it will pass through the target
	if (this->LastDotProduct <= 0)
		return;

	const auto velocity = pSpeed->Magnitude();
	const auto distance = pBullet->Location.DistanceFrom(pBullet->TargetCoords);

	if (velocity > distance)
		*pSpeed *= distance / velocity;
}

bool MissileTrajectory::OpenFire(BulletClass* pBullet)
{
	const auto pType = this->Type;

	// Set the initial launch state of the projectile
	if (pType->UniqueCurve) // Simulate ballistic missile trajectory
	{
		// Basic speed
		pBullet->Velocity.X = 0;
		pBullet->Velocity.Y = 0;
		pBullet->Velocity.Z = 4.0;

		// Do not allow rotation in the initial direction
		this->UseDisperseBurst = false;

		// OriginalDistance is converted to record the maximum height
		if (this->OriginalDistance < (Unsorted::LeptonsPerCell * 5)) // When the distance is very close, the trajectory tends to be parabolic
			this->OriginalDistance = static_cast<int>(this->OriginalDistance * 1.2) + (Unsorted::LeptonsPerCell * 2);
		else if (this->OriginalDistance > (Unsorted::LeptonsPerCell * 15)) // When the distance is far enough, it is the complete trajectory
			this->OriginalDistance = static_cast<int>(this->OriginalDistance * 0.4) + (Unsorted::LeptonsPerCell * 2);
		else // The distance is neither long nor short, it is an adaptive trajectory
			this->OriginalDistance = (Unsorted::LeptonsPerCell * 8);

		// Calculate the maximum height during the ascending phase
		this->OriginalDistance = this->OriginalDistance < 3200 ? this->OriginalDistance / 2 : this->OriginalDistance - 1600;
	}
	else // Under normal circumstances, the trajectory is similar to ROT projectile with an initial launch direction
	{
		if (pType->SuicideAboveRange < 0)
			this->SuicideAboveRange = this->OriginalDistance * (-pType->SuicideAboveRange);
		else if (pType->SuicideAboveRange > 0)
			this->SuicideAboveRange = Unsorted::LeptonsPerCell * pType->SuicideAboveRange;

		// Without setting an initial direction, it will be launched directly towards the target
		if (this->PreAimCoord == CoordStruct::Empty)
		{
			this->InStraight = true;
			pBullet->Velocity.X = pBullet->TargetCoords.X - pBullet->SourceCoords.X;
			pBullet->Velocity.Y = pBullet->TargetCoords.Y - pBullet->SourceCoords.Y;
			pBullet->Velocity.Z = pBullet->TargetCoords.Z - pBullet->SourceCoords.Z;
		}
		else
		{
			this->PreAimDistance = this->PreAimCoord.Magnitude();

			// When the distance is short, the initial moving distance will be reduced
			if (pType->ReduceCoord && this->OriginalDistance < (Unsorted::LeptonsPerCell * 10))
				this->PreAimDistance *= this->OriginalDistance / (Unsorted::LeptonsPerCell * 10);

			this->PreAimDistance += this->Speed;
			this->InitializeBulletNotCurve(pBullet);
		}

		// Calculate speed
		if (this->CalculateBulletVelocity(pBullet, this->Speed))
			this->SuicideAboveRange = 0.001;
	}
}

CoordStruct MissileTrajectory::GetRetargetCenter(const BulletClass* const pBullet) const
{
	// When in the tracking phase, it only retarget within the range in front of it
	if (!this->InStraight)
		return pBullet->TargetCoords;

	const auto futureVelocity = pBullet->Velocity * ((this->Type->RetargetRadius * Unsorted::LeptonsPerCell) / this->Speed);
	return CoordStruct { pBullet->Location.X + static_cast<int>(futureVelocity.X), pBullet->Location.Y + static_cast<int>(futureVelocity.Y), pBullet->Location.Z };
}

void MissileTrajectory::SetBulletNewTarget(BulletClass* const pBullet, AbstractClass* const pTarget)
{
	pBullet->SetTarget(pTarget);
	pBullet->TargetCoords = pTarget->GetCoords();
	this->LastTargetCoord = pBullet->TargetCoords;

	if (this->Type->CruiseEnable)
		this->CruiseEnable = true;
}

void MissileTrajectory::InitializeBulletNotCurve(BulletClass* pBullet)
{
	const auto pType = this->Type;
	double rotateAngle = 0.0;
	const auto pFirer = pBullet->Owner;
	const auto theSource = pFirer ? pFirer->GetCoords() : pBullet->SourceCoords;

	// Calculate the orientation of the coordinate system
	if ((pType->FacingCoord || (pBullet->TargetCoords.Y == theSource.Y && pBullet->TargetCoords.X == theSource.X)) && pFirer)
	{
		if (pFirer->HasTurret())
			rotateAngle = -(pFirer->TurretFacing().GetRadian<32>());
		else
			rotateAngle = -(pFirer->PrimaryFacing.Current().GetRadian<32>());
	}
	else
	{
		rotateAngle = Math::atan2(pBullet->TargetCoords.Y - theSource.Y , pBullet->TargetCoords.X - theSource.X);
	}

	// Determine the firing velocity vector of the bullet
	if (!this->CalculateReducedVelocity(pBullet, rotateAngle))
	{
		pBullet->Velocity.X = this->PreAimCoord.X * Math::cos(rotateAngle) + this->PreAimCoord.Y * Math::sin(rotateAngle);
		pBullet->Velocity.Y = this->PreAimCoord.X * Math::sin(rotateAngle) - this->PreAimCoord.Y * Math::cos(rotateAngle);
		pBullet->Velocity.Z = this->PreAimCoord.Z;
	}

	// Rotate the selected angle
	if (!this->UseDisperseBurst && std::abs(pType->RotateCoord) > 1e-10 && pBullet->WeaponType && pBullet->WeaponType->Burst > 1)
	{
		const auto axis = pType->AxisOfRotation.Get();

		BulletVelocity rotationAxis
		{
			axis.X * Math::cos(rotateAngle) + axis.Y * Math::sin(rotateAngle),
			axis.X * Math::sin(rotateAngle) - axis.Y * Math::cos(rotateAngle),
			static_cast<double>(axis.Z)
		};

		double extraRotate = 0.0;

		if (pType->MirrorCoord)
		{
			if (this->CurrentBurst % 2 == 1)
				rotationAxis *= -1;

			extraRotate = Math::Pi * (pType->RotateCoord * ((this->CurrentBurst / 2) / (pBullet->WeaponType->Burst - 1.0) - 0.5)) / 180;
		}
		else
		{
			extraRotate = Math::Pi * (pType->RotateCoord * (this->CurrentBurst / (pBullet->WeaponType->Burst - 1.0) - 0.5)) / 180;
		}

		pBullet->Velocity = this->RotateAboutTheAxis(pBullet->Velocity, rotationAxis, extraRotate);
	}
}

inline bool MissileTrajectory::CalculateReducedVelocity(BulletClass* pBullet, double rotateAngle)
{
	const auto pType = this->Type;

	if (!pType->ReduceCoord || pType->TurningSpeed <= 1e-10)
		return false;

	// Check if its steering ability is sufficient
	const auto coordMult = (this->OriginalDistance * pType->TurningSpeed / (Unsorted::LeptonsPerCell * 90 / 2));

	if (coordMult >= 1.0)
		return false;

	const BulletVelocity theAimCoord
	{
		this->PreAimCoord.X * Math::cos(rotateAngle) + this->PreAimCoord.Y * Math::sin(rotateAngle),
		this->PreAimCoord.X * Math::sin(rotateAngle) - this->PreAimCoord.Y * Math::cos(rotateAngle),
		static_cast<double>(this->PreAimCoord.Z)
	};
	const BulletVelocity theDistance
	{
		static_cast<double>(pBullet->TargetCoords.X - pBullet->SourceCoords.X),
		static_cast<double>(pBullet->TargetCoords.Y - pBullet->SourceCoords.Y),
		static_cast<double>(pBullet->TargetCoords.Z - pBullet->SourceCoords.Z)
	};

	// Reduce the initial rotation angle
	pBullet->Velocity = (theDistance - theAimCoord) * (1 - coordMult) + theAimCoord;
	return true;
}

bool MissileTrajectory::CurveVelocityChange(BulletClass* pBullet)
{
	const auto pTarget = pBullet->Target;
	const auto pTargetTechno = abstract_cast<TechnoClass*>(pTarget);
	const bool checkValid = (pTarget && pTarget->WhatAmI() == AbstractType::Bullet) || (pTargetTechno && !CheckTechnoIsInvalid(pTargetTechno));
	auto targetLocation = pBullet->TargetCoords;

	// Follow and track the target like a missile
	if (checkValid)
		targetLocation = pTarget->GetCoords();

	pBullet->TargetCoords = targetLocation;

	// Update projectile velocity based on stage
	if (!this->InStraight) // In the launch phase
	{
		const CoordStruct horizonVelocity { targetLocation.X - pBullet->Location.X, targetLocation.Y - pBullet->Location.Y, 0 };
		const auto horizonDistance = horizonVelocity.Magnitude();

		if (horizonDistance > 0)
		{
			// Slowly step up
			auto horizonMult = std::abs(pBullet->Velocity.Z / 64.0) / horizonDistance;
			pBullet->Velocity.X += horizonMult * horizonVelocity.X;
			pBullet->Velocity.Y += horizonMult * horizonVelocity.Y;
			const auto horizonLength = sqrt(pBullet->Velocity.X * pBullet->Velocity.X + pBullet->Velocity.Y * pBullet->Velocity.Y);

			// Limit horizontal maximum speed
			if (horizonLength > 64.0)
			{
				horizonMult = 64.0 / horizonLength;
				pBullet->Velocity.X *= horizonMult;
				pBullet->Velocity.Y *= horizonMult;
			}
		}

		// The launch phase is divided into ascending and descending stages
		if ((pBullet->Location.Z - pBullet->SourceCoords.Z) < this->OriginalDistance && this->Accelerate)
		{
			if (pBullet->Velocity.Z < 160.0) // Accelerated phase of ascent
				pBullet->Velocity.Z += 4.0;
		}
		else // End of ascent
		{
			this->Accelerate = false;
			// Predict the lowest position
			const auto futureHeight = pBullet->Location.Z + 8 * pBullet->Velocity.Z;

			// Start decelerating/accelerating downwards
			if (pBullet->Velocity.Z > -160.0)
				pBullet->Velocity.Z -= 4.0;

			// Enter gliding phase below predicted altitude
			if (futureHeight <= targetLocation.Z || futureHeight <= pBullet->SourceCoords.Z)
				this->InStraight = true;
		}
	}
	else // In the gliding stage
	{
		// Predict hit time
		const auto timeMult = targetLocation.DistanceFrom(pBullet->Location) / 192.0;
		targetLocation.Z += static_cast<int>(timeMult * 48);

		// Calculate the target lead time
		if (checkValid)
		{
			targetLocation.X += static_cast<int>(timeMult * (targetLocation.X - this->LastTargetCoord.X));
			targetLocation.Y += static_cast<int>(timeMult * (targetLocation.Y - this->LastTargetCoord.Y));
		}

		// Stable the fixed flight speed
		auto trajectorySpeed = pBullet->Velocity.Magnitude();

		if (trajectorySpeed < 192.0)
			trajectorySpeed += 4.0;

		if (trajectorySpeed > 192.0)
			trajectorySpeed = 192.0;

		// Calculate the speed change during gliding phase using common steering algorithm
		if (this->ChangeBulletVelocity(pBullet, targetLocation) || this->CalculateBulletVelocity(pBullet, trajectorySpeed))
			return true;
	}

	return false;
}

bool MissileTrajectory::NotCurveVelocityChange(BulletClass* pBullet)
{
	const auto pType = this->Type;

	// Calculate the distance flown
	if (this->SuicideAboveRange > 0)
	{
		this->SuicideAboveRange -= this->Speed;

		if (this->SuicideAboveRange <= 0)
			return true;
	}

	if (this->PreAimDistance > 0)
		this->PreAimDistance -= this->Speed;

	bool velocityUp = false;

	// Calculate speed
	if (this->Accelerate && std::abs(pType->Acceleration) > 1e-10)
	{
		this->Speed += pType->Acceleration;

		// Judging whether to accelerate or decelerate based on acceleration
		if (pType->Acceleration > 0)
		{
			if (this->Speed >= pType->Speed)
			{
				this->Speed = pType->Speed;
				this->Accelerate = false;
			}
		}
		else if (this->Speed <= pType->Speed)
		{
			this->Speed = pType->Speed;
			this->Accelerate = false;
		}

		velocityUp = true;
	}

	// Calculate steering
	if (!pType->LockDirection || !this->InStraight)
	{
		// Check if the target needs to be changed
		if (std::abs(pType->RetargetRadius) > 1e-10 && this->BulletRetargetTechno(pBullet))
			return true;

		// Make the turn
		if (this->PreAimDistance <= 0 && this->StandardVelocityChange(pBullet))
			return true;

		velocityUp = true;
	}

	// Calculate velocity vector
	return velocityUp && this->CalculateBulletVelocity(pBullet, this->Speed);
}

bool MissileTrajectory::StandardVelocityChange(BulletClass* pBullet)
{
	const auto pType = this->Type;
	const auto pTarget = pBullet->Target;
	const auto pTargetTechno = abstract_cast<TechnoClass*>(pTarget);
	const bool checkValid = (pTarget && pTarget->WhatAmI() == AbstractType::Bullet) || (pTargetTechno && !CheckTechnoIsInvalid(pTargetTechno));
	auto targetLocation = pBullet->TargetCoords;

	// Follow and track the target like a missile
	if (checkValid)
		targetLocation = pTarget->GetCoords();

	pBullet->TargetCoords = targetLocation;

	// If the speed is too low, it will cause the lead time calculation results to be too far away and unable to be used
	if (pType->LeadTimeCalculate && checkValid && (pType->UniqueCurve || pType->Speed > 64.0))
	{
		const auto leadSpeed = (pType->Speed + this->Speed) / 2;
		const auto timeMult = targetLocation.DistanceFrom(pBullet->Location) / leadSpeed;
		targetLocation += (targetLocation - this->LastTargetCoord) * timeMult;
	}

	// If in the cruise phase, the steering target will be set at the fixed height
	if (this->CruiseEnable)
	{
		const auto horizontal = Point2D { targetLocation.X - pBullet->Location.X, targetLocation.Y - pBullet->Location.Y };
		const auto horizontalDistance = horizontal.Magnitude();

		// The distance is still long, continue cruising
		if (horizontalDistance > pType->CruiseUnableRange.Get())
		{
			const auto ratio = this->Speed / horizontalDistance;
			targetLocation.X = pBullet->Location.X + static_cast<int>(horizontal.X * ratio);
			targetLocation.Y = pBullet->Location.Y + static_cast<int>(horizontal.Y * ratio);
			const auto altitude = pType->CruiseAltitude + (pType->CruiseAlongLevel ? MapClass::Instance->GetCellFloorHeight(pBullet->Location) : pBullet->SourceCoords.Z);
			targetLocation.Z = (altitude + pBullet->Location.Z) / 2;
		}
		else
		{
			this->CruiseEnable = false;
			this->LastDotProduct = 0;
		}
	}

	// Calculate the velocity direction change
	return this->ChangeBulletVelocity(pBullet, targetLocation);
}

bool MissileTrajectory::ChangeBulletVelocity(BulletClass* pBullet, const CoordStruct& targetLocation)
{
	const auto pType = this->Type;
	const auto bulletVelocity = pBullet->Velocity;
	const auto targetVelocity = BulletVelocity
	{
		static_cast<double>(targetLocation.X - pBullet->Location.X),
		static_cast<double>(targetLocation.Y - pBullet->Location.Y),
		static_cast<double>(targetLocation.Z - pBullet->Location.Z)
	};

	// Calculate the new velocity vector based on turning speed
	const auto dotProduct = (targetVelocity * bulletVelocity);
	const auto cosTheta = dotProduct / sqrt(targetVelocity.MagnitudeSquared() * bulletVelocity.MagnitudeSquared());
	const auto radian = Math::acos(Math::clamp(cosTheta, -1.0, 1.0)); // Ensure that the result range of cos is correct
	const auto turningRadius = (pType->UniqueCurve ? 10.0 : pType->TurningSpeed) * (Math::TwoPi / 360);

	if (std::abs(radian) > turningRadius) // The angle that needs to be rotated is relatively large
	{
		// Calculate the rotation axis
		auto rotationAxis = targetVelocity.CrossProduct(bulletVelocity);

		// Substitute to calculate new velocity
		pBullet->Velocity = this->RotateAboutTheAxis(bulletVelocity, rotationAxis, (radian < 0 ? turningRadius : -turningRadius));

		// Check if the steering ability is insufficient
		if (!pType->UniqueCurve && pType->SuicideShortOfROT && dotProduct <= 0 && (this->InStraight || this->LastDotProduct > 0))
			return true;
	}
	else // When the angle is small, aim directly at the target
	{
		pBullet->Velocity = targetVelocity;
		this->InStraight = true;
	}

	// Record the current value for subsequent checks
	this->LastDotProduct = dotProduct;
	this->LastTargetCoord = pBullet->TargetCoords;

	return false;
}
