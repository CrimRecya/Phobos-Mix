#pragma once

#include "PhobosTrajectory.h"

class LiveShellTrajectoryType : public PhobosTrajectoryType
{
public:
	LiveShellTrajectoryType() : PhobosTrajectoryType()
	{ }
/*
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual void Read(CCINIClass* const pINI, const char* pSection) override;
*/
private:
	template <typename T>
	void Serialize(T& Stm);

public:
	;
};

class LiveShellTrajectory : public PhobosTrajectory
{
public:
	LiveShellTrajectory(LiveShellTrajectoryType const* trajType) : PhobosTrajectory(trajType)
	{ }
/*
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual void OnUnlimbo(BulletClass* pBullet, CoordStruct* pCoord, BulletVelocity* pVelocity) override;
	virtual bool OnAI(BulletClass* pBullet) override;
	virtual bool OnAIPreCheck(BulletClass* pBullet, HouseClass* pOwner) override;
	virtual void OnAIVelocityCheck(BulletClass* pBullet, HouseClass* pOwner) override;
	virtual void OnAILastCheck(BulletClass* pBullet, HouseClass* pOwner) override;
	virtual void OnAIPreDetonate(BulletClass* pBullet) override;
	virtual void OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition) override;
*/
private:
	template <typename T>
	void Serialize(T& Stm);

public:
	;
};

class VirtualTrajectoryType : public PhobosTrajectoryType
{
public:
	VirtualTrajectoryType() : PhobosTrajectoryType()
	{ }
/*
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual void Read(CCINIClass* const pINI, const char* pSection) override;
*/
private:
	template <typename T>
	void Serialize(T& Stm);

public:
	;
};

class VirtualTrajectory : public PhobosTrajectory
{
public:
	VirtualTrajectory(VirtualTrajectoryType const* trajType) : PhobosTrajectory(trajType)
	{ }
/*
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual void OnUnlimbo(BulletClass* pBullet, CoordStruct* pCoord, BulletVelocity* pVelocity) override;
	virtual bool OnAI(BulletClass* pBullet) override;
	virtual bool OnAIPreCheck(BulletClass* pBullet, HouseClass* pOwner) override;
	virtual void OnAIVelocityCheck(BulletClass* pBullet, HouseClass* pOwner) override;
	virtual void OnAILastCheck(BulletClass* pBullet, HouseClass* pOwner) override;
	virtual void OnAIPreDetonate(BulletClass* pBullet) override;
	virtual void OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition) override;
*/
private:
	template <typename T>
	void Serialize(T& Stm);

public:
	;
};
