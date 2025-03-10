#pragma once

#include "PhobosTrajectory.h"
#include "Classification.h"

#include <Ext/WeaponType/Body.h>

class MissileTrajectoryType final : public LiveShellTrajectoryType
{
public:
	MissileTrajectoryType() : LiveShellTrajectoryType()
		, UniqueCurve { false }
		, PreAimCoord { { 0, 0, 0 } }
		, RotateCoord { 0 }
		, MirrorCoord { true }
		, FacingCoord { false }
		, ReduceCoord { true }
		, UseDisperseBurst { false }
		, AxisOfRotation { { 0, 0, 1 } }
		, LaunchSpeed { 0 }
		, Acceleration { 10.0 }
		, TurningSpeed { 10.0 }
		, LockDirection { false }
		, CruiseEnable { false }
		, CruiseUnableRange { Leptons(1280) }
		, CruiseAltitude { 800 }
		, CruiseAlongLevel { false }
		, LeadTimeCalculate { true }
		, RecordSourceCoord { false }
		, RetargetAllies { false }
		, RetargetRadius { 0 }
		, TargetSnapDistance { Leptons(128) }
		, SuicideAboveRange { -3 }
		, SuicideShortOfROT { false }
		, SuicideIfNoWeapon { true }
		, Weapons {}
		, WeaponBurst {}
		, WeaponCount { 0 }
		, WeaponDelay { 1 }
		, WeaponInitialDelay { 0 }
		, WeaponEffectiveRange { Leptons(0) }
		, WeaponSeparate { false }
		, WeaponRetarget { false }
		, WeaponLocation { false }
		, WeaponTendency { false }
		, WeaponHolistic { false }
		, WeaponMarginal { false }
		, WeaponToAllies { false }
		, WeaponDoRepeat { false }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual std::unique_ptr<PhobosTrajectory> CreateInstance(BulletClass* pBullet) const override;
	virtual void Read(CCINIClass* const pINI, const char* pSection) override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Missile; }

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	Valueable<bool> UniqueCurve;
	Valueable<CoordStruct> PreAimCoord;
	Valueable<double> RotateCoord;
	Valueable<bool> MirrorCoord;
	Valueable<bool> FacingCoord;
	Valueable<bool> ReduceCoord;
	Valueable<bool> UseDisperseBurst;
	Valueable<CoordStruct> AxisOfRotation;
	Valueable<double> LaunchSpeed;
	Valueable<double> Acceleration;
	Valueable<double> TurningSpeed;
	Valueable<bool> LockDirection;
	Valueable<bool> CruiseEnable;
	Valueable<Leptons> CruiseUnableRange;
	Valueable<int> CruiseAltitude;
	Valueable<bool> CruiseAlongLevel;
	Valueable<bool> LeadTimeCalculate;
	Valueable<bool> RecordSourceCoord;
	Valueable<bool> RetargetAllies;
	Valueable<double> RetargetRadius;
	Valueable<Leptons> TargetSnapDistance;
	Valueable<double> SuicideAboveRange;
	Valueable<bool> SuicideShortOfROT;
	Valueable<bool> SuicideIfNoWeapon;
	ValueableVector<WeaponTypeClass*> Weapons;
	ValueableVector<int> WeaponBurst;
	Valueable<int> WeaponCount;
	Valueable<int> WeaponDelay;
	Valueable<int> WeaponInitialDelay;
	Valueable<Leptons> WeaponEffectiveRange;
	Valueable<bool> WeaponSeparate;
	Valueable<bool> WeaponRetarget;
	Valueable<bool> WeaponLocation;
	Valueable<bool> WeaponTendency;
	Valueable<bool> WeaponHolistic;
	Valueable<bool> WeaponMarginal;
	Valueable<bool> WeaponToAllies;
	Valueable<bool> WeaponDoRepeat;
};

class MissileTrajectory final : public LiveShellTrajectory
{
public:
	MissileTrajectory(noinit_t) { }
	MissileTrajectory(MissileTrajectoryType const* trajType, BulletClass* pBullet)
		: LiveShellTrajectory(trajType, pBullet)
		, Type { trajType }
		, Speed { trajType->LaunchSpeed }
		, PreAimCoord { trajType->PreAimCoord.Get() }
		, UseDisperseBurst { trajType->UseDisperseBurst }
		, CruiseEnable { trajType->CruiseEnable }
		, SuicideAboveRange { 0 }
		, WeaponCount { trajType->WeaponCount }
		, WeaponTimer {}
		, InStraight { false }
		, Accelerate { true }
		, TargetInTheAir { false }
		, TargetIsTechno { false }
		, OriginalDistance { 0 }
		, CurrentBurst { 0 }
		, ThisWeaponIndex { 0 }
		, LastTargetCoord {}
		, PreAimDistance { 0 }
		, LastDotProduct { 0 }
		, FLHCoord {}
		, BuildingCoord {}
		, FirepowerMult { 1.0 }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Missile; }
	virtual void OnUnlimbo(BulletClass* pBullet) override;
	virtual bool OnAI(BulletClass* pBullet) override;
	virtual bool OnAIPreCheck(BulletClass* pBullet, HouseClass* pOwner) override;
	virtual void OnAIVelocityCheck(BulletClass* pBullet, HouseClass* pOwner) override;
	virtual void OnAILastCheck(BulletClass* pBullet, HouseClass* pOwner) override;
	virtual void OnAIPreDetonate(BulletClass* pBullet) override;
	virtual void OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition) override;
	virtual const PhobosTrajectoryType* GetType() const override { return this->Type; }
	virtual bool OpenFire(BulletClass* pBullet) override;
	virtual CoordStruct GetRetargetCenter(const BulletClass* const pBullet) const override;
	virtual void SetBulletNewTarget(BulletClass* const pBullet, AbstractClass* const pTarget) override;
	virtual bool CalculateBulletVelocity(BulletClass* pBullet) override;

	void InitializeBulletNotCurve(BulletClass* pBullet);
	inline bool CalculateReducedVelocity(BulletClass* pBullet, double rotateAngle);
	bool CurveVelocityChange(BulletClass* pBullet);
	bool NotCurveVelocityChange(BulletClass* pBullet);
	bool StandardVelocityChange(BulletClass* pBullet);
	bool ChangeBulletVelocity(BulletClass* pBullet, const CoordStruct& targetLocation);

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	const MissileTrajectoryType* Type;
	double Speed;
	CoordStruct PreAimCoord;
	bool UseDisperseBurst;
	bool CruiseEnable;
	double SuicideAboveRange;
	int WeaponCount;
	CDTimerClass WeaponTimer;
	bool InStraight;
	bool Accelerate;
	bool TargetInTheAir;
	bool TargetIsTechno;
	int OriginalDistance;
	int CurrentBurst;
	int ThisWeaponIndex;
	CoordStruct LastTargetCoord;
	double PreAimDistance;
	double LastDotProduct;
	CoordStruct FLHCoord;
	CoordStruct BuildingCoord;
	double FirepowerMult;
};
