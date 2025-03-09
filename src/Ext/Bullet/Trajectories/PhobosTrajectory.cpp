#include "PhobosTrajectory.h"
#include "StraightTrajectory.h"
#include "BombardTrajectory.h"
#include "MissileTrajectory.h"
#include "EngraveTrajectory.h"
#include "ParabolaTrajectory.h"
#include "TracingTrajectory.h"

#include <BulletClass.h>
#include <OverlayTypeClass.h>
#include <AircraftTrackerClass.h>
#include <Helpers/Macro.h>

#include <Ext/BulletType/Body.h>
#include <Ext/Bullet/Body.h>
#include <Ext/WeaponType/Body.h>
#include <Ext/WarheadType/Body.h>

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

void PhobosTrajectory::OnUnlimbo(BulletClass* pBullet, CoordStruct* pCoord, BulletVelocity* pVelocity)
{
	pBullet->Velocity = BulletVelocity::Empty;
	const auto pTarget = pBullet->Target;
	const auto pFirer = pBullet->Owner;
	const auto pType = this->GetType();

	this->LastTargetCoord = pBullet->TargetCoords;
	this->TargetIsTechno = static_cast<bool>(abstract_cast<TechnoClass*>(pBullet->Target));
	this->TargetInTheAir = (pTarget->AbstractFlags & AbstractFlags::Object) ? (static_cast<ObjectClass*>(pTarget)->GetHeight() > Unsorted::CellHeight) : false;

	// Record some information
	if (const auto pWeapon = pBullet->WeaponType)
	{
		this->AttenuationRange = pWeapon->Range;
		this->CountOfBurst = pWeapon->Burst;

		if (pType->ApplyRangeModifiers && pFirer)
			this->AttenuationRange = WeaponTypeExt::GetRangeWithModifiers(pWeapon, pFirer);
	}

	if (pFirer)
	{
		for (auto pTrans = pFirer->Transporter; pTrans; pTrans = pTrans->Transporter)
			this->TechnoInTransport = pTrans->UniqueID;

		this->CurrentBurst = pFirer->CurrentBurstIndex;
		this->FirepowerMult = pFirer->FirepowerMultiplier * TechnoExt::ExtMap.Find(pFirer)->AE.FirepowerMultiplier;

		if (pType->MirrorCoord && (pFirer->CurrentBurstIndex & 1))
			this->OffsetCoord.Y = -(this->OffsetCoord.Y);

		// Only necessary when weapons need to be fired
		if (pType->DisperseWeapons.size() && pType->RecordSourceCoord)
			this->GetTechnoFLHCoord(pBullet, pFirer);
	}
	else
	{
		this->NotMainWeapon = true;
	}

	this->PassDetonateTimer.Start(pType->PassDetonateInitialDelay > 0 ? pType->PassDetonateInitialDelay : 0);

	if (!pType->DisperseWeapons.empty() && !pType->DisperseCounts.empty() && this->DisperseCycle)
	{
		this->DisperseCount = pType->DisperseCounts[0];
		this->DisperseTimer.Start(pType->DisperseInitialDelay);
	}
}

