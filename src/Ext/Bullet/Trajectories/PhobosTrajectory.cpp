#include "PhobosTrajectory.h"
#include "StraightTrajectory.h"
#include "BombardTrajectory.h"
#include "MissileTrajectory.h"
#include "EngraveTrajectory.h"
#include "ParabolaTrajectory.h"
#include "TracingTrajectory.h"

#include <OverlayTypeClass.h>

#include <Ext/Bullet/Body.h>
#include <Ext/Techno/Body.h>

TrajectoryTypePointer::TrajectoryTypePointer(TrajectoryFlag flag)
{
	switch (flag)
	{
	case TrajectoryFlag::Straight:
		_ptr = std::make_unique<StraightTrajectoryType>();
		return;
	case TrajectoryFlag::Bombard:
		_ptr = std::make_unique<BombardTrajectoryType>();
		return;
	case TrajectoryFlag::Missile:
		_ptr = std::make_unique<MissileTrajectoryType>();
		return;
	case TrajectoryFlag::Engrave:
		_ptr = std::make_unique<EngraveTrajectoryType>();
		return;
	case TrajectoryFlag::Parabola:
		_ptr = std::make_unique<ParabolaTrajectoryType>();
		return;
	case TrajectoryFlag::Tracing:
		_ptr = std::make_unique<TracingTrajectoryType>();
		return;
	}
	_ptr.reset();
}

namespace detail
{
	template <>
	inline bool read<TrajectoryFlag>(TrajectoryFlag& value, INI_EX& parser, const char* pSection, const char* pKey)
	{
		if (parser.ReadString(pSection, pKey))
		{
			static std::pair<const char*, TrajectoryFlag> FlagNames[] =
			{
				{"Straight", TrajectoryFlag::Straight},
				{"Bombard" ,TrajectoryFlag::Bombard},
				{"Missile", TrajectoryFlag::Missile},
				{"Engrave" ,TrajectoryFlag::Engrave},
				{"Parabola", TrajectoryFlag::Parabola},
				{"Tracing" ,TrajectoryFlag::Tracing},
			};
			for (auto [name, flag] : FlagNames)
			{
				if (_strcmpi(parser.value(), name) == 0)
				{
					value = flag;
					return true;
				}
			}
			Debug::INIParseFailed(pSection, pKey, parser.value(), "Expected a new trajectory type");
		}

		return false;
	}
}

void TrajectoryTypePointer::LoadFromINI(CCINIClass* pINI, const char* pSection)
{
	INI_EX exINI(pINI);
	Nullable<TrajectoryFlag> flag;
	flag.Read(exINI, pSection, "Trajectory");// I assume this shit is parsed once and only once, so I keep the impl here
	if (flag.isset())
	{
		if (!_ptr || _ptr->Flag() != flag.Get())
			std::construct_at(this, flag.Get());
	}
	if (_ptr)
		_ptr->Read(pINI, pSection);
}

bool TrajectoryTypePointer::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	PhobosTrajectoryType* PTR = nullptr;
	if (!Stm.Load(PTR))
		return false;
	TrajectoryFlag flag;
	if (PTR && Stm.Load(flag))
	{
		std::construct_at(this, flag);
		PhobosSwizzle::RegisterChange(PTR, _ptr.get());
		return _ptr->Load(Stm, RegisterForChange);
	}
	return true;
}

bool TrajectoryTypePointer::Save(PhobosStreamWriter& Stm) const
{
	auto* raw = get();
	Stm.Save(raw);
	if (raw)
	{
		auto rtti = raw->Flag();
		Stm.Save(rtti);
		return raw->Save(Stm);
	}
	return true;
}

