#include "PhobosTrajectory.h"

#include <AircraftTrackerClass.h>

#include <Ext/WarheadType/Body.h>

void PhobosTrajectory::OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition)
{
	*pSpeed = this->MovingVelocity; // This is for location calculation, and pBullet->Velocity is for image drawing
}

inline double PhobosTrajectory::Get2DDistance(const CoordStruct& here, const CoordStruct& there)
{
	return Point2D { here.X, here.Y }.DistanceFrom(Point2D { there.X, there.Y });
}

inline double PhobosTrajectory::Get2DVelocity(const BulletVelocity& velocity)
{
	return Vector2D<double>{ velocity.X, velocity.Y }.Magnitude();
}

inline void PhobosTrajectory::SetNewDamage(int& damage, double ratio)
{
	if (damage)
	{
		if (const auto newDamage = static_cast<int>(damage * ratio))
			damage = newDamage;
		else
			damage = Math::sgn(damage);
	}
}

inline bool PhobosTrajectory::CheckTechnoIsInvalid(TechnoClass* pTechno)
{
	// The target is alive
	return (!pTechno->IsAlive || !pTechno->IsOnMap || pTechno->InLimbo || pTechno->IsSinking || pTechno->Health <= 0);
}

inline bool PhobosTrajectory::CheckWeaponCanTarget(WeaponTypeExt::ExtData* pWeaponExt, TechnoClass* pFirer, TechnoClass* pTarget)
{
	// No check for CanTargetHouses
	return !pWeaponExt || (EnumFunctions::IsTechnoEligible(pTarget, pWeaponExt->CanTarget) && pWeaponExt->HasRequiredAttachedEffects(pTarget, pFirer));
}

inline bool PhobosTrajectory::CheckWeaponValidness(HouseClass* pHouse, TechnoClass* pTechno, CellClass* pCell, AffectedHouse flags)
{
	if (pHouse == pTechno->Owner)
		return (flags & AffectedHouse::Owner) != AffectedHouse::None;
	else if (pHouse->IsAlliedWith(pTechno->Owner) || pTechno->IsDisguisedAs(pHouse))
		return (flags & AffectedHouse::Allies) != AffectedHouse::None;
	else if ((flags & AffectedHouse::Enemies) == AffectedHouse::None)
		return false;

	return pTechno->CloakState != CloakState::Cloaked || pCell->Sensors_InclHouse(pHouse->ArrayIndex);
}

// A rectangular shape with a custom width from the current frame to the next frame in length.
std::vector<CellClass*> PhobosTrajectory::GetCellsInProximityRadius(const BulletClass* const pBullet, const Leptons trajectoryProximityRange)
{
	// Seems like the y-axis is reversed, but it's okay.
	const CoordStruct walkCoord { static_cast<int>(pBullet->Velocity.X), static_cast<int>(pBullet->Velocity.Y), 0 };
	const auto sideMult = trajectoryProximityRange / walkCoord.Magnitude();

	const CoordStruct cor1Coord { static_cast<int>(walkCoord.Y * sideMult), static_cast<int>((-walkCoord.X) * sideMult), 0 };
	const CoordStruct cor4Coord { static_cast<int>((-walkCoord.Y) * sideMult), static_cast<int>(walkCoord.X * sideMult), 0 };
	const auto thisCell = CellClass::Coord2Cell(pBullet->Location);

	auto cor1Cell = CellClass::Coord2Cell((pBullet->Location + cor1Coord));
	auto cor4Cell = CellClass::Coord2Cell((pBullet->Location + cor4Coord));

	const auto off1Cell = cor1Cell - thisCell;
	const auto off4Cell = cor4Cell - thisCell;
	const auto nextCell = CellClass::Coord2Cell((pBullet->Location + walkCoord));

	auto cor2Cell = nextCell + off1Cell;
	auto cor3Cell = nextCell + off4Cell;

	// Arrange the vertices of the rectangle in order from bottom to top.
	int cornerIndex = 0;
	CellStruct corner[4] = { cor1Cell, cor2Cell, cor3Cell, cor4Cell };

	for (int i = 1; i < 4; ++i)
	{
		if (corner[cornerIndex].Y > corner[i].Y)
			cornerIndex = i;
	}

	cor1Cell = corner[cornerIndex];
	++cornerIndex %= 4;
	cor2Cell = corner[cornerIndex];
	++cornerIndex %= 4;
	cor3Cell = corner[cornerIndex];
	++cornerIndex %= 4;
	cor4Cell = corner[cornerIndex];

	std::vector<CellStruct> recCells = PhobosTrajectory::GetCellsInRectangle(cor1Cell, cor4Cell, cor2Cell, cor3Cell);
	std::vector<CellClass*> recCellClass;
	recCellClass.reserve(recCells.size());

	for (const auto& pCells : recCells)
	{
		if (CellClass* pRecCell = MapClass::Instance->TryGetCellAt(pCells))
			recCellClass.push_back(pRecCell);
	}

	return recCellClass;
}

