#include "StraightTrajectory.h"

#include <OverlayTypeClass.h>
#include <ScenarioClass.h>
#include <AircraftTrackerClass.h>

#include <Ext/Bullet/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/Techno/Body.h>

std::unique_ptr<PhobosTrajectory> StraightTrajectoryType::CreateInstance() const
{
	return std::make_unique<StraightTrajectory>(this);
}

template<typename T>
void StraightTrajectoryType::Serialize(T& Stm)
{
	Stm
		.Process(this->PassThrough)
		.Process(this->ConfineAtHeight)
		;
}

bool StraightTrajectoryType::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->LiveShellTrajectoryType::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool StraightTrajectoryType::Save(PhobosStreamWriter& Stm) const
{
	this->LiveShellTrajectoryType::Save(Stm);
	const_cast<StraightTrajectoryType*>(this)->Serialize(Stm);
	return true;
}

void StraightTrajectoryType::Read(CCINIClass* const pINI, const char* pSection)
{
	this->PhobosTrajectoryType::Read(pINI, pSection);
	INI_EX exINI(pINI);

	// LiveShell
	this->RotateCoord.Read(exINI, pSection, "Trajectory.Straight.RotateCoord");
	this->OffsetCoord.Read(exINI, pSection, "Trajectory.Straight.OffsetCoord");
	this->AxisOfRotation.Read(exINI, pSection, "Trajectory.Straight.AxisOfRotation");
	this->LeadTimeCalculate.Read(exINI, pSection, "Trajectory.Straight.LeadTimeCalculate");
	this->DetonationDistance.Read(exINI, pSection, "Trajectory.Straight.DetonationDistance");
	this->TargetSnapDistance.Read(exINI, pSection, "Trajectory.Straight.TargetSnapDistance");

	// Straight
	this->PassThrough.Read(exINI, pSection, "Trajectory.Straight.PassThrough");
	this->ConfineAtHeight.Read(exINI, pSection, "Trajectory.Straight.ConfineAtHeight");
}

template<typename T>
void StraightTrajectory::Serialize(T& Stm)
{
	Stm
		.Process(this->Type)
		.Process(this->DetonationDistance)
		;
}

bool StraightTrajectory::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->LiveShellTrajectory::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool StraightTrajectory::Save(PhobosStreamWriter& Stm) const
{
	this->LiveShellTrajectory::Save(Stm);
	const_cast<StraightTrajectory*>(this)->Serialize(Stm);
	return true;
}

void StraightTrajectory::OnUnlimbo(BulletClass* pBullet)
{
	this->LiveShellTrajectory::OnUnlimbo(pBullet);

	if (this->Type->ApplyRangeModifiers)
	{
		if (const auto pFirer = pBullet->Owner)
		{
			if (const auto pWeapon = pBullet->WeaponType)
			{
				// Determine the range of the bullet
				if (this->DetonationDistance >= 0)
					this->DetonationDistance = Leptons(WeaponTypeExt::GetRangeWithModifiers(pWeapon, pFirer, this->DetonationDistance));
				else
					this->DetonationDistance = Leptons(-WeaponTypeExt::GetRangeWithModifiers(pWeapon, pFirer, -this->DetonationDistance));
			}
		}
	}

	if (!BulletExt::ExtMap.Find(pBullet)->DispersedTrajectory)
		this->OpenFire(pBullet);
}

bool StraightTrajectory::OnAI(BulletClass* pBullet)
{
	if (this->WaitOneFrame && this->BulletPrepareCheck(pBullet))
		return false;

	const auto pFirer = pBullet->Owner;
	const auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;
	const auto pType = this->Type;

	if (this->OnAIPreCheck(pBullet, pOwner))
		return true;

	this->OnAIVelocityCheck(pBullet, pOwner);
	this->PhobosTrajectory::OnAI(pBullet);

	if (pType->Speed < 256.0 && pType->ConfineAtHeight > 0 && this->PassAndConfineAtHeight(pBullet))
		return true;

	this->OnAILastCheck(pBullet, pOwner);

	return false;
}

void StraightTrajectory::OnAIPreDetonate(BulletClass* pBullet)
{
	const auto pType = this->Type;

	// Whether to detonate at ground level?
	if (pType->PassDetonateLocal)
		pBullet->SetLocation(CoordStruct { pBullet->Location.X, pBullet->Location.Y, MapClass::Instance->GetCellFloorHeight(pBullet->Location) });

	if (!pType->PassThrough)
		this->LiveShellTrajectory::OnAIPreDetonate(pBullet);
	else
		this->PhobosTrajectory::OnAIPreDetonate(pBullet);
}