bool PhobosTrajectory::OnAI(BulletClass* pBullet)
{
	const auto pOwner = pBullet->Owner ? pBullet->Owner->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;
	const auto pType = this->GetType();

	if (pType->PassDetonate)
		this->PassWithDetonateAt(pBullet, pOwner);

	if (this->ProximityImpact != 0 && pType->ProximityRadius.Get() > 0)
		this->PrepareForDetonateAt(pBullet, pOwner);

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
	// Let the distance slightly exceed
	double locationDistance = 32.0;
	bool velocityCheck = false;

	const auto pType = this->GetType();
	const bool checkThrough = (!pType->ThroughBuilding || !pType->ThroughVehicles);

	if (pType->Speed < 256.0) // Low speed with checkSubject was already done well.
	{
		// Blocked by obstacles?
		if (checkThrough && this->CheckThroughAndSubjectInCell(pBullet, MapClass::Instance->GetCellAt(pBullet->Location), pOwner))
			velocityCheck = true;
	}
	else
	{
		const bool subjectToGround = this->GetCanHitGround();
		const bool subjectToWalls = pBullet->Type->SubjectToWalls;
		const bool checkSubject = (subjectToGround || subjectToWalls);

		// When in high speed, it's necessary to check each cell on the path that the next frame will pass through
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
					velocityCheck = true;
					locationDistance += PhobosTrajectory::Get2DDistance(curCoord, theSourceCoords);
					break;
				}

				curCoord += stepCoord;
				pCurCell = MapClass::Instance->GetCellAt(curCoord);
			}

			// TODO Fire storm wall?
		}
	}

	// Check if the bullet needs to slow down the speed
	if (velocityCheck)
	{
		this->RemainingDistance = 0;
		const auto velocity = PhobosTrajectory::Get2DVelocity(pBullet->Velocity);

		if (locationDistance < velocity)
			pBullet->Velocity *= (locationDistance / velocity);
	}
	else if (this->RemainingDistance < pType->Speed) // Check if the distance to the destination exceeds the speed limit
	{
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
	const auto position = pDetonateAt->GetCoords();
	const auto distance = position.DistanceFrom(pBullet->Location);
	const auto velocity = pBullet->Velocity.Magnitude();

	pBullet->SetTarget(pDetonateAt);
	pBullet->TargetCoords = position;

	if (std::abs(velocity) > 1e-10 && distance < velocity)
		pBullet->Velocity *= distance / velocity;

	// Need to cause additional damage?
	if (!this->ProximityImpact)
		return;

	const auto pWH = pType->ProximityWarhead;

	if (!pWH)
		return;

	auto damage = this->GetTheTrueDamage(this->ProximityDamage, pBullet, pType->ProximityMedial ? nullptr : pDetonateAt, false);

	if (pType->ProximityDirect)
		pDetonateAt->ReceiveDamage(&damage, 0, pWH, pBullet->Owner, false, false, pOwner);
	else if (pType->ProximityMedial)
		WarheadTypeExt::DetonateAt(pWH, pBullet->Location, pBullet->Owner, damage, pOwner);
	else
		WarheadTypeExt::DetonateAt(pWH, position, pBullet->Owner, damage, pOwner, pDetonateAt);

	this->CalculateNewDamage(pBullet);
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

BulletVelocity PhobosTrajectory::RotateAboutTheAxis(const BulletVelocity& theSpeed, BulletVelocity& theAxis, double theRadian)
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

void PhobosTrajectory::DisperseBurstSubstitution(BulletClass* pBullet, const CoordStruct& axis, double rotateCoord, int curBurst, int maxBurst, bool mirror)
{
	const auto createBulletTargetToSource = pBullet->TargetCoords - pBullet->SourceCoords;
	const auto rotateAngle = Math::atan2(createBulletTargetToSource.Y , createBulletTargetToSource.X);

	// Calculate the actual rotation axis
	BulletVelocity rotationAxis
	{
		axis.X * Math::cos(rotateAngle) + axis.Y * Math::sin(rotateAngle),
		axis.X * Math::sin(rotateAngle) - axis.Y * Math::cos(rotateAngle),
		static_cast<double>(axis.Z)
	};

	double extraRotate = 0.0;

	if (mirror)
	{
		if (curBurst % 2 == 1)
			rotationAxis *= -1;

		extraRotate = Math::Pi * (rotateCoord * ((curBurst / 2) / (maxBurst - 1.0) - 0.5)) / 180;
	}
	else
	{
		extraRotate = Math::Pi * (rotateCoord * (curBurst / (maxBurst - 1.0) - 0.5)) / 180;
	}

	// Rotate the selected angle
	pBullet->Velocity = PhobosTrajectory::RotateAboutTheAxis(pBullet->Velocity, rotationAxis, extraRotate);
}

bool PhobosTrajectory::BulletRetargetTechno(BulletClass* pBullet)
{
	const auto pType = this->GetType();
	bool check = false;

	// Will only attempt to search for a new target when the original target is a techno
	if (this->TargetIsTechno)
	{
		if (!pBullet->Target)
			check = true;
		else if (pBullet->Target->AbstractFlags & AbstractFlags::Techno)
			check = PhobosTrajectory::CheckTechnoIsInvalid(static_cast<TechnoClass*>(pBullet->Target));
		// Current target may be a bullet, and will not retarget at this time, in order to adapt to thermal decoys
	}

	if (!check)
		return false;

	// Check whether need to detonate directly after the target was lost
	if (pType->RetargetRadius < 0)
		return true;

	const auto pFirer = pBullet->Owner;
	auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;

	// Replace with neutral house when the firer house does not exist
	if (!pOwner || pOwner->Defeated)
	{
		if (const auto pNeutral = HouseClass::FindNeutral())
			pOwner = pNeutral;
		else
			return false;
	}

	const auto retargetCoords = this->GetRetargetCenter(pBullet);
	const auto retargetRange = pType->RetargetRadius * Unsorted::LeptonsPerCell;
	TechnoClass* pNewTechno = nullptr;

	// Find the first target
	if (!this->TargetInTheAir) // Only get same type (on ground / in air)
	{
		const auto retargetCell = CellClass::Coord2Cell(retargetCoords);

		for (CellSpreadEnumerator thisCell(static_cast<size_t>(pType->RetargetRadius + 0.99)); thisCell; ++thisCell)
		{
			if (const auto pCell = MapClass::Instance->TryGetCellAt(*thisCell + retargetCell))
			{
				for (auto pObject = pCell->GetContent(); pObject; pObject = pObject->NextObject)
				{
					const auto pTechno = abstract_cast<TechnoClass*>(pObject);

					if (!pTechno || PhobosTrajectory::CheckTechnoIsInvalid(pTechno))
						continue;

					const auto pTechnoType = pTechno->GetTechnoType();

					if (!pTechnoType->LegalTarget)
						continue;
					else if (pTechno->WhatAmI() == AbstractType::Building && static_cast<BuildingClass*>(pTechno)->Type->InvisibleInGame)
						continue;
					else if (MapClass::GetTotalDamage(100, pBullet->WH, pTechnoType->Armor, 0) == 0)
						continue;
					else if (pTechno->GetCoords().DistanceFrom(retargetCoords) > retargetRange)
						continue;

					if (const auto pWeapon = pBullet->WeaponType)
					{
						if (pTechno->GetCoords().DistanceFrom(pFirer ? pFirer->GetCoords() : pBullet->SourceCoords) > pWeapon->Range)
							continue;

						const auto pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);

						if (!PhobosTrajectory::CheckWeaponCanTarget(pWeaponExt, pFirer, pTechno))
							continue;
						if (!PhobosTrajectory::CheckWeaponValidness(pOwner, pTechno, pCell, pWeaponExt->CanTargetHouses))
							continue;
					}

					pNewTechno = pTechno;
					break;
				}
			}

			if (pNewTechno)
				break;
		}
	}
	else
	{
		const auto airTracker = &AircraftTrackerClass::Instance;
		airTracker->FillCurrentVector(MapClass::Instance->GetCellAt(retargetCoords), Game::F2I(pType->RetargetRadius));

		for (auto pTechno = airTracker->Get(); pTechno; pTechno = airTracker->Get())
		{
			if (PhobosTrajectory::CheckTechnoIsInvalid(pTechno))
				continue;

			const auto pTechnoType = pTechno->GetTechnoType();

			if (!pTechnoType->LegalTarget)
				continue;
			else if (MapClass::GetTotalDamage(100, pBullet->WH, pTechnoType->Armor, 0) == 0)
				continue;
			else if (pTechno->GetCoords().DistanceFrom(retargetCoords) > retargetRange)
				continue;

			if (const auto pWeapon = pBullet->WeaponType)
			{
				if (pTechno->GetCoords().DistanceFrom(pFirer ? pFirer->GetCoords() : pBullet->SourceCoords) > pWeapon->Range)
					continue;

				const auto pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);

				if (!PhobosTrajectory::CheckWeaponCanTarget(pWeaponExt, pFirer, pTechno))
					continue;
				if (!PhobosTrajectory::CheckWeaponValidness(pOwner, pTechno, pTechno->GetCell(), pWeaponExt->CanTargetHouses))
					continue;
			}

			pNewTechno = pTechno;
			break;
		}
	}

	// Replace if there is a new target
	if (pNewTechno)
		this->SetBulletNewTarget(pBullet, pNewTechno);

	// If not found, in order to minimize the response time, it will continue to check in the next frame, so the performance will be reduced a bit
	return false;
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
	const auto pWH = pType->ProximityWarhead;

	if (!pWH)
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

		// Cause damage
		auto damage = this->GetTheTrueDamage(this->ProximityDamage, pBullet, pType->ProximityMedial ? nullptr : pTechno, false);

		if (pType->ProximityDirect)
			pTechno->ReceiveDamage(&damage, 0, pWH, pBullet->Owner, false, false, pOwner);
		else if (pType->ProximityMedial)
			WarheadTypeExt::DetonateAt(pWH, pBullet->Location, pBullet->Owner, damage, pOwner);
		else
			WarheadTypeExt::DetonateAt(pWH, pTechno->GetCoords(), pBullet->Owner, damage, pOwner, pTechno);

		this->CalculateNewDamage(pBullet);

		if (this->ProximityImpact > 0)
			--this->ProximityImpact;
	}
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