bool TrajectoryPointer::Load(PhobosStreamReader& Stm, bool registerForChange)
{
	PhobosTrajectory* PTR = nullptr;
	if (!Stm.Load(PTR))
		return false;
	TrajectoryFlag flag = TrajectoryFlag::Invalid;
	if (PTR && Stm.Load(flag))
	{
		switch (flag)
		{
		case TrajectoryFlag::Straight:
			_ptr = std::make_unique<StraightTrajectory>(noinit_t {});
			break;
		case TrajectoryFlag::Bombard:
			_ptr = std::make_unique<BombardTrajectory>(noinit_t {});
			break;
		case TrajectoryFlag::Missile:
			_ptr = std::make_unique<MissileTrajectory>(noinit_t {});
			break;
		case TrajectoryFlag::Engrave:
			_ptr = std::make_unique<EngraveTrajectory>(noinit_t {});
			break;
		case TrajectoryFlag::Parabola:
			_ptr = std::make_unique<ParabolaTrajectory>(noinit_t {});
			break;
		case TrajectoryFlag::Tracing:
			_ptr = std::make_unique<TracingTrajectory>(noinit_t {});
			break;
		default:
			_ptr.reset();
			break;
		}
		if (_ptr.get())
		{
			// PhobosSwizzle::RegisterChange(PTR, _ptr.get()); // not used elsewhere yet, if anyone does then reenable this shit
			return _ptr->Load(Stm, registerForChange);
		}
	}
	return true;
}

bool TrajectoryPointer::Save(PhobosStreamWriter& Stm) const
{
	auto* raw = get();
	Stm.Save(raw);
	if (raw)
	{
		auto rtti = raw->Flag();
		Stm.Save(rtti);
		return raw->Save(Stm);
	}
	return true;
}

// ------------------------------------------------------------------------------ //

void PhobosTrajectory::OnUnlimbo()
{
	const auto pBullet = this->Bullet;
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
		const auto burst = pFirer->CurrentBurstIndex;
		this->CurrentBurst = (burst & 1) ? (-burst - 1) : burst;
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
		this->PassDetonateTimer.Start(pType->PassDetonateInitialDelay);
	// Initialize additional weapons
	if (!pType->DisperseWeapons.empty() && !pType->DisperseCounts.empty() && this->DisperseCycle)
	{
		this->DisperseCount = pType->DisperseCounts[0];
		this->DisperseTimer.Start(pType->DisperseInitialDelay);
	}
}

bool PhobosTrajectory::OnAI()
{
	this->ChangeBulletFacing();
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
	double ratio = 1.0;

	const auto pType = this->GetType();
	const bool checkThrough = (!pType->ThroughBuilding || !pType->ThroughVehicles);
	const auto velocity = PhobosTrajectory::Get2DVelocity(this->MovingVelocity);
	// Low speed with checkSubject was already done well
	if (velocity < Unsorted::LeptonsPerCell)
	{
		// Blocked by obstacles?
		if (checkThrough)
		{
			const auto pFirer = pBullet->Owner;
			const auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;

			if (this->CheckThroughAndSubjectInCell(MapClass::Instance->GetCellAt(pBullet->Location), pOwner))
			{
				if (32.0 < velocity)
					ratio = (32.0 / velocity);
			}
		}
		// Check whether about to fall into the ground
		if (std::abs(this->MovingVelocity.Z) > Unsorted::CellHeight)
		{
			const auto theTargetCoords = pBullet->Location + PhobosTrajectory::Vector2Coord(this->MovingVelocity);
			const auto cellHeight = MapClass::Instance->GetCellFloorHeight(theTargetCoords);

			if (cellHeight < theTargetCoords.Z)
				return;

			const auto newRatio = std::abs((pBullet->Location.Z - cellHeight) / this->MovingVelocity.Z);

			if (ratio > newRatio)
				ratio = newRatio;
		}
	}
	else
	{
		// When in high speed, it's necessary to check each cell on the path that the next frame will pass through
		double locationDistance = 0.0;
		bool velocityCheck = false;
		const bool subjectToGround = this->GetCanHitGround();
		const bool subjectToWalls = pBullet->Type->SubjectToWalls;

		const auto& theSourceCoords = pBullet->Location;
		const auto theTargetCoords = theSourceCoords + PhobosTrajectory::Vector2Coord(this->MovingVelocity);
		const auto pFirer = pBullet->Owner;
		const auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;

		if (checkThrough || subjectToGround || subjectToWalls)
		{
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
					|| (checkThrough && this->CheckThroughAndSubjectInCell(pCurCell, pOwner))) // Blocked by obstacles?
				{
					locationDistance = PhobosTrajectory::Get2DDistance(curCoord, theSourceCoords);
					velocityCheck = true;
					break;
				}

				curCoord += stepCoord;
				pCurCell = MapClass::Instance->GetCellAt(curCoord);
			}
		}

		if (!pBullet->Type->IgnoresFirestorm)
		{
			const auto fireStormCoords = MapClass::Instance->FindFirstFirestorm(theSourceCoords, theTargetCoords, pOwner);

			if (fireStormCoords != CoordStruct::Empty)
			{
				const auto distance = PhobosTrajectory::Get2DDistance(fireStormCoords, theSourceCoords);

				if (!velocityCheck || distance < locationDistance)
					locationDistance = distance;

				velocityCheck = true;
			}
		}
		// Check if the bullet needs to slow down the speed
		if (velocityCheck)
		{
			// Let the distance slightly exceed
			locationDistance += 32.0;
			// It may not be necessary to compare them again, but still do so
			if (locationDistance < velocity)
				ratio = (locationDistance / velocity);
		}
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
	// No damage, no anims...
	if (pType->PeacefulVanish.Get(this->Flag() == TrajectoryFlag::Engrave || pType->ProximityImpact || pType->DisperseCycle))
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
	const auto& source = pBullet->SourceCoords;
	const auto& target = pBullet->TargetCoords;

	// There may be a frame that hasn't started updating yet but will be drawn on the screen
	if (this->MovingVelocity != BulletVelocity::Empty)
		pBullet->Velocity = this->MovingVelocity;
	else // Lead time bug
		pBullet->Velocity = BulletVelocity { static_cast<double>(target.X - source.X), static_cast<double>(target.Y - source.Y), 0 };

	const auto pType = this->GetType();

	if (pType->BulletSpin || pType->BulletOnPlane)
		pBullet->Velocity.Z = 0;
	// this->ChangeBulletFacing();
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

