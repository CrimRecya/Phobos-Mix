#include "EngraveTrajectory.h"

#include <LaserDrawClass.h>

#include <Ext/WeaponType/Body.h>
#include <Ext/Bullet/Body.h>
#include <Ext/Techno/Body.h>

std::unique_ptr<PhobosTrajectory> EngraveTrajectoryType::CreateInstance(BulletClass* pBullet) const
{
	return std::make_unique<EngraveTrajectory>(this, pBullet);
}

template<typename T>
void EngraveTrajectoryType::Serialize(T& Stm)
{
	Stm
		.Process(this->IsLaser)
		.Process(this->IsIntense)
		.Process(this->IsHouseColor)
		.Process(this->IsSingleColor)
		.Process(this->LaserInnerColor)
		.Process(this->LaserOuterColor)
		.Process(this->LaserOuterSpread)
		.Process(this->LaserThickness)
		.Process(this->LaserDuration)
		.Process(this->LaserDelay)
		;
}

bool EngraveTrajectoryType::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->VirtualTrajectoryType::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool EngraveTrajectoryType::Save(PhobosStreamWriter& Stm) const
{
	this->VirtualTrajectoryType::Save(Stm);
	const_cast<EngraveTrajectoryType*>(this)->Serialize(Stm);
	return true;
}

void EngraveTrajectoryType::Read(CCINIClass* const pINI, const char* pSection)
{
	this->PhobosTrajectoryType::Read(pINI, pSection);
	INI_EX exINI(pINI);

	this->Speed = Math::min(128.0, this->Speed);

	// Virtual
	this->VirtualSourceCoord.Read(exINI, pSection, "Trajectory.Engrave.SourceCoord");
	this->VirtualTargetCoord.Read(exINI, pSection, "Trajectory.Engrave.TargetCoord");
	this->AllowFirerTurning.Read(exINI, pSection, "Trajectory.Engrave.AllowFirerTurning");

	// Engrave
	this->IsLaser.Read(exINI, pSection, "Trajectory.Engrave.IsLaser");
	this->IsIntense.Read(exINI, pSection, "Trajectory.Engrave.IsIntense");
	this->IsHouseColor.Read(exINI, pSection, "Trajectory.Engrave.IsHouseColor");
	this->IsSingleColor.Read(exINI, pSection, "Trajectory.Engrave.IsSingleColor");
	this->LaserInnerColor.Read(exINI, pSection, "Trajectory.Engrave.LaserInnerColor");
	this->LaserOuterColor.Read(exINI, pSection, "Trajectory.Engrave.LaserOuterColor");
	this->LaserOuterSpread.Read(exINI, pSection, "Trajectory.Engrave.LaserOuterSpread");
	this->LaserThickness.Read(exINI, pSection, "Trajectory.Engrave.LaserThickness");
	this->LaserThickness = Math::max(1, this->LaserThickness);
	this->LaserDuration.Read(exINI, pSection, "Trajectory.Engrave.LaserDuration");
	this->LaserDuration = Math::max(1, this->LaserDuration);
	this->LaserDelay.Read(exINI, pSection, "Trajectory.Engrave.LaserDelay");
	this->LaserDelay = Math::max(1, this->LaserDelay);
}

template<typename T>
void EngraveTrajectory::Serialize(T& Stm)
{
	Stm
		.Process(this->Type)
		.Process(this->LaserTimer)
		;
}

bool EngraveTrajectory::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->VirtualTrajectory::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool EngraveTrajectory::Save(PhobosStreamWriter& Stm) const
{
	this->VirtualTrajectory::Save(Stm);
	const_cast<EngraveTrajectory*>(this)->Serialize(Stm);
	return true;
}

void EngraveTrajectory::OnUnlimbo()
{
	this->VirtualTrajectory::OnUnlimbo();

	this->LaserTimer.Start(0);
	const auto pBullet = this->Bullet;

	if (!BulletExt::ExtMap.Find(pBullet)->DispersedTrajectory)
		this->OpenFire();
}

bool EngraveTrajectory::OnAIDetonateCheck()
{
	if (this->VirtualTrajectory::OnAIDetonateCheck())
		return true;

	return this->PlaceOnCorrectHeight();
}

