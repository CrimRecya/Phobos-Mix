#pragma once

#include "PhobosTrajectory.h"

class LiveShellTrajectoryType : public PhobosTrajectoryType
{
public:
	LiveShellTrajectoryType() : PhobosTrajectoryType()
		, RotateCoord { 0 }
		, OffsetCoord { { 0, 0, 0 } }
		, AxisOfRotation { { 0, 0, 1 } }
		, LeadTimeCalculate { false }
		, SubjectToGround { false }
		, EarlyDetonation { false }
		, DetonationHeight { -1 }
		, DetonationDistance { Leptons(102) }
		, TargetSnapDistance { Leptons(128) }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
//	virtual void Read(CCINIClass* const pINI, const char* pSection) override; // Read separately

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	Valueable<double> RotateCoord;
	Valueable<PartialVector3D<int>> OffsetCoord;
	Valueable<PartialVector3D<int>> AxisOfRotation;
	Valueable<bool> LeadTimeCalculate;
	bool SubjectToGround; // Auto set
	Valueable<bool> EarlyDetonation;
	Valueable<int> DetonationHeight;
	Valueable<Leptons> DetonationDistance;
	Valueable<Leptons> TargetSnapDistance;
};

class LiveShellTrajectory : public PhobosTrajectory
{
public:
	LiveShellTrajectory() { }
	LiveShellTrajectory(LiveShellTrajectoryType const* trajType, BulletClass* pBullet)
		: PhobosTrajectory(trajType, pBullet)
		, LastTargetCoord {}
		, WaitOneFrame { 0 }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual void OnUnlimbo(BulletClass* pBullet) override;
	virtual void OnAIPreDetonate(BulletClass* pBullet) override;

	static BulletVelocity RotateAboutTheAxis(const BulletVelocity& theSpeed, BulletVelocity& theAxis, double theRadian);

	CoordStruct GetOnlyStableOffsetCoords(double rotateRadian);
	CoordStruct GetInaccurateTargetCoords(BulletClass* pBullet, const CoordStruct& baseCoord, double distance);
	void DisperseBurstSubstitution(BulletClass* pBullet, double baseRadian);

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	CoordStruct LastTargetCoord;
	int WaitOneFrame;
};

class VirtualTrajectoryType : public PhobosTrajectoryType
{
public:
	VirtualTrajectoryType() : PhobosTrajectoryType()
		, VirtualSourceCoord { { 0, 0, 0 } }
		, VirtualTargetCoord { { 0, 0, 0 } }
		, AllowFirerTurning { true }
		, IgnoresFirestorm { true }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
//	virtual void Read(CCINIClass* const pINI, const char* pSection) override; // Read separately

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	Valueable<PartialVector3D<int>> VirtualSourceCoord;
	Valueable<PartialVector3D<int>> VirtualTargetCoord;
	Valueable<bool> AllowFirerTurning;
	bool IgnoresFirestorm; // Auto set
};

class VirtualTrajectory : public PhobosTrajectory
{
public:
	VirtualTrajectory() { }
	VirtualTrajectory(VirtualTrajectoryType const* trajType, BulletClass* pBullet)
		: PhobosTrajectory(trajType, pBullet)
		, SurfaceFirerID { 0 }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual void OnUnlimbo(BulletClass* pBullet) override;

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	DWORD SurfaceFirerID;
};
