#include "PhobosVirtualTrajectory.h"

#include <Ext/Techno/Body.h>

template<typename T>
void VirtualTrajectoryType::Serialize(T& Stm)
{
	Stm
		.Process(this->VirtualSourceCoord)
		.Process(this->VirtualTargetCoord)
		.Process(this->AllowFirerTurning)
		.Process(this->IgnoresFirestorm)
		;
}

bool VirtualTrajectoryType::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->PhobosTrajectoryType::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool VirtualTrajectoryType::Save(PhobosStreamWriter& Stm) const
{
	this->PhobosTrajectoryType::Save(Stm);
	const_cast<VirtualTrajectoryType*>(this)->Serialize(Stm);
	return true;
}
/*
void VirtualTrajectoryType::Read(CCINIClass* const pINI, const char* pSection) // Read separately
{
	this->PhobosTrajectoryType::Read(pINI, pSection);
	INI_EX exINI(pINI);

	this->VirtualSourceCoord.Read(exINI, pSection, "Trajectory.VirtualSourceCoord");
	this->VirtualTargetCoord.Read(exINI, pSection, "Trajectory.VirtualTargetCoord");
	this->AllowFirerTurning.Read(exINI, pSection, "Trajectory.AllowFirerTurning");
}
*/
template<typename T>
void VirtualTrajectory::Serialize(T& Stm)
{
	Stm
		.Process(this->SurfaceFirerID)
		;
}

bool VirtualTrajectory::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->PhobosTrajectory::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool VirtualTrajectory::Save(PhobosStreamWriter& Stm) const
{
	this->PhobosTrajectory::Save(Stm);
	const_cast<VirtualTrajectory*>(this)->Serialize(Stm);
	return true;
}

void VirtualTrajectory::OnUnlimbo()
{
	this->PhobosTrajectory::OnUnlimbo();

	this->RemainingDistance = INT_MAX;

	for (auto pTrans = this->Bullet->Owner; pTrans; pTrans = pTrans->Transporter)
		this->SurfaceFirerID = pTrans->UniqueID;
}

bool VirtualTrajectory::OnAI()
{
	if (this->OnAIDetonateCheck())
		return true;

	this->OnAIVelocityCheck();

	if (this->PhobosTrajectory::OnAI())
		return true;

	this->OnAINextFrameCheck();
	return false;
}

bool VirtualTrajectory::OnAIDetonateCheck()
{
	if (!this->NotMainWeapon && this->InvalidFireCondition(this->Bullet->Owner))
		return true;

	return this->PhobosTrajectory::OnAIDetonateCheck();
}

bool VirtualTrajectory::InvalidFireCondition(TechnoClass* pTechno)
{
	if (!pTechno)
		return true;

	for (auto pTrans = pTechno->Transporter; pTrans; pTrans = pTrans->Transporter)
		pTechno = pTrans;

	if (!TechnoExt::IsActive(pTechno) || this->SurfaceFirerID != pTechno->UniqueID)
		return true;

	if (static_cast<const VirtualTrajectoryType*>(this->GetType())->AllowFirerTurning)
		return false;

	const auto tgtDir = DirStruct(-PhobosTrajectory::Get2DOpRadian(pTechno->GetCoords(), this->Bullet->TargetCoords));
	const auto& face = pTechno->HasTurret() && pTechno->WhatAmI() == AbstractType::Unit ? pTechno->SecondaryFacing : pTechno->PrimaryFacing;
	const auto curDir = face.Current();
	// Similar to the vanilla 45 degree turret facing check design
	return (std::abs(static_cast<short>(static_cast<short>(tgtDir.Raw) - static_cast<short>(curDir.Raw))) >= 4096);
}