/*!
	Rotate one vector by a certain angle towards the direction of another vector.

	\param from Vector that needs to be rotated. It doesn't need to be a standardized vector.
	\param to The final direction vector that needs to be oriented. It doesn't need to be a standardized vector.
	\param turningRadian The maximum radius that can rotate. Note that it must be a positive number.

	\returns The calculation result of the new vector.

	\author CrimRecya
*/
BulletVelocity PhobosTrajectory::RotateVector(const BulletVelocity& from, const BulletVelocity& to, double turningRadian)
{
	// Try using the vector to calculate the included angle
	const auto dotProduct = (to * from);
	const auto baseFactor = sqrt(to.MagnitudeSquared() * from.MagnitudeSquared());
	// Not valid vector
	if (baseFactor <= 1e-10)
		return to;
	// Calculate the cosine of the angle when the conditions are suitable
	const auto cosTheta = dotProduct / baseFactor;
	// Ensure that the result range of cos is correct
	const auto radian = Math::acos(Math::clamp(cosTheta, -1.0, 1.0));
	// When the angle is small, aim directly at the target
	if (std::abs(radian) <= turningRadian)
		return to;
	// Calculate the rotation axis
	auto rotationAxis = to.CrossProduct(from);
	// The radian can rotate, input the correct direction
	const auto rotateRadian = (radian < 0 ? turningRadian : -turningRadian);
	// Substitute to calculate new velocity
	return PhobosTrajectory::RotateAboutTheAxis(from, rotationAxis, rotateRadian);
}

/*!
	Rotate the vector around the axis of rotation by a fixed angle.

	\param vector Vector that needs to be rotated. It doesn't need to be a standardized vector.
	\param axis The vector of rotation axis. This operation will standardize it.
	\param radian The angle of rotation, positive or negative determines its direction of rotation.

	\returns The calculation result of the new vector.

	\author CrimRecya
*/
BulletVelocity PhobosTrajectory::RotateAboutTheAxis(const BulletVelocity& vector, BulletVelocity& axis, double radian)
{
	const auto axisLengthSquared = axis.MagnitudeSquared();
	// Zero axis vector is not acceptable
	if (axisLengthSquared < 1e-10)
		return vector;
	// Rotate around the axis of rotation
	axis *= 1 / sqrt(axisLengthSquared);
	const auto cosRotate = Math::cos(radian);
	// Substitute the formula to calculate the new vector
	return ((vector * cosRotate) + (axis * ((1 - cosRotate) * (vector * axis))) + (axis.CrossProduct(vector) * Math::sin(radian)));
}