void PhobosTrajectory::GetTechnoFLHCoord(BulletClass* pBullet, TechnoClass* pTechno)
{
	const auto pExt = TechnoExt::ExtMap.Find(pTechno);

	// Record the launch location, the building has an additional offset
	if (!pExt || !pExt->LastWeaponType || pExt->LastWeaponType->Projectile != pBullet->Type)
	{
		this->FLHCoord = CoordStruct::Empty;
		this->NotMainWeapon = true;
		return;
	}
	else if (pTechno->WhatAmI() == AbstractType::Building)
	{
		// The difference between the building and other units here comes from the difference between its GetCoords() and GetRenderCoords()
		const auto pBuilding = static_cast<BuildingClass*>(pTechno);
		Matrix3D mtx;
		mtx.MakeIdentity();

		if (pTechno->HasTurret())
		{
			TechnoTypeExt::ApplyTurretOffset(pBuilding->Type, &mtx);
			mtx.RotateZ(static_cast<float>(pTechno->TurretFacing().GetRadian<32>()));
		}

		mtx.Translate(static_cast<float>(pExt->LastWeaponFLH.X), static_cast<float>(pExt->LastWeaponFLH.Y), static_cast<float>(pExt->LastWeaponFLH.Z));
		const auto result = mtx.GetTranslation();
		this->BuildingCoord = pBullet->SourceCoords - pBuilding->GetRenderCoords() - CoordStruct { static_cast<int>(result.X), -static_cast<int>(result.Y), static_cast<int>(result.Z) };
	}

	this->FLHCoord = pExt->LastWeaponFLH;
}

