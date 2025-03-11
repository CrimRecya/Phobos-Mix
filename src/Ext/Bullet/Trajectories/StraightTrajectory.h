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
	virtual void OnUnlimbo() override;
	virtual bool OnAI() override;
	virtual void OnAIPreDetonate() override;
	virtual const PhobosTrajectoryType* GetType() const override { return this->Type; }
	virtual bool OpenFire() override;
	virtual bool GetCanHitGround() const override { return this->Type->SubjectToGround; }

	void PrepareForOpenFire();
	int GetVelocityZ();
	bool BulletPrepareCheck();
	bool PassAndConfineAtHeight();

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	const StraightTrajectoryType* Type;
	Leptons DetonationDistance;
};