void PhobosTrajectory::ChangeBulletFacing()
{
	const auto pType = this->GetType();

	if (pType->BulletStable)
		return;

	const auto pBullet = this->Bullet;
	constexpr double ratio = Math::TwoPi / 256;

	if (pType->BulletSpin)
	{
		const auto radian = Math::atan2(pBullet->Velocity.Y, pBullet->Velocity.X) + (pType->BulletROT * ratio);
		pBullet->Velocity.X = Math::cos(radian);
		pBullet->Velocity.Y = Math::sin(radian);
		pBullet->Velocity.Z = 0;
	}
	else if (pType->BulletROT < 0)
	{
		pBullet->Velocity = this->MovingVelocity;
		pBullet->Velocity *= (1 / pBullet->Velocity.Magnitude());
	}
	else
	{
		auto desiredFacing = PhobosTrajectory::Coord2Vector(pBullet->TargetCoords - pBullet->Location);

		if (pType->BulletOnPlane)
		{
			pBullet->Velocity.Z = 0;
			desiredFacing.Z = 0;
		}

		if (pType->BulletROT)
			pBullet->Velocity = PhobosTrajectory::RotateVector(pBullet->Velocity, desiredFacing, (pType->BulletROT * ratio));
		else
			pBullet->Velocity = desiredFacing;

		pBullet->Velocity *= (1 / pBullet->Velocity.Magnitude());
	}
}

