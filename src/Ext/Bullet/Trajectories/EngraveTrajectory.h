#pragma once

#include "PhobosTrajectory.h"
#include "Classification.h"

class EngraveTrajectoryType final : public VirtualTrajectoryType
{
public:
	EngraveTrajectoryType() : VirtualTrajectoryType()
		, SourceCoord { { 0, 0 } }
		, TargetCoord { { 0, 0 } }
		, MirrorCoord { true }
		, UseDisperseCoord { false }
		, ApplyRangeModifiers { false }
		, AllowFirerTurning { true }
		, Duration { 0 }
		, IsLaser { true }
		, IsIntense { false }
		, IsHouseColor { false }
		, IsSingleColor { false }
		, LaserInnerColor { { 0, 0, 0 } }
		, LaserOuterColor { { 0, 0, 0 } }
		, LaserOuterSpread { { 0, 0, 0 } }
		, LaserThickness { 3 }
		, LaserDuration { 1 }
		, LaserDelay { 1 }
		, DamageDelay { 2 }
		, ProximityImpact { 0 }
		, ProximityWarhead {}
		, ProximityDamage { 0 }
		, ProximityRadius { Leptons(179) }
		, ProximityDirect { false }
		, ProximityMedial { false }
		, ProximityAllies { false }
		, ProximityFlight { false }
		, ProximitySuicide { false }
		, ConfineOnGround { true }
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual std::unique_ptr<PhobosTrajectory> CreateInstance(BulletClass* pBullet) const override;
	virtual void Read(CCINIClass* const pINI, const char* pSection) override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Engrave; }

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	Valueable<Point2D> SourceCoord;
	Valueable<Point2D> TargetCoord;
	Valueable<bool> MirrorCoord;
	Valueable<bool> UseDisperseCoord;
	Valueable<bool> ApplyRangeModifiers;
	Valueable<bool> AllowFirerTurning;
	Valueable<int> Duration;
	Valueable<bool> IsLaser;
	Valueable<bool> IsIntense;
	Valueable<bool> IsHouseColor;
	Valueable<bool> IsSingleColor;
	Valueable<ColorStruct> LaserInnerColor;
	Valueable<ColorStruct> LaserOuterColor;
	Valueable<ColorStruct> LaserOuterSpread;
	Valueable<int> LaserThickness;
	Valueable<int> LaserDuration;
	Valueable<int> LaserDelay;
	Valueable<int> DamageDelay;
	Valueable<int> ProximityImpact;
	Valueable<WarheadTypeClass*> ProximityWarhead;
	Valueable<int> ProximityDamage;
	Valueable<Leptons> ProximityRadius;
	Valueable<bool> ProximityDirect;
	Valueable<bool> ProximityMedial;
	Valueable<bool> ProximityAllies;
	Valueable<bool> ProximityFlight;
	Valueable<bool> ProximitySuicide;
	Valueable<bool> ConfineOnGround;
};

class EngraveTrajectory final : public VirtualTrajectory
{
public:
	EngraveTrajectory(noinit_t) { }
	EngraveTrajectory(EngraveTrajectoryType const* trajType, BulletClass* pBullet)
		: VirtualTrajectory(trajType, pBullet)
		, Type { trajType }
		, SourceCoord { trajType->SourceCoord.Get() }
		, TargetCoord { trajType->TargetCoord.Get() }
		, Duration { trajType->Duration }
		, LaserTimer {}
		, DamageTimer {}
		, TechnoInTransport { 0 }
		, NotMainWeapon { false }
		, FLHCoord {}
		, BuildingCoord {}
		, StartCoord {}
		, ProximityImpact { trajType->ProximityImpact }
		, TheCasualty {}
	{ }

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual TrajectoryFlag Flag() const override { return TrajectoryFlag::Engrave; }
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

	void SetEngraveDirection(double rotateAngle);
	void GetTechnoFLHCoord(TechnoClass* pTechno);
	inline void CheckMirrorCoord(TechnoClass* pTechno);
	bool InvalidFireCondition(TechnoClass* pTechno);
	int GetFloorCoordHeight(const CoordStruct& coord);
	bool PlaceOnCorrectHeight();
	void DrawEngraveLaser(TechnoClass* pTechno, HouseClass* pOwner);
	inline void DetonateLaserWarhead(TechnoClass* pTechno, HouseClass* pOwner);
	void PrepareForDetonateAt(HouseClass* pOwner);

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	const EngraveTrajectoryType* Type;
	Point2D SourceCoord;
	Point2D TargetCoord;
	int Duration;
	CDTimerClass LaserTimer;
	CDTimerClass DamageTimer;
	DWORD TechnoInTransport;
	bool NotMainWeapon;
	CoordStruct FLHCoord;
	CoordStruct BuildingCoord;
	CoordStruct StartCoord;
	int ProximityImpact;
	std::map<int, int> TheCasualty; // Only for recording existence
};