// Can ONLY fill RECTANGLE. Record cells in the order of "draw left boundary, draw right boundary, fill middle, and move up one level".
std::vector<CellStruct> PhobosTrajectory::GetCellsInRectangle(const CellStruct bottomStaCell, const CellStruct leftMidCell, const CellStruct rightMidCell, const CellStruct topEndCell)
{
	std::vector<CellStruct> recCells;
	const auto cellNums = (std::abs(topEndCell.Y - bottomStaCell.Y) + 1) * (std::abs(rightMidCell.X - leftMidCell.X) + 1);
	recCells.reserve(cellNums);
	recCells.push_back(bottomStaCell);

	if (bottomStaCell == leftMidCell || bottomStaCell == rightMidCell) // A straight line
	{
		auto middleCurCell = bottomStaCell;

		const auto middleTheDist = topEndCell - bottomStaCell;
		const CellStruct middleTheUnit { static_cast<short>(Math::sgn(middleTheDist.X)), static_cast<short>(Math::sgn(middleTheDist.Y)) };
		const CellStruct middleThePace { static_cast<short>(middleTheDist.X * middleTheUnit.X), static_cast<short>(middleTheDist.Y * middleTheUnit.Y) };
		auto mTheCurN = static_cast<float>((middleThePace.Y - middleThePace.X) / 2.0);

		while (middleCurCell != topEndCell)
		{
			if (mTheCurN > 0)
			{
				mTheCurN -= middleThePace.X;
				middleCurCell.Y += middleTheUnit.Y;
				recCells.push_back(middleCurCell);
			}
			else if (mTheCurN < 0)
			{
				mTheCurN += middleThePace.Y;
				middleCurCell.X += middleTheUnit.X;
				recCells.push_back(middleCurCell);
			}
			else
			{
				mTheCurN += middleThePace.Y - middleThePace.X;
				middleCurCell.X += middleTheUnit.X;
				recCells.push_back(middleCurCell);
				middleCurCell.X -= middleTheUnit.X;
				middleCurCell.Y += middleTheUnit.Y;
				recCells.push_back(middleCurCell);
				middleCurCell.X += middleTheUnit.X;
				recCells.push_back(middleCurCell);
			}
		}
	}
	else // Complete rectangle
	{
		auto leftCurCell = bottomStaCell;
		auto rightCurCell = bottomStaCell;
		auto middleCurCell = bottomStaCell;

		bool leftNext = false;
		bool rightNext = false;
		bool leftSkip = false;
		bool rightSkip = false;
		bool leftContinue = false;
		bool rightContinue = false;

		const auto left1stDist = leftMidCell - bottomStaCell;
		const CellStruct left1stUnit { static_cast<short>(Math::sgn(left1stDist.X)), static_cast<short>(Math::sgn(left1stDist.Y)) };
		const CellStruct left1stPace { static_cast<short>(left1stDist.X * left1stUnit.X), static_cast<short>(left1stDist.Y * left1stUnit.Y) };
		auto left1stCurN = static_cast<float>((left1stPace.Y - left1stPace.X) / 2.0);

		const auto left2ndDist = topEndCell - leftMidCell;
		const CellStruct left2ndUnit { static_cast<short>(Math::sgn(left2ndDist.X)), static_cast<short>(Math::sgn(left2ndDist.Y)) };
		const CellStruct left2ndPace { static_cast<short>(left2ndDist.X * left2ndUnit.X), static_cast<short>(left2ndDist.Y * left2ndUnit.Y) };
		auto left2ndCurN = static_cast<float>((left2ndPace.Y - left2ndPace.X) / 2.0);

		const auto right1stDist = rightMidCell - bottomStaCell;
		const CellStruct right1stUnit { static_cast<short>(Math::sgn(right1stDist.X)), static_cast<short>(Math::sgn(right1stDist.Y)) };
		const CellStruct right1stPace { static_cast<short>(right1stDist.X * right1stUnit.X), static_cast<short>(right1stDist.Y * right1stUnit.Y) };
		auto right1stCurN = static_cast<float>((right1stPace.Y - right1stPace.X) / 2.0);

		const auto right2ndDist = topEndCell - rightMidCell;
		const CellStruct right2ndUnit { static_cast<short>(Math::sgn(right2ndDist.X)), static_cast<short>(Math::sgn(right2ndDist.Y)) };
		const CellStruct right2ndPace { static_cast<short>(right2ndDist.X * right2ndUnit.X), static_cast<short>(right2ndDist.Y * right2ndUnit.Y) };
		auto right2ndCurN = static_cast<float>((right2ndPace.Y - right2ndPace.X) / 2.0);

		while (leftCurCell != topEndCell || rightCurCell != topEndCell)
		{
			while (leftCurCell != topEndCell) // Left
			{
				if (!leftNext) // Bottom Left Side
				{
					if (left1stCurN > 0)
					{
						left1stCurN -= left1stPace.X;
						leftCurCell.Y += left1stUnit.Y;

						if (leftCurCell == leftMidCell)
						{
							leftNext = true;
						}
						else
						{
							recCells.push_back(leftCurCell);
							break;
						}
					}
					else
					{
						left1stCurN += left1stPace.Y;
						leftCurCell.X += left1stUnit.X;

						if (leftCurCell == leftMidCell)
						{
							leftNext = true;
							leftSkip = true;
						}
					}
				}
				else // Top Left Side
				{
					if (left2ndCurN >= 0)
					{
						if (leftSkip)
						{
							leftSkip = false;
							left2ndCurN -= left2ndPace.X;
							leftCurCell.Y += left2ndUnit.Y;
						}
						else
						{
							leftContinue = true;
							break;
						}
					}
					else
					{
						left2ndCurN += left2ndPace.Y;
						leftCurCell.X += left2ndUnit.X;
					}
				}

				if (leftCurCell != rightCurCell) // Avoid double counting cells.
					recCells.push_back(leftCurCell);
			}

			while (rightCurCell != topEndCell) // Right
			{
				if (!rightNext) // Bottom Right Side
				{
					if (right1stCurN > 0)
					{
						right1stCurN -= right1stPace.X;
						rightCurCell.Y += right1stUnit.Y;

						if (rightCurCell == rightMidCell)
						{
							rightNext = true;
						}
						else
						{
							recCells.push_back(rightCurCell);
							break;
						}
					}
					else
					{
						right1stCurN += right1stPace.Y;
						rightCurCell.X += right1stUnit.X;

						if (rightCurCell == rightMidCell)
						{
							rightNext = true;
							rightSkip = true;
						}
					}
				}
				else // Top Right Side
				{
					if (right2ndCurN >= 0)
					{
						if (rightSkip)
						{
							rightSkip = false;
							right2ndCurN -= right2ndPace.X;
							rightCurCell.Y += right2ndUnit.Y;
						}
						else
						{
							rightContinue = true;
							break;
						}
					}
					else
					{
						right2ndCurN += right2ndPace.Y;
						rightCurCell.X += right2ndUnit.X;
					}
				}

				if (rightCurCell != leftCurCell) // Avoid double counting cells.
					recCells.push_back(rightCurCell);
			}

			middleCurCell = leftCurCell;
			middleCurCell.X += 1;

			while (middleCurCell.X < rightCurCell.X) // Center
			{
				recCells.push_back(middleCurCell);
				middleCurCell.X += 1;
			}

			if (leftContinue) // Continue Top Left Side
			{
				leftContinue = false;
				left2ndCurN -= left2ndPace.X;
				leftCurCell.Y += left2ndUnit.Y;
				recCells.push_back(leftCurCell);
			}

			if (rightContinue) // Continue Top Right Side
			{
				rightContinue = false;
				right2ndCurN -= right2ndPace.X;
				rightCurCell.Y += right2ndUnit.Y;
				recCells.push_back(rightCurCell);
			}
		}
	}

	return recCells;
}