bool PhobosTrajectory::CheckTolerantAndSynchronize()
{
	const auto pBullet = this->Bullet;
	const auto pType = this->GetType();
	auto pFirer = pBullet->Owner;

	if (!pBullet->Target && !pType->TolerantTime)
		return true;

	if (pFirer)
	{
		for (auto pTrans = pFirer->Transporter; pTrans; pTrans = pTrans->Transporter)
			pFirer = pTrans;
	}

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

// =============================
// load / save

void PhobosTrajectoryType::Read(CCINIClass* const pINI, const char* pSection)
{
	INI_EX exINI(pINI);

	this->Speed.Read(exINI, pSection, "Trajectory.Speed");
	this->Speed = Math::max(0.001, this->Speed);
	this->Duration.Read(exINI, pSection, "Trajectory.Duration");
	this->TolerantTime.Read(exINI, pSection, "Trajectory.TolerantTime");
	this->BulletROT.Read(exINI, pSection, "Trajectory.BulletROT");
	this->BulletSpin.Read(exINI, pSection, "Trajectory.BulletSpin");
	this->BulletStable.Read(exINI, pSection, "Trajectory.BulletStable");
	this->BulletOnPlane.Read(exINI, pSection, "Trajectory.BulletOnPlane");
	this->MirrorCoord.Read(exINI, pSection, "Trajectory.MirrorCoord");
	this->RetargetRadius.Read(exINI, pSection, "Trajectory.RetargetRadius");
	this->Synchronize.Read(exINI, pSection, "Trajectory.Synchronize");
	this->PeacefulVanish.Read(exINI, pSection, "Trajectory.PeacefulVanish");
	this->ApplyRangeModifiers.Read(exINI, pSection, "Trajectory.ApplyRangeModifiers");
	this->UseDisperseCoord.Read(exINI, pSection, "Trajectory.UseDisperseCoord");
	this->RecordSourceCoord.Read(exINI, pSection, "Trajectory.RecordSourceCoord");

	this->PassDetonate.Read(exINI, pSection, "Trajectory.PassDetonate");
	this->PassDetonateWarhead.Read<true>(exINI, pSection, "Trajectory.PassDetonateWarhead");
	this->PassDetonateDamage.Read(exINI, pSection, "Trajectory.PassDetonateDamage");
	this->PassDetonateDelay.Read(exINI, pSection, "Trajectory.PassDetonateDelay");
	this->PassDetonateDelay = Math::max(1, this->PassDetonateDelay);
	this->PassDetonateInitialDelay.Read(exINI, pSection, "Trajectory.PassDetonateInitialDelay");
	this->PassDetonateInitialDelay = Math::max(0, this->PassDetonateInitialDelay);
	this->PassDetonateLocal.Read(exINI, pSection, "Trajectory.PassDetonateLocal");
	this->ProximityImpact.Read(exINI, pSection, "Trajectory.ProximityImpact");
	this->ProximityWarhead.Read<true>(exINI, pSection, "Trajectory.ProximityWarhead");
	this->ProximityDamage.Read(exINI, pSection, "Trajectory.ProximityDamage");
	this->ProximityRadius.Read(exINI, pSection, "Trajectory.ProximityRadius");
	this->ProximityDirect.Read(exINI, pSection, "Trajectory.ProximityDirect");
	this->ProximityMedial.Read(exINI, pSection, "Trajectory.ProximityMedial");
	this->ProximityAllies.Read(exINI, pSection, "Trajectory.ProximityAllies");
	this->ProximityFlight.Read(exINI, pSection, "Trajectory.ProximityFlight");
	this->ThroughVehicles.Read(exINI, pSection, "Trajectory.ThroughVehicles");
	this->ThroughBuilding.Read(exINI, pSection, "Trajectory.ThroughBuilding");
	this->DamageEdgeAttenuation.Read(exINI, pSection, "Trajectory.DamageEdgeAttenuation");
	this->DamageEdgeAttenuation = Math::max(0.0, this->DamageEdgeAttenuation);
	this->DamageCountAttenuation.Read(exINI, pSection, "Trajectory.DamageCountAttenuation");
	this->DamageCountAttenuation = Math::max(0.0, this->DamageCountAttenuation);

	this->DisperseWeapons.Read(exINI, pSection, "Trajectory.DisperseWeapons");
	this->DisperseBursts.Read(exINI, pSection, "Trajectory.DisperseBursts");
	this->DisperseCounts.Read(exINI, pSection, "Trajectory.DisperseCounts");
	this->DisperseDelays.Read(exINI, pSection, "Trajectory.DisperseDelays");
	this->DisperseCycle.Read(exINI, pSection, "Trajectory.DisperseCycle");
	this->DisperseInitialDelay.Read(exINI, pSection, "Trajectory.DisperseInitialDelay");
	this->DisperseEffectiveRange.Read(exINI, pSection, "Trajectory.DisperseEffectiveRange");
	this->DisperseSeparate.Read(exINI, pSection, "Trajectory.DisperseSeparate");
	this->DisperseRetarget.Read(exINI, pSection, "Trajectory.DisperseRetarget");
	this->DisperseLocation.Read(exINI, pSection, "Trajectory.DisperseLocation");
	this->DisperseTendency.Read(exINI, pSection, "Trajectory.DisperseTendency");
	this->DisperseHolistic.Read(exINI, pSection, "Trajectory.DisperseHolistic");
	this->DisperseMarginal.Read(exINI, pSection, "Trajectory.DisperseMarginal");
	this->DisperseDoRepeat.Read(exINI, pSection, "Trajectory.DisperseDoRepeat");
	this->DisperseSuicide.Read(exINI, pSection, "Trajectory.DisperseSuicide");
	this->DisperseFromFirer.Read(exINI, pSection, "Trajectory.DisperseFromFirer");
	this->DisperseFaceCheck.Read(exINI, pSection, "Trajectory.DisperseFaceCheck");
	this->DisperseForceFire.Read(exINI, pSection, "Trajectory.DisperseForceFire");
	this->DisperseCoord.Read(exINI, pSection, "Trajectory.DisperseCoord");
}

bool PhobosTrajectoryType::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->Serialize(Stm);
	return true;
}

