#pragma once

#include "PhobosTrajectory.h"
#include "Classification.h"

enum class TraceTargetMode : int
{
	Connection = 0,
	Global = 1,
	Body = 2,
	Turret = 3,
	RotateCW = 4,
	RotateCCW = 5,
};

class TracingTrajectoryType final : public VirtualTrajectoryType
{
public:
	TracingTrajectoryType() : VirtualTrajectoryType()
		, TraceMode { TraceTargetMode::Connection }
		, TheDuration { 0 }
		, TolerantTime { -1 }
		, ROT { -1 }
		, BulletSpin { false }
		, PeacefulVanish { false }
		, TraceTheTarget { true }
		, CreateAtTarget { false }
		, CreateCoord { { 0, 0, 0 } }
		, OffsetCoord { { 0, 0, 0 } }
		, WeaponCoord { { 0, 0, 0 } }
		, UseDisperseCoord { false }
		, AllowFirerTurning { true }
		, WeaponFromFirer { true }
		, Weapons {}
		, WeaponCount {}
		, WeaponDelay {}
		, WeaponInitialDelay { 0 }
		, WeaponCycle { -1 }
		, WeaponCheck { false }
		, Synchronize { true }
		, SuicideAboveRange { false }
		, SuicideIfNoWeapon { false }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual std::unique_ptr<PhobosTrajectory> CreateInstance(BulletClass* pBullet) const override;
	virtual void Read(CCINIClass* const pINI, const char* pSection) override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Tracing; }

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	Valueable<TraceTargetMode> TraceMode;
	Valueable<int> TheDuration;
	Valueable<int> TolerantTime;
	Valueable<int> ROT;
	Valueable<bool> BulletSpin;
	Valueable<bool> PeacefulVanish;
	Valueable<bool> TraceTheTarget;
	Valueable<bool> CreateAtTarget;
	Valueable<CoordStruct> CreateCoord;
	Valueable<CoordStruct> OffsetCoord;
	Valueable<CoordStruct> WeaponCoord;
	Valueable<bool> UseDisperseCoord;
	Valueable<bool> AllowFirerTurning;
	Valueable<bool> WeaponFromFirer;
	ValueableVector<WeaponTypeClass*> Weapons;
	ValueableVector<int> WeaponCount;
	ValueableVector<int> WeaponDelay;
	Valueable<int> WeaponInitialDelay;
	Valueable<int> WeaponCycle;
	Valueable<bool> WeaponCheck;
	Valueable<bool> Synchronize;
	Valueable<bool> SuicideAboveRange;
	Valueable<bool> SuicideIfNoWeapon;
};

class TracingTrajectory final : public VirtualTrajectory
{
public:
	TracingTrajectory(noinit_t) { }
	TracingTrajectory(TracingTrajectoryType const* trajType, BulletClass* pBullet)
		: VirtualTrajectory(trajType, pBullet)
		, Type { trajType }
		, WeaponIndex { 0 }
		, WeaponCount { 0 }
		, WeaponCycle { trajType->WeaponCycle }
		, ExistTimer {}
		, WeaponTimer {}
		, TolerantTimer {}
		, TechnoInTransport { 0 }
		, NotMainWeapon { false }
		, FLHCoord {}
		, BuildingCoord {}
		, FirepowerMult { 1.0 }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Tracing; }
	virtual void OnUnlimbo() override;
	virtual bool OnAI() override;
	virtual bool OnAIDetonateCheck() override;
	virtual void OnAIVelocityCheck() override;
	virtual void OnAINextFrameCheck() override;
	virtual void OnAIPreDetonate() override;
	virtual void OnAIVelocity(BulletVelocity* pSpeed, BulletVelocity* pPosition) override;
	virtual const PhobosTrajectoryType* GetType() const override { return this->Type; }
	virtual bool OpenFire() override;
	virtual bool GetCanHitGround() const override { return false; }
	virtual CoordStruct GetRetargetCenter() const override { return this->Bullet->Location; }

	void GetTechnoFLHCoord(TechnoClass* pTechno);
	void SetSourceLocation();
	void InitializeDuration(int duration);
	bool InvalidFireCondition(TechnoClass* pTechno);
	bool BulletDetonatePreCheck();
	void ChangeFacing();
	bool CheckFireFacing();
	BulletVelocity ChangeVelocity();
	AbstractClass* GetBulletTarget(TechnoClass* pTechno, HouseClass* pOwner, WeaponTypeClass* pWeapon);
	CoordStruct GetWeaponFireCoord(TechnoClass* pTechno);
	bool PrepareTracingWeapon();
	void CreateTracingBullets(WeaponTypeClass* pWeapon);

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	const TracingTrajectoryType* Type;
	int WeaponIndex;
	int WeaponCount;
	int WeaponCycle;
	CDTimerClass ExistTimer;
	CDTimerClass WeaponTimer;
	CDTimerClass TolerantTimer;
	DWORD TechnoInTransport;
	bool NotMainWeapon;
	CoordStruct FLHCoord;
	CoordStruct BuildingCoord;
	double FirepowerMult;
};