bool StraightTrajectory::OpenFire(BulletClass* pBullet)
{
	// Wait, or launch immediately?
	if (!this->Type->LeadTimeCalculate || !abstract_cast<FootClass*>(pBullet->Target))
		this->PrepareForOpenFire(pBullet);
	else
		this->WaitOneFrame = 2;
}

void StraightTrajectory::SetBulletNewTarget(BulletClass* const pBullet, AbstractClass* const pTarget)
{
	pBullet->SetTarget(pTarget);
	pBullet->TargetCoords = pTarget->GetCoords();
}

void StraightTrajectory::PrepareForOpenFire(BulletClass* pBullet)
{
	const auto pType = this->Type;
	auto theTargetCoords = pBullet->TargetCoords;
	auto theSourceCoords = pBullet->SourceCoords;

	// TODO If I could calculate this before firing, perhaps it can solve the problem of one frame delay and not so correct turret orientation.
	if (pType->LeadTimeCalculate)
	{
		if (const auto pTarget = pBullet->Target)
		{
			theTargetCoords = pTarget->GetCoords();
			theSourceCoords = pBullet->Location;

			// Solving trigonometric functions
			if (theTargetCoords != this->LastTargetCoord)
			{
				const auto extraOffsetCoord = theTargetCoords - this->LastTargetCoord;
				const auto targetSourceCoord = theSourceCoords - theTargetCoords;
				const auto lastSourceCoord = theSourceCoords - this->LastTargetCoord;

				const auto theDistanceSquared = targetSourceCoord.MagnitudeSquared();
				const auto targetSpeedSquared = extraOffsetCoord.MagnitudeSquared();
				const auto targetSpeed = sqrt(targetSpeedSquared);

				const auto crossFactor = lastSourceCoord.CrossProduct(targetSourceCoord).MagnitudeSquared();
				const auto verticalDistanceSquared = crossFactor / targetSpeedSquared;

				const auto horizonDistanceSquared = theDistanceSquared - verticalDistanceSquared;
				const auto horizonDistance = sqrt(horizonDistanceSquared);

				const auto straightSpeedSquared = pType->Speed * pType->Speed;
				const auto baseFactor = straightSpeedSquared - targetSpeedSquared;
				const auto squareFactor = baseFactor * verticalDistanceSquared + straightSpeedSquared * horizonDistanceSquared;

				// Is there a solution?
				if (squareFactor > 1e-10)
				{
					const auto minusFactor = -(horizonDistance * targetSpeed);
					int travelTime = 0;

					if (std::abs(baseFactor) < 1e-10)
					{
						travelTime = std::abs(horizonDistance) > 1e-10 ? (static_cast<int>(theDistanceSquared / (2 * horizonDistance * targetSpeed)) + 1) : 0;
					}
					else
					{
						const auto travelTimeM = static_cast<int>((minusFactor - sqrt(squareFactor)) / baseFactor);
						const auto travelTimeP = static_cast<int>((minusFactor + sqrt(squareFactor)) / baseFactor);

						if (travelTimeM > 0 && travelTimeP > 0)
							travelTime = travelTimeM < travelTimeP ? travelTimeM : travelTimeP;
						else if (travelTimeM > 0)
							travelTime = travelTimeM;
						else if (travelTimeP > 0)
							travelTime = travelTimeP;

						if (targetSourceCoord.MagnitudeSquared() < lastSourceCoord.MagnitudeSquared())
							travelTime += 1;
						else
							travelTime += 2;
					}

					theTargetCoords += extraOffsetCoord * travelTime;
				}
			}
		}
	}

	const double rotateRadian = this->Get2DOpRadian(((theTargetCoords == theSourceCoords && pBullet->Owner) ? pBullet->Owner->GetCoords() : theSourceCoords), theTargetCoords);

	// Add the fixed offset value
	if (pType->OffsetCoord != CoordStruct::Empty)
		theTargetCoords += this->GetOnlyStableOffsetCoords(rotateRadian);

	// Add random offset value
	if (pBullet->Type->Inaccurate)
		theTargetCoords = this->GetInaccurateTargetCoords(pBullet, theTargetCoords, theSourceCoords.DistanceFrom(theTargetCoords));

	// Determine the distance that the bullet can travel
	if (!pType->PassThrough)
		this->RemainingDistance += static_cast<int>(theSourceCoords.DistanceFrom(theTargetCoords) + pType->Speed);
	else if (this->DetonationDistance > 0)
		this->RemainingDistance += static_cast<int>(this->DetonationDistance + pType->Speed);
	else if (this->DetonationDistance < 0)
		this->RemainingDistance += static_cast<int>(theSourceCoords.DistanceFrom(theTargetCoords) - this->DetonationDistance + pType->Speed);
	else
		this->RemainingDistance = INT_MAX;

	// Determine the firing velocity vector of the bullet
	pBullet->TargetCoords = theTargetCoords;
	pBullet->Velocity.X = static_cast<double>(theTargetCoords.X - theSourceCoords.X);
	pBullet->Velocity.Y = static_cast<double>(theTargetCoords.Y - theSourceCoords.Y);
	pBullet->Velocity.Z = (pType->ConfineAtHeight > 0 && pType->PassDetonateLocal) ? 0 : static_cast<double>(this->GetVelocityZ(pBullet));

	// Rotate the selected angle
	if (std::abs(pType->RotateCoord) > 1e-10 && this->CountOfBurst > 1)
		this->DisperseBurstSubstitution(pBullet, rotateRadian);

	// Substitute the speed to calculate velocity
	if (this->CalculateBulletVelocity(pBullet))
		this->RemainingDistance = 0;
}