bool PhobosTrajectoryType::Save(PhobosStreamWriter& Stm) const
{
	const_cast<PhobosTrajectoryType*>(this)->Serialize(Stm);
	return true;
}

template<typename T>
void PhobosTrajectoryType::Serialize(T& Stm)
{
	Stm
		.Process(this->Speed)
		.Process(this->Duration)
		.Process(this->TolerantTime)
		.Process(this->BulletROT)
		.Process(this->BulletSpin)
		.Process(this->BulletStable)
		.Process(this->BulletOnPlane)
		.Process(this->MirrorCoord)
		.Process(this->RetargetRadius)
		.Process(this->Synchronize)
		.Process(this->PeacefulVanish)
		.Process(this->ApplyRangeModifiers)
		.Process(this->UseDisperseCoord)
		.Process(this->RecordSourceCoord)

		.Process(this->PassDetonate)
		.Process(this->PassDetonateWarhead)
		.Process(this->PassDetonateDamage)
		.Process(this->PassDetonateDelay)
		.Process(this->PassDetonateInitialDelay)
		.Process(this->PassDetonateLocal)
		.Process(this->ProximityImpact)
		.Process(this->ProximityWarhead)
		.Process(this->ProximityDamage)
		.Process(this->ProximityRadius)
		.Process(this->ProximityDirect)
		.Process(this->ProximityMedial)
		.Process(this->ProximityAllies)
		.Process(this->ProximityFlight)
		.Process(this->ThroughVehicles)
		.Process(this->ThroughBuilding)
		.Process(this->DamageEdgeAttenuation)
		.Process(this->DamageCountAttenuation)

		.Process(this->DisperseWeapons)
		.Process(this->DisperseBursts)
		.Process(this->DisperseCounts)
		.Process(this->DisperseDelays)
		.Process(this->DisperseCycle)
		.Process(this->DisperseInitialDelay)
		.Process(this->DisperseEffectiveRange)
		.Process(this->DisperseSeparate)
		.Process(this->DisperseRetarget)
		.Process(this->DisperseLocation)
		.Process(this->DisperseTendency)
		.Process(this->DisperseHolistic)
		.Process(this->DisperseMarginal)
		.Process(this->DisperseDoRepeat)
		.Process(this->DisperseSuicide)
		.Process(this->DisperseFromFirer)
		.Process(this->DisperseFaceCheck)
		.Process(this->DisperseForceFire)
		.Process(this->DisperseCoord)
		;
}

bool PhobosTrajectory::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->Serialize(Stm);
	return true;
}

bool PhobosTrajectory::Save(PhobosStreamWriter& Stm) const
{
	const_cast<PhobosTrajectory*>(this)->Serialize(Stm);
	return true;
}

template<typename T>
void PhobosTrajectory::Serialize(T& Stm)
{
	Stm
		.Process(this->Bullet)
		.Process(this->MovingVelocity)
		.Process(this->MovingSpeed)
		.Process(this->DurationTimer)
		.Process(this->TolerantTimer)
		.Process(this->FirepowerMult)
		.Process(this->AttenuationRange)
		.Process(this->RemainingDistance)
		.Process(this->TargetInTheAir)
		.Process(this->TargetIsTechno)
		.Process(this->NotMainWeapon)
		.Process(this->ShouldDetonate)
		.Process(this->FLHCoord)
		.Process(this->BuildingCoord)
		.Process(this->CurrentBurst)
		.Process(this->CountOfBurst)

		.Process(this->PassDetonateDamage)
		.Process(this->PassDetonateTimer)
		.Process(this->ProximityImpact)
		.Process(this->ProximityDamage)
		.Process(this->ExtraCheck)
		.Process(this->TheCasualty)

		.Process(this->DisperseIndex)
		.Process(this->DisperseCount)
		.Process(this->DisperseCycle)
		.Process(this->DisperseTimer)
		;
}

// =============================
// hooks