void EngraveTrajectory::OnAINextFrameCheck()
{
	this->PhobosTrajectory::OnAINextFrameCheck();

	if (!this->Type->IsLaser || !this->LaserTimer.Completed())
		return;

	const auto pBullet = this->Bullet;
	const auto pFirer = pBullet->Owner;
	const auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;
	this->DrawEngraveLaser(pFirer, pOwner);
}

void EngraveTrajectory::OpenFire()
{
	const auto pBullet = this->Bullet;
	const auto pType = this->Type;
	const auto pFirer = pBullet->Owner;
	// To be used later, no reference
	auto source = pBullet->SourceCoords;
	auto target = pBullet->TargetCoords;
	CoordStruct virtualSource = pType->VirtualSourceCoord.Get();
	CoordStruct virtualTarget = pType->VirtualTargetCoord.Get();
	virtualSource.Z = 0;
	virtualTarget.Z = 0;
	const double rotateRadian = this->Get2DOpRadian((pFirer ? pFirer->GetCoords() : source), target);

	if (!this->NotMainWeapon && pType->MirrorCoord && this->CurrentBurst < 0)
	{
		virtualSource.Y = -virtualSource.Y;
		virtualTarget.Y = -virtualTarget.Y;
	}
	// Special case: Starting from the launch position
	if (virtualSource.X != 0 || virtualSource.Y != 0)
		source = target + PhobosTrajectory::Vector2Coord(PhobosTrajectory::HorizontalRotate(virtualSource, rotateRadian));

	if (!this->TargetInTheAir)
		source.Z = this->GetFloorCoordHeight(source);

	pBullet->SetLocation(source);
	target += PhobosTrajectory::Vector2Coord(PhobosTrajectory::HorizontalRotate(virtualTarget, rotateRadian));

	this->MovingVelocity.X = target.X - source.X;
	this->MovingVelocity.Y = target.Y - source.Y;
	this->MovingVelocity.Z = 0;

	if (this->CalculateBulletVelocity(pType->Speed))
		this->ShouldDetonate = true;

	this->PhobosTrajectory::OpenFire();
}

bool EngraveTrajectory::CalculateBulletVelocity(const double speed)
{
	// Only call once
	// Substitute the speed to calculate velocity
	auto velocityLength = this->MovingVelocity.Magnitude();

	if (velocityLength < 1e-10)
		return true;

	const auto pBullet = this->Bullet;
	const auto pType = this->Type;
	const auto pFirer = pBullet->Owner;
	// Calculate additional range
	if (pType->ApplyRangeModifiers && pFirer)
	{
		if (const auto pWeapon = pBullet->WeaponType)
			velocityLength = static_cast<double>(WeaponTypeExt::GetRangeWithModifiers(pWeapon, pFirer, static_cast<int>(velocityLength)));
	}
	// Automatically calculate duration
	if (pType->Duration <= 0)
		this->DurationTimer.Start(static_cast<int>(velocityLength / pType->Speed) + 1);

	this->MovingVelocity *= speed / velocityLength;
	this->MovingSpeed = speed;

	return false;
}

int EngraveTrajectory::GetFloorCoordHeight(const CoordStruct& coord)
{
	const auto onFloor = MapClass::Instance->GetCellFloorHeight(coord);
	const auto onBridge = MapClass::Instance->GetCellAt(coord)->ContainsBridge() ? onFloor + CellClass::BridgeHeight : onFloor;
	const auto pBullet = this->Bullet;
	// Take the higher position
	return (pBullet->SourceCoords.Z >= onBridge || pBullet->TargetCoords.Z >= onBridge) ? onBridge : onFloor;
}