bool PhobosTrajectory::CheckThroughAndSubjectInCell(BulletClass* pBullet, CellClass* pCell, HouseClass* pOwner)
{
	const auto pType = this->GetType();

	for (auto pObject = pCell->GetContent(); pObject; pObject = pObject->NextObject)
	{
		const auto pTechno = abstract_cast<TechnoClass*>(pObject);

		// Non technos and not target friendly forces will be excluded
		if (!pTechno || (pOwner && pOwner->IsAlliedWith(pTechno->Owner) && pTechno != pBullet->Target))
			continue;

		const auto absType = pTechno->WhatAmI();

		// Check building obstacles
		if (absType == AbstractType::Building)
		{
			const auto pBuilding = static_cast<BuildingClass*>(pTechno);

			if (pBuilding->Type->InvisibleInGame)
				continue;

			if (pBuilding->IsStrange() ? !pType->ThroughVehicles : !pType->ThroughBuilding)
			{
				this->ExtraCheck = pTechno;
				return true;
			}
		}

		// Check unit obstacles
		if (!pType->ThroughVehicles && (absType == AbstractType::Unit || absType == AbstractType::Aircraft))
		{
			this->ExtraCheck = pTechno;
			return true;
		}
	}

	return false;
}

void PhobosTrajectory::CalculateNewDamage(BulletClass* pBullet)
{
	const auto ratio = this->GetType()->DamageCountAttenuation.Get();

	// Calculate the attenuation damage under three different scenarios
	if (ratio != 1.0)
	{
		// If the ratio is not 0, the lowest damage will be retained
		if (ratio)
		{
			PhobosTrajectory::SetNewDamage(pBullet->Health, ratio);
			PhobosTrajectory::SetNewDamage(this->ProximityDamage, ratio);
			PhobosTrajectory::SetNewDamage(this->PassDetonateDamage, ratio);
		}
		else
		{
			pBullet->Health = 0;
			this->ProximityDamage = 0;
			this->PassDetonateDamage = 0;
		}
	}
}

