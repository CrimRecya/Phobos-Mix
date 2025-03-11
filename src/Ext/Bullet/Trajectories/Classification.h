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

	Valueable<double> RotateCoord; // The maximum rotation angle of the initial velocity vector on the axis of rotation
	Valueable<PartialVector3D<int>> OffsetCoord; // Offset of target position, refers to the initial target position on Missile
	Valueable<PartialVector3D<int>> AxisOfRotation; // RotateCoord's rotation axis
	Valueable<bool> LeadTimeCalculate; // Predict the moving direction of the target
	bool SubjectToGround; // Auto set
	Valueable<bool> EarlyDetonation; // Calculating DetonationHeight in the rising phase rather than the falling phase
	Valueable<int> DetonationHeight; // At what height did it detonate in advance
	Valueable<Leptons> DetonationDistance; // Explode at a distance from the target, different on AAA and BBB
	Valueable<Leptons> TargetSnapDistance; // Snap to target when detonating with a distance less than this

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
//	virtual void Read(CCINIClass* const pINI, const char* pSection) override; // Read separately

private:
	template <typename T>
	void Serialize(T& Stm);
};

class LiveShellTrajectory : public PhobosTrajectory
{
public:
	LiveShellTrajectory() { }
	LiveShellTrajectory(LiveShellTrajectoryType const* trajType, BulletClass* pBullet)
		: PhobosTrajectory(trajType, pBullet)
		, LastTargetCoord { CoordStruct::Empty }
		, WaitOneFrame { 0 }
	{ }

	CoordStruct LastTargetCoord; // The target is located in the previous frame, used to calculate the lead time
	int WaitOneFrame; // Attempts to launch when update

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual void OnUnlimbo() override;
	virtual bool OnAI() override;
	virtual void OnAIPreDetonate() override;
	virtual void FireTrajectory() { this->OpenFire(); } // New

	static BulletVelocity RotateAboutTheAxis(const BulletVelocity& theSpeed, BulletVelocity& theAxis, double theRadian);

	bool BulletPrepareCheck();
	CoordStruct GetOnlyStableOffsetCoords(double rotateRadian);
	CoordStruct GetInaccurateTargetCoords(const CoordStruct& baseCoord, double distance);
	void DisperseBurstSubstitution(double baseRadian);

private:
	template <typename T>
	void Serialize(T& Stm);
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

	Valueable<PartialVector3D<int>> VirtualSourceCoord; // Initial location of the projectile
	Valueable<PartialVector3D<int>> VirtualTargetCoord; // move to location of the projectile
	Valueable<bool> AllowFirerTurning; // Allow firer not facing projectiles
	bool IgnoresFirestorm; // Auto set

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
//	virtual void Read(CCINIClass* const pINI, const char* pSection) override; // Read separately

private:
	template <typename T>
	void Serialize(T& Stm);
};

class VirtualTrajectory : public PhobosTrajectory
{
public:
	VirtualTrajectory() { }
	VirtualTrajectory(VirtualTrajectoryType const* trajType, BulletClass* pBullet)
		: PhobosTrajectory(trajType, pBullet)
		, SurfaceFirerID { 0 }
	{ }

	DWORD SurfaceFirerID; // UniqueID of the "launcher"

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual void OnUnlimbo() override;
	virtual bool OnAI() override;
	virtual bool OnAIDetonateCheck() override;

	bool InvalidFireCondition(TechnoClass* pTechno);

private:
	template <typename T>
	void Serialize(T& Stm);
};
