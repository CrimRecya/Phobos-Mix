#pragma once

#include "PhobosTrajectory.h"
#include "Classification.h"

class SampleTrajectoryType final : public PhobosTrajectoryType
{
public:
	SampleTrajectoryType() : PhobosTrajectoryType()
		, TargetSnapDistance { Leptons(128) }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual std::unique_ptr<PhobosTrajectory> CreateInstance() const override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Invalid; } // TrajectoryFlag
	virtual void Read(CCINIClass* const pINI, const char* pSection) override;

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	Valueable<Leptons> TargetSnapDistance;
};

class SampleTrajectory final : public PhobosTrajectory
{
public:
	SampleTrajectory(SampleTrajectoryType const* trajType) : PhobosTrajectory(trajType)
		, Type { trajType }
		, TargetSnapDistance { trajType->TargetSnapDistance }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Invalid; } // TrajectoryFlag
	virtual void OnUnlimbo(BulletClass* pBullet) override;
	virtual bool OnAI(BulletClass* pBullet) override;
	virtual bool OnAIPreCheck(BulletClass* pBullet, HouseClass* pOwner) override { return true; }
	virtual void OnAIVelocityCheck(BulletClass* pBullet, HouseClass* pOwner) override {};
	virtual void OnAILastCheck(BulletClass* pBullet, HouseClass* pOwner) override {};
	virtual void OnAIPreDetonate(BulletClass* pBullet) override;
	virtual void OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition) override;
	virtual TrajectoryCheckReturnType OnAITargetCoordCheck(BulletClass* pBullet) override;
	virtual TrajectoryCheckReturnType OnAITechnoCheck(BulletClass* pBullet, TechnoClass* pTechno) override;
	virtual const PhobosTrajectoryType* GetType() const override { return this->Type; }
	virtual bool OpenFire(BulletClass* pBullet) override;
	virtual bool GetCanHitGround() const override { return true; }
	virtual CoordStruct GetRetargetCenter(const BulletClass* const pBullet) const override { return pBullet->TargetCoords; }
	virtual void SetBulletNewTarget(BulletClass* const pBullet, AbstractClass* const pTarget) override;

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	SampleTrajectoryType const* Type;
	Leptons TargetSnapDistance;
};
