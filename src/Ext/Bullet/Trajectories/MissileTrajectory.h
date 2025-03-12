#pragma once

#include "PhobosTrajectory.h"

#include <Ext/WeaponType/Body.h>

class MissileTrajectoryType final : public LiveShellTrajectoryType
{
public:
	MissileTrajectoryType() : LiveShellTrajectoryType()
		, UniqueCurve { false }
		, FacingCoord { false }
		, ReduceCoord { true }
		, LaunchSpeed { 0 }
		, Acceleration { 10.0 }
		, TurningSpeed { 10.0 }
		, LockDirection { false }
		, CruiseEnable { false }
		, CruiseUnableRange { Leptons(1280) }
		, CruiseAltitude { 800 }
		, CruiseAlongLevel { false }
		, SuicideAboveRange { -3 }
		, SuicideShortOfROT { false }
	{ }

	Valueable<bool> UniqueCurve;
	Valueable<bool> FacingCoord;
	Valueable<bool> ReduceCoord;
	Valueable<double> LaunchSpeed;
	Valueable<double> Acceleration;
	Valueable<double> TurningSpeed;
	Valueable<bool> LockDirection;
	Valueable<bool> CruiseEnable;
	Valueable<Leptons> CruiseUnableRange;
	Valueable<int> CruiseAltitude;
	Valueable<bool> CruiseAlongLevel;
	Valueable<double> SuicideAboveRange;
	Valueable<bool> SuicideShortOfROT;

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual std::unique_ptr<PhobosTrajectory> CreateInstance(BulletClass* pBullet) const override;
	virtual void Read(CCINIClass* const pINI, const char* pSection) override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Missile; }

private:
	template <typename T>
	void Serialize(T& Stm);
};

class MissileTrajectory final : public LiveShellTrajectory
{
public:
	MissileTrajectory(noinit_t) { }
	MissileTrajectory(MissileTrajectoryType const* trajType, BulletClass* pBullet)
		: LiveShellTrajectory(trajType, pBullet)
		, Type { trajType }
		, CruiseEnable { trajType->CruiseEnable }
		, InStraight { false }
		, Accelerate { true }
		, OriginalDistance { 0 }
		, PreAimDistance { 0 }
		, LastDotProduct { 0 }
	{ }

	const MissileTrajectoryType* Type;
	bool CruiseEnable;
	bool InStraight;
	bool Accelerate;
	int OriginalDistance;
	double PreAimDistance;
	double LastDotProduct;

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Missile; }
	virtual void OnUnlimbo() override;
	virtual bool OnAI() override;
	virtual bool OnAIDetonateCheck() override;
	virtual void OnAIVelocityCheck() override;
	virtual const PhobosTrajectoryType* GetType() const override { return this->Type; }
	virtual void OpenFire() override;
	virtual CoordStruct GetRetargetCenter() const override;
	virtual void SetBulletNewTarget(AbstractClass* const pTarget) override;
	virtual bool CalculateBulletVelocity(const double speed) override;

private:
	void InitializeBulletNotCurve();
	inline bool CalculateReducedVelocity(double rotateRadian);
	bool CurveVelocityChange();
	bool NotCurveVelocityChange();
	bool StandardVelocityChange();
	bool ChangeBulletVelocity(const CoordStruct& targetLocation);

	template <typename T>
	void Serialize(T& Stm);
};