bool EngraveTrajectory::PlaceOnCorrectHeight()
{
	if (!this->TargetInTheAir)
		return false;

	const auto pBullet = this->Bullet;
	auto bulletCoords = pBullet->Location;
	const auto futureCoords = bulletCoords + PhobosTrajectory::Vector2Coord(this->MovingVelocity);
	// Calculate where will be located in the next frame
	const auto checkDifference = this->GetFloorCoordHeight(futureCoords) - futureCoords.Z;
	// When crossing the cliff, directly move the position of the bullet, otherwise change the vertical velocity (384 -> (4 * Unsorted::LevelHeight - 32(error range)))
	if (std::abs(checkDifference) >= 384)
	{
		if (pBullet->Type->SubjectToCliffs)
			return true;
		// Move from low altitude to high altitude
		if (checkDifference > 0)
		{
			bulletCoords.Z += checkDifference;
			pBullet->SetLocation(bulletCoords);
		}
		else
		{
			const auto nowDifference = bulletCoords.Z - this->GetFloorCoordHeight(bulletCoords);
			// Less than 384 and greater than the maximum difference that can be achieved between two non cliffs
			if (nowDifference >= 256)
			{
				bulletCoords.Z -= nowDifference;
				pBullet->SetLocation(bulletCoords);
			}
		}
	}
	else
	{
		this->MovingVelocity.Z += checkDifference;
		this->MovingSpeed = this->MovingVelocity.Magnitude();
	}

	return false;
}

void EngraveTrajectory::DrawEngraveLaser(TechnoClass* pTechno, HouseClass* pOwner)
{
	const auto pBullet = this->Bullet;
	const auto pType = this->Type;
	this->LaserTimer.Start(pType->LaserDelay);
	auto fireCoord = pBullet->SourceCoords;

	for (auto pTrans = pTechno->Transporter; pTrans; pTrans = pTrans->Transporter)
		pTechno = pTrans;
	// Considering that the CurrentBurstIndex may be different, it is not possible to call existing functions
	if (!this->NotMainWeapon && pTechno && !pTechno->InLimbo)
	{
		if (pTechno->WhatAmI() != AbstractType::Building)
		{
			// The building turret uses PrimaryFacing and GetRenderCoords() to calculate the actual position, so this function is not available
			fireCoord = TechnoExt::GetFLHAbsoluteCoords(pTechno, this->FLHCoord, pTechno->HasTurret());
		}
		else
		{
			const auto pBuilding = static_cast<BuildingClass*>(pTechno);
			Matrix3D mtx;
			mtx.MakeIdentity();

			if (pTechno->HasTurret())
			{
				TechnoTypeExt::ApplyTurretOffset(pBuilding->Type, &mtx);
				mtx.RotateZ(static_cast<float>(pTechno->TurretFacing().GetRadian<32>()));
			}

			mtx.Translate(static_cast<float>(this->FLHCoord.X), static_cast<float>(this->FLHCoord.Y), static_cast<float>(this->FLHCoord.Z));
			const auto result = mtx.GetTranslation();
			fireCoord = pBuilding->GetCoords() + this->BuildingCoord + CoordStruct { static_cast<int>(result.X), -static_cast<int>(result.Y), static_cast<int>(result.Z) };
		}
	}

	auto targetCoord = pBullet->Location;
	const auto fireStormCoords = MapClass::Instance->FindFirstFirestorm(fireCoord, targetCoord, pOwner);

	if (fireStormCoords != CoordStruct::Empty)
	{
		targetCoord = fireStormCoords;
		targetCoord.Z = MapClass::Instance->GetCellFloorHeight(fireStormCoords); // TODO accurate
	}

	// Draw laser from head to tail
	if (pType->IsHouseColor || pType->IsSingleColor)
	{
		const auto pLaser = GameCreate<LaserDrawClass>(fireCoord, targetCoord, ((pType->IsHouseColor && pOwner) ? pOwner->LaserColor : pType->LaserInnerColor), ColorStruct { 0, 0, 0 }, ColorStruct { 0, 0, 0 }, pType->LaserDuration);
		pLaser->IsHouseColor = true;
		pLaser->Thickness = pType->LaserThickness;
		pLaser->IsSupported = pType->IsIntense;
	}
	else
	{
		const auto pLaser = GameCreate<LaserDrawClass>(fireCoord, targetCoord, pType->LaserInnerColor, pType->LaserOuterColor, pType->LaserOuterSpread, pType->LaserDuration);
		pLaser->IsHouseColor = false;
		pLaser->Thickness = 3;
		pLaser->IsSupported = false;
	}
}
