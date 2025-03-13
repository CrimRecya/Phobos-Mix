#pragma once

#include "PhobosTrajectory.h"

class SampleTrajectoryType final : public PhobosTrajectoryType
{
public:
	SampleTrajectoryType() : PhobosTrajectoryType()
		, TargetSnapDistance { Leptons(128) }
	{ }

	Valueable<Leptons> TargetSnapDistance;

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual std::unique_ptr<PhobosTrajectory> CreateInstance(BulletClass* pBullet) const override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Invalid; } // TrajectoryFlag
	virtual void Read(CCINIClass* const pINI, const char* pSection) override;

private:
	template <typename T>
	void Serialize(T& Stm);
};

class SampleTrajectory final : public PhobosTrajectory
{
public:
	SampleTrajectory(noinit_t) { }
	SampleTrajectory(SampleTrajectoryType const* trajType, BulletClass* pBullet)
		: PhobosTrajectory(trajType, pBullet)
		, Type { trajType }
		, TargetSnapDistance { trajType->TargetSnapDistance }
	{ }

	SampleTrajectoryType const* Type;
	Leptons TargetSnapDistance;

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Invalid; } // TrajectoryFlag
	virtual void OnUnlimbo() override;
	virtual bool OnAI() override;
	virtual bool OnAIDetonateCheck() override;
	virtual void OnAIVelocityCheck() override;
	virtual void OnAINextFrameCheck() override;
	virtual void OnAIPreDetonate() override;
	virtual void OnAIVelocity(BulletVelocity* pSpeed, BulletVelocity* pPosition) override;
	virtual TrajectoryCheckReturnType OnAITargetCoordCheck() override;
	virtual TrajectoryCheckReturnType OnAITechnoCheck(TechnoClass* pTechno) override;
	virtual const PhobosTrajectoryType* GetType() const override { return this->Type; }
	virtual void OpenFire() override;
	virtual bool GetCanHitGround() const override;
	virtual CoordStruct GetRetargetCenter() const override;
	virtual void SetBulletNewTarget(AbstractClass* const pTarget) override;
	virtual bool CalculateBulletVelocity(const double speed) override;

private:
	template <typename T>
	void Serialize(T& Stm);
};