void PhobosTrajectory::PassWithDetonateAt(BulletClass* pBullet, HouseClass* pOwner)
{
	if (!this->PassDetonateTimer.Completed())
		return;

	const auto pType = this->GetType();
	const auto pWH = pType->PassDetonateWarhead;

	if (!pWH)
		return;

	this->PassDetonateTimer.Start(pType->PassDetonateDelay > 0 ? pType->PassDetonateDelay : 1);
	auto detonateCoords = pBullet->Location;

	// Whether to detonate at ground level?
	if (pType->PassDetonateLocal)
		detonateCoords.Z = MapClass::Instance->GetCellFloorHeight(detonateCoords);

	const auto damage = this->GetTheTrueDamage(this->PassDetonateDamage, pBullet, nullptr, false);
	WarheadTypeExt::DetonateAt(pWH, detonateCoords, pBullet->Owner, damage, pOwner);
	this->CalculateNewDamage(pBullet);
}

// Select suitable targets and choose the closer targets then attack each target only once.
void PhobosTrajectory::PrepareForDetonateAt(BulletClass* pBullet, HouseClass* pOwner)
{
	const auto pType = this->GetType();

	if (!pType->ProximityWarhead)
		return;

	// Step 1: Find valid targets on the ground within range.
	const auto radius = pType->ProximityRadius.Get();
	std::vector<CellClass*> recCellClass = PhobosTrajectory::GetCellsInProximityRadius(pBullet, radius);
	const size_t cellSize = recCellClass.size() * 2;
	size_t vectSize = cellSize;
	size_t thisSize = 0;

	const CoordStruct velocityCrd
	{
		static_cast<int>(pBullet->Velocity.X),
		static_cast<int>(pBullet->Velocity.Y),
		static_cast<int>(pBullet->Velocity.Z)
	};
	const auto velocitySq = velocityCrd.MagnitudeSquared();
	const auto pTarget = pBullet->Target;

	std::vector<TechnoClass*> validTechnos;
	validTechnos.reserve(vectSize);

	for (const auto& pRecCell : recCellClass)
	{
		for (auto pObject = pRecCell->GetContent(); pObject; pObject = pObject->NextObject)
		{
			const auto pTechno = abstract_cast<TechnoClass*>(pObject);

			if (!pTechno || PhobosTrajectory::CheckTechnoIsInvalid(pTechno))
				continue;

			const auto absType = pTechno->WhatAmI();

			if (absType == AbstractType::Building && static_cast<BuildingClass*>(pTechno)->Type->InvisibleInGame)
				continue;

			// Not directly harming friendly forces
			if (!pType->ProximityAllies && pOwner && pOwner->IsAlliedWith(pTechno->Owner) && pTechno != pTarget)
				continue;

			// Check distance
			const auto targetCrd = pTechno->GetCoords();
			const auto distanceCrd = targetCrd - pBullet->Location;

			if (distanceCrd * velocityCrd < 0 && distanceCrd.Magnitude() > radius) // In front of the bullet
				continue;

			const auto nextDistanceCrd = distanceCrd - velocityCrd;

			if (nextDistanceCrd * velocityCrd > 0 && nextDistanceCrd.Magnitude() > radius) // Behind the next frame bullet
				continue;

			const auto cross = distanceCrd.CrossProduct(nextDistanceCrd).MagnitudeSquared();
			const auto distance = (velocitySq > 1e-10) ? sqrt(cross / velocitySq) : distanceCrd.Magnitude();

			if (absType != AbstractType::Building && distance > radius) // In the cylinder
				continue;

			if (thisSize >= vectSize)
			{
				vectSize += cellSize;
				validTechnos.reserve(vectSize);
			}

			validTechnos.push_back(pTechno);
			thisSize += 1;
		}
	}

	// Step 2: Find valid targets in the air within range if necessary.
	if (pType->ProximityFlight)
	{
		const auto airTracker = &AircraftTrackerClass::Instance;
		airTracker->FillCurrentVector(MapClass::Instance->GetCellAt(pBullet->Location + velocityCrd * 0.5),
			Game::F2I(sqrt(radius * radius + (velocitySq / 4)) / Unsorted::LeptonsPerCell));

		for (auto pTechno = airTracker->Get(); pTechno; pTechno = airTracker->Get())
		{
			if (PhobosTrajectory::CheckTechnoIsInvalid(pTechno))
				continue;

			// Not directly harming friendly forces
			if (!pType->ProximityAllies && pOwner && pOwner->IsAlliedWith(pTechno->Owner) && pTechno != pTarget)
				continue;

			// Check distance
			const auto targetCrd = pTechno->GetCoords();
			const auto distanceCrd = targetCrd - pBullet->Location;

			if (distanceCrd * velocityCrd < 0 && distanceCrd.Magnitude() > radius) // In front of the bullet
				continue;

			const auto nextDistanceCrd = distanceCrd - velocityCrd;

			if (nextDistanceCrd * velocityCrd > 0 && nextDistanceCrd.Magnitude() > radius) // Behind the next frame bullet
				continue;

			const auto cross = distanceCrd.CrossProduct(nextDistanceCrd).MagnitudeSquared();
			const auto distance = (velocitySq > 1e-10) ? sqrt(cross / velocitySq) : distanceCrd.Magnitude();

			if (distance > radius) // In the cylinder
				continue;

			if (thisSize >= vectSize)
			{
				vectSize += cellSize;
				validTechnos.reserve(vectSize);
			}

			validTechnos.push_back(pTechno);
			thisSize += 1;
		}
	}

	// Step 3: Record each target without repetition.
	std::vector<int> casualtyChecked;
	casualtyChecked.reserve(std::max(validTechnos.size(), this->TheCasualty.size()));

	if (const auto pFirer = pBullet->Owner)
		this->TheCasualty[pFirer->UniqueID] = 20;

	// Update Record
	for (const auto& [ID, remainTime] : this->TheCasualty)
	{
		if (remainTime > 0)
			this->TheCasualty[ID] = remainTime - 1;
		else
			casualtyChecked.push_back(ID);
	}

	for (const auto& ID : casualtyChecked)
		this->TheCasualty.erase(ID);

	std::vector<TechnoClass*> validTargets;

	// checking for duplicate
	for (const auto& pTechno : validTechnos)
	{
		if (!this->TheCasualty.contains(pTechno->UniqueID))
			validTargets.push_back(pTechno);

		this->TheCasualty[pTechno->UniqueID] = 20;
	}

	// Step 4: Detonate warheads in sequence based on distance.
	const auto targetsSize = validTargets.size();

	if (this->ProximityImpact > 0 && static_cast<int>(targetsSize) > this->ProximityImpact)
	{
		std::sort(&validTargets[0], &validTargets[targetsSize],[pBullet](TechnoClass* pTechnoA, TechnoClass* pTechnoB)
		{
			const auto distanceA = pTechnoA->GetCoords().DistanceFromSquared(pBullet->SourceCoords);
			const auto distanceB = pTechnoB->GetCoords().DistanceFromSquared(pBullet->SourceCoords);

			// Distance priority
			if (distanceA < distanceB)
				return true;

			if (distanceA > distanceB)
				return false;

			return pTechnoA->UniqueID < pTechnoB->UniqueID;
		});
	}

	for (const auto& pTechno : validTargets)
	{
		// Not effective for the technos following it.
		if (pTechno == this->ExtraCheck)
			break;

		// Last chance
		if (this->ProximityImpact == 1)
		{
			this->ExtraCheck = pTechno;
			break;
		}

		// Skip technos that are within range but will not obstruct and cannot be passed through
		const auto absType = pTechno->WhatAmI();

		if (!pType->ThroughVehicles && (absType == AbstractType::Unit || absType == AbstractType::Aircraft))
			continue;

		if (absType == AbstractType::Building && (static_cast<BuildingClass*>(pTechno)->IsStrange() ? !pType->ThroughVehicles : !pType->ThroughBuilding))
			continue;

		this->ProximityDetonateAt(pBullet, pOwner, pTechno);

		// Record the number of times
		if (this->ProximityImpact > 0)
			--this->ProximityImpact;
	}
}