CoordStruct PhobosTrajectory::GetWeaponFireCoord(BulletClass* pBullet, TechnoClass* pTechno)
{
	const auto pType = this->GetType();

	if (pType->DisperseFromFirer)
	{
		for (auto pTrans = pTechno->Transporter; pTrans; pTrans = pTrans->Transporter)
			pTechno = pTrans;

		if (!this->NotMainWeapon && pTechno && !pTechno->InLimbo)
		{
			if (pTechno->WhatAmI() != AbstractType::Building)
				return TechnoExt::GetFLHAbsoluteCoords(pTechno, this->FLHCoord, pTechno->HasTurret());

			const auto pBuilding = static_cast<BuildingClass*>(pTechno);
			Matrix3D mtx;
			mtx.MakeIdentity();

			if (pTechno->HasTurret())
			{
				TechnoTypeExt::ApplyTurretOffset(pBuilding->Type, &mtx);
				mtx.RotateZ(static_cast<float>(pTechno->TurretFacing().GetRadian<32>()));
			}

			mtx.Translate(static_cast<float>(this->FLHCoord.X), static_cast<float>(this->FLHCoord.Y), static_cast<float>(this->FLHCoord.Z));
			const auto result = mtx.GetTranslation();

			return (pBuilding->GetRenderCoords() + this->BuildingCoord + CoordStruct { static_cast<int>(result.X), -static_cast<int>(result.Y), static_cast<int>(result.Z) });
		}

		return pBullet->SourceCoords;
	}

	const auto& weaponCoord = pType->DisperseCoord.Get();

	if (weaponCoord == CoordStruct::Empty)
		return pBullet->Location;

	const auto rotateRadian = Math::atan2(pBullet->TargetCoords.Y - pBullet->Location.Y , pBullet->TargetCoords.X - pBullet->Location.X);
	CoordStruct fireOffsetCoord
	{
		static_cast<int>(weaponCoord.X * Math::cos(rotateRadian) + weaponCoord.Y * Math::sin(rotateRadian)),
		static_cast<int>(weaponCoord.X * Math::sin(rotateRadian) - weaponCoord.Y * Math::cos(rotateRadian)),
		weaponCoord.Z
	};

	return pBullet->Location + fireOffsetCoord;
}

bool PhobosTrajectory::CheckFireFacing(BulletClass* pBullet)
{
	const auto pType = this->GetType();

	if (!pType->DisperseFaceCheck || pType->BulletROT < 0 || pType->BulletSpin)
		return true;

	const auto& theBullet = pBullet->Location;
	const auto& theTarget = pBullet->TargetCoords;
	const auto targetDir = DirStruct { Math::atan2(theTarget.Y - theBullet.Y, theTarget.X - theBullet.X) };
	const auto bulletDir = DirStruct { Math::atan2(pBullet->Velocity.Y, pBullet->Velocity.X) };

	return std::abs(static_cast<short>(static_cast<short>(targetDir.Raw) - static_cast<short>(bulletDir.Raw))) <= (2048 + (pType->BulletROT << 8));
}

bool PhobosTrajectory::PrepareDisperseWeapon(BulletClass* pBullet)
{
	const auto pType = this->GetType();

	if (!this->DisperseCycle)
		return pType->DisperseSuicide;

	if (this->DisperseCount)
	{
		const auto pFirer = pBullet->Owner;
		auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;

		// Replace with neutral house when the firer house does not exist
		if (!pOwner || pOwner->Defeated)
		{
			if (const auto pNeutral = HouseClass::FindNeutral())
				pOwner = pNeutral;
			else
				return true;
		}

		const auto fireCoord = this->GetWeaponFireCoord(pBullet, pFirer);
		this->FireDisperseWeapon(pBullet, pFirer, fireCoord, pOwner);
	}

	if (const int validDelays = pType->DisperseDelays.size())
	{
		const auto delay = pType->DisperseDelays[(this->DisperseIndex < validDelays) ? this->DisperseIndex : (validDelays - 1)];
		this->DisperseTimer.Start((delay > 0) ? delay : 1);
	}

	// Record of Launch Times
	if (this->DisperseCount < 0 || --this->DisperseCount > 0)
		return false;

	const int groupSize = pType->DisperseSeparate ? pType->DisperseWeapons.size() : pType->DisperseCounts.size();

	// Next group
	if (++this->DisperseIndex < groupSize)
	{
		const int validCounts = pType->DisperseCounts.size();
		this->DisperseCount = validCounts > 0 ? pType->DisperseCounts[(this->DisperseIndex < validCounts) ? this->DisperseIndex : (validCounts - 1)] : 0;

		return false;
	}

	// Next cycle
	this->DisperseIndex = 0;
	this->DisperseCount = pType->DisperseCounts.empty() ? 0 : pType->DisperseCounts[0];

	if (this->DisperseCycle < 0 || --this->DisperseCycle > 0)
		return false;

	// Stop
	this->DisperseTimer.Stop();

	// Detonate if the number of attempts is exhausted at the end of the attack
	return pType->DisperseSuicide;
}

