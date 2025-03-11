#include "PhobosTrajectory.h"
#include "StraightTrajectory.h"
#include "BombardTrajectory.h"
#include "MissileTrajectory.h"
#include "EngraveTrajectory.h"
#include "ParabolaTrajectory.h"
#include "TracingTrajectory.h"

#include <Ext/Bullet/Body.h>

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
		.Process(this->MovingVelocity)
		.Process(this->DurationTimer)
		.Process(this->TolerantTimer)
		.Process(this->FirepowerMult)
		.Process(this->AttenuationRange)
		.Process(this->RemainingDistance)
		.Process(this->TargetInTheAir)
		.Process(this->TargetIsTechno)
		.Process(this->NotMainWeapon)
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