int StraightTrajectory::GetVelocityZ(BulletClass* pBullet)
{
	const auto pType = this->Type;
	auto sourceCellZ = pBullet->SourceCoords.Z;
	auto targetCellZ = pBullet->TargetCoords.Z;
	auto bulletVelocityZ = static_cast<int>(targetCellZ - sourceCellZ);

	// Subtract directly if no need to pass through the target
	if (!pType->PassThrough)
		return bulletVelocityZ;

	if (const auto pTechno = pBullet->Owner)
	{
		const auto pCell = pTechno->GetCell();
		sourceCellZ = pCell->Level * Unsorted::LevelHeight;

		if (pCell->ContainsBridge() && pTechno->OnBridge)
			sourceCellZ += CellClass::BridgeHeight;
	}

	if (const auto pTarget = abstract_cast<ObjectClass*>(pBullet->Target))
	{
		const auto pCell = pTarget->GetCell();
		targetCellZ = pCell->Level * Unsorted::LevelHeight;

		if (pCell->ContainsBridge() && pTarget->OnBridge)
			targetCellZ += CellClass::BridgeHeight;
	}

	// If both are at the same height, use the DetonationDistance to calculate which position behind the target needs to be aimed (32 -> error range)
	if (sourceCellZ == targetCellZ || std::abs(bulletVelocityZ) <= 32)
	{
		// Infinite distance, horizontal emission
		if (!this->DetonationDistance)
			return 0;

		const auto distanceOfTwo = PhobosTrajectory::Get2DDistance(pBullet->SourceCoords, pBullet->TargetCoords);
		const auto theDistance = (this->DetonationDistance < 0) ? (distanceOfTwo - this->DetonationDistance) : this->DetonationDistance;

		// Calculate the ratio for subsequent speed calculation
		if (std::abs(theDistance) < 1e-10)
			return 0;

		bulletVelocityZ = static_cast<int>(bulletVelocityZ * (distanceOfTwo / theDistance));
	}

	return bulletVelocityZ;
}

bool StraightTrajectory::BulletPrepareCheck(BulletClass* pBullet)
{
	// The time between bullets' Unlimbo() and Update() is completely uncertain.
	// Target will update location after techno firing, which may result in inaccurate
	// target position recorded by the LastTargetCoord in Unlimbo(). Therefore, it's
	// necessary to record the position during the first Update(). - CrimRecya
	if (this->WaitOneFrame == 2)
	{
		if (const auto pTarget = pBullet->Target)
		{
			this->LastTargetCoord = pTarget->GetCoords();
			this->WaitOneFrame = 1;
			return true;
		}
	}

	this->WaitOneFrame = 0;
	this->PrepareForOpenFire(pBullet);

	return false;
}

bool StraightTrajectory::PassAndConfineAtHeight(BulletClass* pBullet)
{
	const CoordStruct futureCoords
	{
		pBullet->Location.X + static_cast<int>(pBullet->Velocity.X),
		pBullet->Location.Y + static_cast<int>(pBullet->Velocity.Y),
		pBullet->Location.Z + static_cast<int>(pBullet->Velocity.Z)
	};

	auto checkDifference = MapClass::Instance->GetCellFloorHeight(futureCoords) - futureCoords.Z;

	if (MapClass::Instance->GetCellAt(futureCoords)->ContainsBridge())
	{
		const auto differenceOnBridge = checkDifference + CellClass::BridgeHeight;

		if (std::abs(differenceOnBridge) < std::abs(checkDifference))
			checkDifference = differenceOnBridge;
	}

	// The height does not exceed the cliff, or the cliff can be ignored? (384 -> (4 * Unsorted::LevelHeight - 32(error range)))
	if (std::abs(checkDifference) < 384 || !pBullet->Type->SubjectToCliffs)
	{
		const auto pType = this->Type;
		pBullet->Velocity.Z += static_cast<double>(checkDifference + pType->ConfineAtHeight);

		if (!pType->PassDetonateLocal && this->CalculateBulletVelocity(pBullet))
			return true;
	}
	else
	{
		return true;
	}

	return false;
}