void PhobosTrajectory::FireDisperseWeapon(BulletClass* pBullet, TechnoClass* pFirer, const CoordStruct& sourceCoord, HouseClass* pOwner)
{
	const auto pType = this->GetType();

	// Launch quantity check
	const int validWeapons = pType->DisperseWeapons.size();
	const int validBursts = pType->DisperseBursts.size();

	if (!validWeapons || !validBursts)
		return;

	// Set basic target
	const auto pTarget = pBullet->Target ? pBullet->Target
		: (this->TargetInTheAir ? nullptr : MapClass::Instance->TryGetCellAt(pBullet->TargetCoords));

	// Launch weapons in sequence
	for (int weaponNum = 0; weaponNum < validWeapons; ++weaponNum)
	{
		int curIndex = weaponNum;

		// Only launch one group
		if (pType->DisperseSeparate)
		{
			// Set the current weapon number
			curIndex = Math::min(validWeapons - 1, this->DisperseIndex);

			// End directly after firing this weapon
			weaponNum = validWeapons;
		}

		const auto pWeapon = pType->DisperseWeapons[curIndex];
		const auto pWeaponExt = WeaponTypeExt::ExtMap.Find(pWeapon);
		const auto burstCount = pType->DisperseBursts[(curIndex < validBursts) ? curIndex : (validBursts - 1)];

		if (burstCount <= 0)
			continue;

		// Only attack the bullet itself
		if (pType->DisperseFromFirer)
		{
			for (int burstNum = 0; burstNum < burstCount; burstNum++)
				this->CreateDisperseBullets(pFirer, sourceCoord, pWeapon, pBullet, pOwner, burstNum, burstCount);

			continue;
		}

		// Only attack the original target
		if (!pType->DisperseRetarget)
		{
			// Launch only when the target exist
			if (pTarget)
			{
				for (int burstNum = 0; burstNum < burstCount; burstNum++)
					this->CreateDisperseBullets(pFirer, sourceCoord, pWeapon, pTarget, pOwner, burstNum, burstCount);
			}

			continue;
		}

		int burstNow = 0;

		// Prioritize attacking the original target once
		if (pType->DisperseTendency && burstCount > 0 && pTarget)
		{
			this->CreateDisperseBullets(pFirer, sourceCoord, pWeapon, pTarget, pOwner, burstNow, burstCount);
			++burstNow;

			if (burstCount <= 1)
				continue;
		}

		// Select new targets:
		// Where to select?
		const auto centerCoords = pType->DisperseLocation ? pBullet->Location : pBullet->TargetCoords;
		const auto centerCell = CellClass::Coord2Cell(centerCoords);

		std::vector<AbstractClass*> validTechnos;
		std::vector<AbstractClass*> validObjects;
		std::vector<AbstractClass*> validCells;

		// Select what?
		const bool checkTechnos = (pWeaponExt->CanTarget & AffectedTarget::AllContents) != AffectedTarget::None;
		const bool checkObjects = pType->DisperseMarginal;
		const bool checkCells = (pWeaponExt->CanTarget & AffectedTarget::AllCells) != AffectedTarget::None;

		const size_t initialSize = pWeapon->Range >> 7;

		if (checkTechnos)
			validTechnos.reserve(initialSize);

		if (checkObjects)
			validObjects.reserve(initialSize >> 1);

		if (checkCells)
			validCells.reserve(initialSize);

		// How to select?
		if (pType->DisperseHolistic || !this->TargetInTheAir || checkCells) // On land targets
		{
			// Ensure that the same building is not recorded repeatedly
			std::set<TechnoClass*> inserted;

			for (CellSpreadEnumerator thisCell(static_cast<size_t>((static_cast<double>(pWeapon->Range) / Unsorted::LeptonsPerCell) + 0.99)); thisCell; ++thisCell)
			{
				if (const auto pCell = MapClass::Instance->TryGetCellAt(*thisCell + centerCell))
				{
					if (checkCells && EnumFunctions::IsCellEligible(pCell, pWeaponExt->CanTarget, true, true))
						validCells.push_back(pCell);

					if (!pType->DisperseHolistic && this->TargetInTheAir)
						continue;

					for (auto pObject = pCell->GetContent(); pObject; pObject = pObject->NextObject)
					{
						const auto pTechno = abstract_cast<TechnoClass*>(pObject);

						if (!pTechno)
						{
							if (checkObjects && (!pType->DisperseTendency || pType->DisperseDoRepeat || pObject != pTarget))
							{
								const auto pObjType = pObject->GetType();

								if (pObjType && !pObjType->Immune && centerCoords.DistanceFrom(pObject->GetCoords()) <= pWeapon->Range)
									validObjects.push_back(pObject);
							}

							continue;
						}

						if (!checkTechnos || PhobosTrajectory::CheckTechnoIsInvalid(pTechno))
							continue;

						const auto pTechnoType = pTechno->GetTechnoType();

						if (!pTechnoType->LegalTarget)
							continue;
						else if (pType->DisperseTendency && !pType->DisperseDoRepeat && pTechno == pTarget)
							continue;

						const auto isBuilding = pTechno->WhatAmI() == AbstractType::Building;

						if (isBuilding && (static_cast<BuildingClass*>(pTechno)->Type->InvisibleInGame || inserted.contains(pTechno)))
							continue;
						else if (centerCoords.DistanceFrom(pTechno->GetCoords()) > pWeapon->Range)
							continue;
						else if (MapClass::GetTotalDamage(100, pWeapon->Warhead, pTechnoType->Armor, 0) == 0)
							continue;
						else if (!PhobosTrajectory::CheckWeaponCanTarget(pWeaponExt, pFirer, pTechno))
							continue;
						else if (!PhobosTrajectory::CheckWeaponValidness(pOwner, pTechno, pCell, pWeaponExt->CanTargetHouses))
							continue;

						validTechnos.push_back(pTechno);

						if (isBuilding)
							inserted.insert(pTechno);
					}
				}
			}
		}

		if ((pType->DisperseHolistic || this->TargetInTheAir) && checkTechnos) // In air targets
		{
			const auto airTracker = &AircraftTrackerClass::Instance;
			airTracker->FillCurrentVector(MapClass::Instance->GetCellAt(centerCoords), Game::F2I(static_cast<double>(pWeapon->Range) / Unsorted::LeptonsPerCell));

			for (auto pTechno = airTracker->Get(); pTechno; pTechno = airTracker->Get())
			{
				if (PhobosTrajectory::CheckTechnoIsInvalid(pTechno))
					continue;

				const auto pTechnoType = pTechno->GetTechnoType();

				if (!pTechnoType->LegalTarget)
					continue;
				else if (pType->DisperseTendency && !pType->DisperseDoRepeat && pTechno == pTarget)
					continue;
				else if (centerCoords.DistanceFrom(pTechno->GetCoords()) > pWeapon->Range)
					continue;
				else if (MapClass::GetTotalDamage(100, pWeapon->Warhead, pTechnoType->Armor, 0) == 0)
					continue;
				else if (!PhobosTrajectory::CheckWeaponCanTarget(pWeaponExt, pFirer, pTechno))
					continue;
				else if (!PhobosTrajectory::CheckWeaponValidness(pOwner, pTechno, pTechno->GetCell(), pWeaponExt->CanTargetHouses))
					continue;

				validTechnos.push_back(pTechno);
			}
		}

		// Arrange the targets
		int burstRemain = burstCount - burstNow;
		std::vector<AbstractClass*> validTargets;
		validTargets.reserve(burstRemain);
		std::vector<AbstractClass*>* vectors[3] = { &validTechnos, &validObjects, &validCells };

		if (pType->DisperseDoRepeat) // Repeatedly attack new targets
		{
			for (const auto pVector : vectors)
			{
				if (pVector->empty())
					continue;

				const int size = pVector->size();
				const int base = burstRemain / size;
				const int remainder = burstRemain % size;

				if (remainder && size > 1) // Shuffle
				{
					for (int i = size - 1; i > 0; --i)
					{
						const int j = ScenarioClass::Instance->Random.RandomRanged(0, i);

						if (i != j)
							std::swap((*pVector)[i], (*pVector)[j]);
					}
				}

				// Fill in multiple items in order
				for (int i = 0; i < size; ++i)
				{
					int count = base + (i < remainder ? 1 : 0);

					for (int j = 0; j < count; ++j)
						validTargets.push_back((*pVector)[i]);
				}

				break;
			}
		}
		else // Missile attacks on all optional targets
		{
			for (const auto pVector : vectors)
			{
				if (burstRemain <= 0)
					break;

				if (pVector->empty())
					continue;

				const int size = pVector->size();
				const int take = Math::min(burstRemain, size);

				if (take != size && size > 1) // Shuffle
				{
					for (int i = size - 1; i > 0; --i)
					{
						const int j = ScenarioClass::Instance->Random.RandomRanged(0, i);

						if (i != j)
							std::swap((*pVector)[i], (*pVector)[j]);
					}
				}

				// Fill in all optional targets in order once
				validTargets.insert(validTargets.end(), pVector->begin(), pVector->begin() + take);
				burstRemain -= take;
			}
		}

		// When WeaponTendency=false, if no suitable target can be found, attempt to attack the original target once
		if (validTargets.empty() && pTarget && burstNow != 1)
			validTargets.push_back(pTarget);

		for (const auto& pNewTarget : validTargets)
		{
			this->CreateDisperseBullets(pFirer, sourceCoord, pWeapon, pNewTarget, pOwner, burstNow, burstCount);
			++burstNow;
		}
	}
}