DEFINE_HOOK(0x4666F7, BulletClass_AI_Trajectories, 0x6)
{
	enum { Detonate = 0x467E53 };

	GET(BulletClass*, pThis, EBP);

	auto const pExt = BulletExt::ExtMap.Find(pThis);
	bool detonate = false;

	if (auto pTraj = pExt->Trajectory.get())
		detonate = pTraj->OnAI();

	if (detonate && !pThis->SpawnNextAnim)
		return Detonate;

	return 0;
}

DEFINE_HOOK(0x467E53, BulletClass_AI_PreDetonation_Trajectories, 0x6)
{
	GET(BulletClass*, pThis, EBP);

	auto const pExt = BulletExt::ExtMap.Find(pThis);

	if (auto pTraj = pExt->Trajectory.get())
		pTraj->OnAIPreDetonate();

	return 0;
}

DEFINE_HOOK(0x46745C, BulletClass_AI_Position_Trajectories, 0x7)
{
	GET(BulletClass*, pThis, EBP);
	LEA_STACK(BulletVelocity*, pSpeed, STACK_OFFSET(0x1AC, -0x11C));
	LEA_STACK(BulletVelocity*, pPosition, STACK_OFFSET(0x1AC, -0x144));

	auto const pExt = BulletExt::ExtMap.Find(pThis);

	if (auto pTraj = pExt->Trajectory.get())
		pTraj->OnAIVelocity(pSpeed, pPosition);

	// Trajectory can use Velocity only for turning Image's direction
	// The true position in the next frame will be calculate after here
	if (pExt->Trajectory && pExt->LaserTrails.size())
	{
		CoordStruct futureCoords
		{
			static_cast<int>(pSpeed->X + pPosition->X),
			static_cast<int>(pSpeed->Y + pPosition->Y),
			static_cast<int>(pSpeed->Z + pPosition->Z)
		};

		for (auto& trail : pExt->LaserTrails)
		{
			if (!trail.LastLocation.isset())
				trail.LastLocation = pThis->Location;

			trail.Update(futureCoords);
		}
	}

	return 0;
}

DEFINE_HOOK(0x4677D3, BulletClass_AI_TargetCoordCheck_Trajectories, 0x5)
{
	enum { SkipCheck = 0x4678F8, ContinueAfterCheck = 0x467879, Detonate = 0x467E53 };

	GET(BulletClass*, pThis, EBP);

	auto const pExt = BulletExt::ExtMap.Find(pThis);

	if (auto pTraj = pExt->Trajectory.get())
	{
		switch (pTraj->OnAITargetCoordCheck())
		{
		case TrajectoryCheckReturnType::SkipGameCheck:
			return SkipCheck;
			break;
		case TrajectoryCheckReturnType::SatisfyGameCheck:
			return ContinueAfterCheck;
			break;
		case TrajectoryCheckReturnType::Detonate:
			return Detonate;
			break;
		default:
			break;
		}
	}

	return 0;
}

DEFINE_HOOK(0x467927, BulletClass_AI_TechnoCheck_Trajectories, 0x5)
{
	enum { SkipCheck = 0x467A26, ContinueAfterCheck = 0x467514 };

	GET(BulletClass*, pThis, EBP);
	GET(TechnoClass*, pTechno, ESI);

	auto const pExt = BulletExt::ExtMap.Find(pThis);

	if (auto pTraj = pExt->Trajectory.get())
	{
		switch (pTraj->OnAITechnoCheck(pTechno))
		{
		case TrajectoryCheckReturnType::SkipGameCheck:
			return SkipCheck;
			break;
		case TrajectoryCheckReturnType::SatisfyGameCheck:
			return ContinueAfterCheck;
			break;
		default:
			break;
		}
	}

	return 0;
}

DEFINE_HOOK(0x468B72, BulletClass_Unlimbo_Trajectories, 0x5)
{
	GET(BulletClass*, pThis, EBX);

	auto const pExt = BulletExt::ExtMap.Find(pThis);
	auto const pTypeExt = pExt->TypeExtData;

	if (pTypeExt && pTypeExt->TrajectoryType)
	{
		pExt->Trajectory = pTypeExt->TrajectoryType->CreateInstance(pThis);
		pExt->Trajectory->OnUnlimbo();
	}

	return 0;
}