void PhobosTrajectory::ProximityDetonateAt(BulletClass* pBullet, HouseClass* pOwner, TechnoClass* pTarget)
{
	const auto pType = this->GetType();
	auto damage = this->GetTheTrueDamage(this->ProximityDamage, pBullet, pType->ProximityMedial ? nullptr : pTarget, false);

	// Choose the method of causing damage
	if (pType->ProximityDirect)
		pTarget->ReceiveDamage(&damage, 0, pType->ProximityWarhead, pBullet->Owner, false, false, pOwner);
	else if (pType->ProximityMedial)
		WarheadTypeExt::DetonateAt(pType->ProximityWarhead, pBullet->Location, pBullet->Owner, damage, pOwner);
	else
		WarheadTypeExt::DetonateAt(pType->ProximityWarhead, pTarget, pBullet->Owner, damage, pOwner);

	this->CalculateNewDamage(pBullet);
}

int PhobosTrajectory::GetTheTrueDamage(int damage, BulletClass* pBullet, TechnoClass* pTechno, bool self)
{
	if (damage == 0)
		return 0;

	const auto pType = this->GetType();

	// Calculate damage distance attenuation
	if (pType->DamageEdgeAttenuation != 1.0)
	{
		const auto damageMultiplier = this->GetExtraDamageMultiplier(pBullet, pTechno);
		const auto calculatedDamage = self ? damage * damageMultiplier : damage * this->FirepowerMult * damageMultiplier;
		const auto signal = Math::sgn(calculatedDamage);
		damage = static_cast<int>(calculatedDamage);

		// Retain minimal damage
		if (!damage && pType->DamageEdgeAttenuation > 0.0)
			damage = signal;
	}

	return damage;
}

double PhobosTrajectory::GetExtraDamageMultiplier(BulletClass* pBullet, TechnoClass* pTechno)
{
	double damageMult = 1.0;
	const auto distance = pBullet->Location.DistanceFrom(pBullet->SourceCoords);

	if (this->AttenuationRange < static_cast<int>(distance))
		return this->GetType()->DamageEdgeAttenuation;

	// Remove the first cell distance for calculation
	if (distance > 256.0)
		damageMult += (this->GetType()->DamageEdgeAttenuation - 1.0) * ((distance - 256.0) / (static_cast<double>(this->AttenuationRange - 256)));

	return damageMult;
}