void PhobosTrajectory::CreateDisperseBullets(TechnoClass* pTechno, const CoordStruct& sourceCoord, WeaponTypeClass* pWeapon, AbstractClass* pTarget, HouseClass* pOwner, int curBurst, int maxBurst)
{
	const auto finalDamage = static_cast<int>(pWeapon->Damage * this->FirepowerMult);

	if (const auto pCreateBullet = pWeapon->Projectile->CreateBullet(pTarget, pTechno, finalDamage, pWeapon->Warhead, pWeapon->Speed, pWeapon->Bright))
	{
		// Record basic information
		BulletExt::SimulatedFiringUnlimbo(pCreateBullet, pOwner, pWeapon, sourceCoord, false);
		const auto pBulletExt = BulletExt::ExtMap.Find(pCreateBullet);

		if (const auto pTraj = pBulletExt->Trajectory.get())
		{
			const auto flag = pTraj->Flag();

			if (flag == TrajectoryFlag::Missile)
			{
				const auto pTrajectory = static_cast<MissileTrajectory*>(pTraj);
				const auto pTrajType = pTrajectory->Type;
				pTrajectory->FirepowerMult = this->FirepowerMult;

				// The created bullet's velocity calculation has been completed, so we should stack the calculations.
				if (pTrajectory->UseDisperseBurst && std::abs(pTrajType->RotateCoord) > 1e-10 && curBurst >= 0 && maxBurst > 1 && !pTrajType->UniqueCurve && pTrajectory->PreAimCoord != CoordStruct::Empty)
					this->PhobosTrajectory::DisperseBurstSubstitution(pCreateBullet, pTrajType->AxisOfRotation.Get(), pTrajType->RotateCoord, curBurst, maxBurst, pTrajType->MirrorCoord);

				if (pTrajType->RecordSourceCoord && pTechno && this->GetType()->RecordSourceCoord && this->FLHCoord != CoordStruct::Empty)
				{
					pTrajectory->FLHCoord = this->FLHCoord;
					pTrajectory->BuildingCoord = this->BuildingCoord;
					pTrajectory->CurrentBurst = this->CurrentBurst;
				}
			}
			else if (flag == TrajectoryFlag::Straight)
			{
				const auto pTrajectory = static_cast<StraightTrajectory*>(pTraj);
				const auto pTrajType = pTrajectory->Type;
				pTrajectory->FirepowerMult = this->FirepowerMult;

				// The straight trajectory bullets has LeadTimeCalculate=true are not calculate its velocity yet.
				if (pTrajectory->UseDisperseBurst && std::abs(pTrajType->RotateCoord) > 1e-10 && curBurst >= 0 && maxBurst > 1)
				{
					if (pTrajType->LeadTimeCalculate && abstract_cast<FootClass*>(pTarget))
					{
						pTrajectory->CurrentBurst = curBurst;
						pTrajectory->CountOfBurst = maxBurst;
						pTrajectory->UseDisperseBurst = false;
					}
					else
					{
						this->PhobosTrajectory::DisperseBurstSubstitution(pCreateBullet, pTrajType->AxisOfRotation.Get(), pTrajType->RotateCoord, curBurst, maxBurst, pTrajType->MirrorCoord);
					}
				}
			}
			else if (flag == TrajectoryFlag::Bombard)
			{
				const auto pTrajectory = static_cast<BombardTrajectory*>(pTraj);
				const auto pTrajType = pTrajectory->Type;

				// The bombard trajectory bullets without NoLaunch and FreeFallOnTarget can change the velocity.
				if (pTrajectory->UseDisperseBurst && std::abs(pTrajType->RotateCoord) > 1e-10 && curBurst >= 0 && maxBurst > 1 && (!pTrajType->NoLaunch || !pTrajType->FreeFallOnTarget))
				{
					pTrajectory->CurrentBurst = curBurst;
					pTrajectory->CountOfBurst = maxBurst;
					pTrajectory->UseDisperseBurst = false;

					// Bombard is quite special, in this case it needs to be calculated twice
					if (!pTrajType->NoLaunch || !pTrajType->LeadTimeCalculate || !abstract_cast<FootClass*>(pTarget))
						this->PhobosTrajectory::DisperseBurstSubstitution(pCreateBullet, pTrajType->AxisOfRotation.Get(), pTrajType->RotateCoord, curBurst, maxBurst, pTrajType->MirrorCoord);
				}
			}
			else if (flag == TrajectoryFlag::Engrave)
			{
				const auto pTrajectory = static_cast<EngraveTrajectory*>(pTraj);
				const auto pTrajType = pTrajectory->Type;

				// Inherit the fire location of the record
				if (pTrajType->UseDisperseCoord && pTechno && this->GetType()->RecordSourceCoord && this->FLHCoord != CoordStruct::Empty)
				{
					pTrajectory->FLHCoord = this->FLHCoord;
					pTrajectory->BuildingCoord = this->BuildingCoord;
					pTrajectory->NotMainWeapon = false;

					// Special circumstances, mirror Engrave starting and ending positions
					if ((this->CurrentBurst % 2) && pTrajType->MirrorCoord)
					{
						pTrajectory->SourceCoord.Y = -(pTrajectory->SourceCoord.Y);
						pTrajectory->TargetCoord.Y = -(pTrajectory->TargetCoord.Y);

						const auto theSource = pTechno->GetCoords();
						const auto rotateAngle = Math::atan2(sourceCoord.Y - theSource.Y , sourceCoord.X - theSource.X);
						pTrajectory->SetEngraveDirection(pCreateBullet, rotateAngle);
						auto coordDistance = pCreateBullet->Velocity.Magnitude();
						pCreateBullet->Velocity *= (coordDistance > 1e-10) ? (pTrajType->Speed / coordDistance) : 0;
					}
				}
				else
				{
					pTrajectory->NotMainWeapon = true;
				}
			}
			else if (flag == TrajectoryFlag::Parabola)
			{
				const auto pTrajectory = static_cast<ParabolaTrajectory*>(pTraj);
				const auto pTrajType = pTrajectory->Type;

				// The parabola trajectory bullets has LeadTimeCalculate=true are not calculate its velocity yet.
				if (pTrajectory->UseDisperseBurst && std::abs(pTrajType->RotateCoord) > 1e-10 && curBurst >= 0 && maxBurst > 1)
				{
					if (pTrajType->LeadTimeCalculate && abstract_cast<FootClass*>(pTarget))
					{
						pTrajectory->CurrentBurst = curBurst;
						pTrajectory->CountOfBurst = maxBurst;
						pTrajectory->UseDisperseBurst = false;
					}
					else
					{
						this->PhobosTrajectory::DisperseBurstSubstitution(pCreateBullet, pTrajType->AxisOfRotation.Get(), pTrajType->RotateCoord, curBurst, maxBurst, pTrajType->MirrorCoord);
					}
				}
			}
			else if (flag == TrajectoryFlag::Tracing)
			{
				const auto pTrajectory = static_cast<TracingTrajectory*>(pTraj);
				const auto pTrajType = pTrajectory->Type;
				pTrajectory->FirepowerMult = this->FirepowerMult;

				// Inherit the fire location of the record
				if (pTrajType->UseDisperseCoord && pTechno && this->GetType()->RecordSourceCoord && this->FLHCoord != CoordStruct::Empty)
				{
					pTrajectory->FLHCoord = this->FLHCoord;
					pTrajectory->BuildingCoord = this->BuildingCoord;
					pTrajectory->NotMainWeapon = false;
				}
				else
				{
					pTrajectory->NotMainWeapon = true;
				}
			}
		}

		// Simulate the actual weapon launch effect
		BulletExt::SimulatedFiringEffects(pCreateBullet, pOwner, nullptr, true, true);
	}
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
	this->TargetSnapDistance.Read(exINI, pSection, "Trajectory.TargetSnapDistance");

	this->PassDetonate.Read(exINI, pSection, "Trajectory.PassDetonate");
	this->PassDetonateWarhead.Read<true>(exINI, pSection, "Trajectory.PassDetonateWarhead");
	this->PassDetonateDamage.Read(exINI, pSection, "Trajectory.PassDetonateDamage");
	this->PassDetonateDelay.Read(exINI, pSection, "Trajectory.PassDetonateDelay");
	this->PassDetonateInitialDelay.Read(exINI, pSection, "Trajectory.PassDetonateInitialDelay");
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

	this->RotateCoord.Read(exINI, pSection, "Trajectory.RotateCoord");
	this->OffsetCoord.Read(exINI, pSection, "Trajectory.OffsetCoord");
	this->AxisOfRotation.Read(exINI, pSection, "Trajectory.AxisOfRotation");
	this->UseDisperseBurst.Read(exINI, pSection, "Trajectory.UseDisperseBurst");
	this->LeadTimeCalculate.Read(exINI, pSection, "Trajectory.LeadTimeCalculate");
	this->EarlyDetonation.Read(exINI, pSection, "Trajectory.EarlyDetonation");
	this->DetonationHeight.Read(exINI, pSection, "Trajectory.DetonationHeight");
	this->DetonationDistance.Read(exINI, pSection, "Trajectory.DetonationDistance");

//	this->VirtualSourceCoord.Read(exINI, pSection, "Trajectory.VirtualSourceCoord");
//	this->VirtualTargetCoord.Read(exINI, pSection, "Trajectory.VirtualTargetCoord");
	this->AllowFirerTurning.Read(exINI, pSection, "Trajectory.AllowFirerTurning");
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
		.Process(this->TargetSnapDistance)

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

		.Process(this->RotateCoord)
		.Process(this->OffsetCoord)
		.Process(this->AxisOfRotation)
		.Process(this->UseDisperseBurst)
		.Process(this->LeadTimeCalculate)
		.Process(this->EarlyDetonation)
		.Process(this->DetonationHeight)
		.Process(this->DetonationDistance)

		.Process(this->VirtualSourceCoord)
		.Process(this->VirtualTargetCoord)
		.Process(this->AllowFirerTurning)
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
		.Process(this->DurationTimer)
		.Process(this->TolerantTimer)
		.Process(this->FirepowerMult)
		.Process(this->AttenuationRange)
		.Process(this->FLHCoord)
		.Process(this->BuildingCoord)

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
		.Process(this->TargetInTheAir)
		.Process(this->TargetIsTechno)

		.Process(this->OffsetCoord)
		.Process(this->LastTargetCoord)
		.Process(this->CurrentBurst)
		.Process(this->CountOfBurst)
		.Process(this->WaitOneFrame)
		.Process(this->SubjectToGround)

		.Process(this->VirtualSourceCoord)
		.Process(this->VirtualTargetCoord)
		.Process(this->TechnoInTransport)
		.Process(this->NotMainWeapon)
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
		detonate = pTraj->OnAI(pThis);

	if (detonate && !pThis->SpawnNextAnim)
		return Detonate;

	return 0;
}

