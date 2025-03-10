#pragma once

#include "PhobosTrajectory.h"
#include "Classification.h"

class StraightTrajectoryType final : public LiveShellTrajectoryType
{
public:
	StraightTrajectoryType() : LiveShellTrajectoryType()
		, PassThrough { false }
		, ConfineAtHeight { 0 }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual std::unique_ptr<PhobosTrajectory> CreateInstance(BulletClass* pBullet) const override;
	virtual void Read(CCINIClass* const pINI, const char* pSection) override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Straight; }

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	Valueable<bool> PassThrough;
	Valueable<int> ConfineAtHeight;
};

class StraightTrajectory final : public LiveShellTrajectory
{
public:
	StraightTrajectory(noinit_t) { }
	StraightTrajectory(StraightTrajectoryType const* trajType, BulletClass* pBullet)
		: LiveShellTrajectory(trajType, pBullet)
		, Type { trajType }
		, DetonationDistance { trajType->DetonationDistance }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Straight; }
	virtual void OnUnlimbo(BulletClass* pBullet) override;
	virtual bool OnAI(BulletClass* pBullet) override;
	virtual void OnAIPreDetonate(BulletClass* pBullet) override;
	virtual const PhobosTrajectoryType* GetType() const override { return this->Type; }
	virtual bool OpenFire(BulletClass* pBullet) override;
	virtual bool GetCanHitGround() const override { return this->Type->SubjectToGround; }
	virtual CoordStruct GetRetargetCenter(const BulletClass* const pBullet) const override { return pBullet->TargetCoords; }
	virtual void SetBulletNewTarget(BulletClass* const pBullet, AbstractClass* const pTarget) override;

	void PrepareForOpenFire(BulletClass* pBullet);
	int GetVelocityZ(BulletClass* pBullet);
	bool BulletPrepareCheck(BulletClass* pBullet);
	bool PassAndConfineAtHeight(BulletClass* pBullet);

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	const StraightTrajectoryType* Type;
	Leptons DetonationDistance;
};
