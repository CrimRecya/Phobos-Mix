#pragma once

#include "PhobosTrajectory.h"
#include "Classification.h"

enum class ParabolaFireMode : int
{
	Speed = 0,
	Height = 1,
	Angle = 2,
	SpeedAndHeight = 3,
	HeightAndAngle = 4,
	SpeedAndAngle = 5,
};

class ParabolaTrajectoryType final : public LiveShellTrajectoryType
{
public:
	ParabolaTrajectoryType() : LiveShellTrajectoryType()
		, OpenFireMode { ParabolaFireMode::Speed }
		, ThrowHeight { 600 }
		, LaunchAngle { 30.0 }
		, DetonationAngle { -90.0 }
		, BounceTimes { 0 }
		, BounceOnWater { false }
		, BounceDetonate { false }
		, BounceAttenuation { 0.8 }
		, BounceCoefficient { 0.8 }
	{ }

	Valueable<ParabolaFireMode> OpenFireMode;
	Valueable<int> ThrowHeight;
	Valueable<double> LaunchAngle;
	Valueable<double> DetonationAngle;
	Valueable<int> BounceTimes;
	Valueable<bool> BounceOnWater;
	Valueable<bool> BounceDetonate;
	Valueable<double> BounceAttenuation;
	Valueable<double> BounceCoefficient;

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual std::unique_ptr<PhobosTrajectory> CreateInstance(BulletClass* pBullet) const override;
	virtual void Read(CCINIClass* const pINI, const char* pSection) override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Parabola; }

private:
	template <typename T>
	void Serialize(T& Stm);
};

class ParabolaTrajectory final : public LiveShellTrajectory
{
public:
	ParabolaTrajectory(noinit_t) { }
	ParabolaTrajectory(ParabolaTrajectoryType const* trajType, BulletClass* pBullet)
		: LiveShellTrajectory(trajType, pBullet)
		, Type { trajType }
		, ThrowHeight { trajType->ThrowHeight > 0 ? trajType->ThrowHeight : 600 }
		, BounceTimes { trajType->BounceTimes }
		, ShouldBounce { false }
		, LastVelocity {}
	{ }

	const ParabolaTrajectoryType* Type;
	int ThrowHeight;
	int BounceTimes;
	bool ShouldBounce;
	BulletVelocity LastVelocity;

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Parabola; }
	virtual void OnUnlimbo() override;
	virtual bool OnAIDetonateCheck() override;
	virtual void OnAIVelocityCheck() override;
	virtual void OnAIPreDetonate() override;
	virtual const PhobosTrajectoryType* GetType() const override { return this->Type; }
	virtual bool OpenFire() override;
	virtual void FireTrajectory() override;
	virtual void MultiplyBulletVelocity(const double ratio, const bool shouldDetonate) override;

private:
	void CalculateBulletVelocityRightNow(const CoordStruct& pSourceCoords, double gravity);
	void CalculateBulletVelocityLeadTime(const CoordStruct& pSourceCoords, double gravity);
	double SearchVelocity(double horizontalDistance, int distanceCoordsZ, double radian, double gravity);
	double CheckVelocityEquation(double horizontalDistance, int distanceCoordsZ, double velocity, double radian, double gravity);
	double SolveFixedSpeedMeetTime(const CoordStruct& pSourceCrd, const CoordStruct& pTargetCrd, const CoordStruct& pOffsetCrd, double horizontalSpeed);
	double SearchFixedHeightMeetTime(const CoordStruct& pSourceCrd, const CoordStruct& pTargetCrd, const CoordStruct& pOffsetCrd, double gravity);
	double CheckFixedHeightEquation(const CoordStruct& pSourceCrd, const CoordStruct& pTargetCrd, const CoordStruct& pOffsetCrd, double meetTime, double gravity);
	double SearchFixedAngleMeetTime(const CoordStruct& pSourceCrd, const CoordStruct& pTargetCrd, const CoordStruct& pOffsetCrd, double radian, double gravity);
	double CheckFixedAngleEquation(const CoordStruct& pSourceCrd, const CoordStruct& pTargetCrd, const CoordStruct& pOffsetCrd, double meetTime, double radian, double gravity);
	bool CalculateBulletVelocityAfterBounce(CellClass* pCell);
	BulletVelocity GetGroundNormalVector(CellClass* pCell);
	static bool CheckBulletHitCliff(short X, short Y, int bulletHeight, int lastCellHeight);

	template <typename T>
	void Serialize(T& Stm);
};