DEFINE_HOOK(0x467E53, BulletClass_AI_PreDetonation_Trajectories, 0x6)
{
	GET(BulletClass*, pThis, EBP);

	auto const pExt = BulletExt::ExtMap.Find(pThis);

	if (auto pTraj = pExt->Trajectory.get())
		pTraj->OnAIPreDetonate(pThis);

	return 0;
}

DEFINE_HOOK(0x46745C, BulletClass_AI_Position_Trajectories, 0x7)
{
	GET(BulletClass*, pThis, EBP);
	LEA_STACK(BulletVelocity*, pSpeed, STACK_OFFSET(0x1AC, -0x11C));
	LEA_STACK(BulletVelocity*, pPosition, STACK_OFFSET(0x1AC, -0x144));

	auto const pExt = BulletExt::ExtMap.Find(pThis);

	if (auto pTraj = pExt->Trajectory.get())
		pTraj->OnAIVelocity(pThis, pSpeed, pPosition);

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
		switch (pTraj->OnAITargetCoordCheck(pThis))
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
		switch (pTraj->OnAITechnoCheck(pThis, pTechno))
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
	GET_STACK(CoordStruct*, pCoord, STACK_OFFSET(0x54, 0x4));
	GET_STACK(BulletVelocity*, pVelocity, STACK_OFFSET(0x54, 0x8));

	auto const pExt = BulletExt::ExtMap.Find(pThis);
	auto const pTypeExt = pExt->TypeExtData;

	if (pTypeExt && pTypeExt->TrajectoryType)
	{
		pExt->Trajectory = pTypeExt->TrajectoryType->CreateInstance();
		pExt->Trajectory->OnUnlimbo(pThis, pCoord, pVelocity);
	}

	return 0;
}
