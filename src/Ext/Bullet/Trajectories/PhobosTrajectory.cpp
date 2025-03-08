#include <Ext/BulletType/Body.h>
#include <Ext/Bullet/Body.h>
#include <Ext/WeaponType/Body.h>

#include <BulletClass.h>
#include <Helpers/Macro.h>

#include "StraightTrajectory.h"
#include "BombardTrajectory.h"
#include "MissileTrajectory.h"
#include "EngraveTrajectory.h"
#include "ParabolaTrajectory.h"
#include "TracingTrajectory.h"

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

// A rectangular shape with a custom width from the current frame to the next frame in length.
std::vector<CellClass*> PhobosTrajectoryType::GetCellsInProximityRadius(const BulletClass* const pBullet, const Leptons trajectoryProximityRange)
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

	std::vector<CellStruct> recCells = PhobosTrajectoryType::GetCellsInRectangle(cor1Cell, cor4Cell, cor2Cell, cor3Cell);
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
std::vector<CellStruct> PhobosTrajectoryType::GetCellsInRectangle(const CellStruct bottomStaCell, const CellStruct leftMidCell, const CellStruct rightMidCell, const CellStruct topEndCell)
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
	this->RetargetRadius.Read(exINI, pSection, "Trajectory.RetargetRadius");
	this->Synchronize.Read(exINI, pSection, "Trajectory.Synchronize");
	this->PeacefulVanish.Read(exINI, pSection, "Trajectory.PeacefulVanish");
	this->ApplyRangeModifiers.Read(exINI, pSection, "Trajectory.ApplyRangeModifiers");
	this->TargetSnapDistance.Read(exINI, pSection, "Trajectory.TargetSnapDistance");
	this->UseDisperseCoord.Read(exINI, pSection, "Trajectory.UseDisperseCoord");
	this->RecordSourceCoord.Read(exINI, pSection, "Trajectory.RecordSourceCoord");

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
	this->EdgeAttenuation.Read(exINI, pSection, "Trajectory.EdgeAttenuation");
	this->EdgeAttenuation = Math::max(0.0, this->EdgeAttenuation);
	this->CountAttenuation.Read(exINI, pSection, "Trajectory.CountAttenuation");
	this->CountAttenuation = Math::max(0.0, this->CountAttenuation);

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
		.Process(this->RetargetRadius)
		.Process(this->Synchronize)
		.Process(this->PeacefulVanish)
		.Process(this->ApplyRangeModifiers)
		.Process(this->TargetSnapDistance)
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
		.Process(this->EdgeAttenuation)
		.Process(this->CountAttenuation)

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
		.Process(this->Duration)
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
