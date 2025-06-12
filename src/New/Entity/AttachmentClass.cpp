#include "AttachmentClass.h"

#include <Dir.h>
#include <BulletClass.h>
#include <BulletTypeClass.h>
#include <WarheadTypeClass.h>

#include <ObjBase.h>

#include <Ext/Techno/Body.h>
#include <Locomotion/AttachmentLocomotionClass.h>

std::vector<AttachmentClass*> AttachmentClass::Array;

void AttachmentClass::TransformChildFace(const DirStruct& dir, int time)
{
	this->StartDir = this->GetChildFace();
	this->DesiredDir = dir;
	this->TransDirTimer.Start(time);
}

DirStruct AttachmentClass::GetChildFace() const
{
	if (this->TransDirTimer.TimeLeft && this->TransDirTimer.HasTimeLeft())
	{
		const short diff = static_cast<short>(static_cast<short>(this->DesiredDir.Raw) - static_cast<short>(this->StartDir.Raw));
		return DirStruct { (this->DesiredDir.Raw - diff * this->TransDirTimer.GetTimeLeft() / this->TransDirTimer.TimeLeft) };
	}

	return this->DesiredDir;
}

DirStruct AttachmentClass::GetChildDirection() const
{
	const auto pParent = this->Parent;
	DirStruct childDir = this->GetType()->IsOnTurret ? pParent->SecondaryFacing.Current() : pParent->PrimaryFacing.Current();

	return DirStruct { (childDir.Raw + this->GetChildFace().Raw) };
}

void AttachmentClass::TransformChildFLH(const CoordStruct& flh, int time)
{
	this->StartFLH = this->GetChildFLH();
	this->DesiredFLH = flh;
	this->TransFLHTimer.Start(time);
}

CoordStruct AttachmentClass::GetChildFLH() const
{
	if (this->TransFLHTimer.TimeLeft && this->TransFLHTimer.HasTimeLeft())
	{
		const double ratio = static_cast<double>(this->TransFLHTimer.GetTimeLeft()) / this->TransFLHTimer.TimeLeft;
		return (this->DesiredFLH - (this->DesiredFLH - this->StartFLH) * ratio);
	}

	return this->DesiredFLH;
}

CoordStruct AttachmentClass::GetChildLocation() const
{
	return TechnoExt::GetFLHAbsoluteCoords(this->Parent, this->GetChildFLH(), this->GetType()->IsOnTurret);
}

AttachmentClass::~AttachmentClass()
{
	// clean up non-owning references
	if (this->Child)
	{
		auto const& pChildExt = TechnoExt::ExtMap.Find(Child);
		pChildExt->ParentAttachment = nullptr;
	}

	auto position = std::find(AttachmentClass::Array.begin(), AttachmentClass::Array.end(), this);

	if (position != AttachmentClass::Array.end())
		AttachmentClass::Array.erase(position);
}

void AttachmentClass::Initialize()
{
	if (this->Child)
		return;

	if (this->GetType()->RespawnAtCreation)
		this->CreateChild();
}

void AttachmentClass::CreateChild()
{
	if (auto const pChildType = this->GetChildType())
	{
		if (pChildType->WhatAmI() != AbstractType::UnitType)
			return;

		if (const auto pTechno = static_cast<TechnoClass*>(pChildType->CreateObject(this->Parent->Owner)))
		{
			this->AttachChild(pTechno);
		}
		else
		{
			Debug::Log("[" __FUNCTION__ "] Failed to create child %s of parent %s!\n",
				pChildType->ID, this->Parent->GetTechnoType()->ID);
		}
	}
}

void AttachmentClass::AI()
{
	AttachmentTypeClass* pType = this->GetType();

	if (!this->Child)
	{
		if (pType->RespawnDelay == 0)
		{
			this->CreateChild();
		}
		else if (pType->RespawnDelay > 0)
		{
			if (!this->RespawnTimer.HasStarted())
			{
				this->RespawnTimer.Start(pType->RespawnDelay);
			}
			else if (this->RespawnTimer.Completed())
			{
				this->CreateChild();
				this->RespawnTimer.Stop();
			}
		}
	}

	if (this->Child)
	{
		if (this->Child->InLimbo && !this->Parent->InLimbo)
			this->Unlimbo();
		else if (!this->Child->InLimbo && this->Parent->InLimbo)
			this->Limbo();

		this->Child->SetLocation(this->GetChildLocation());
		this->Child->PrimaryFacing.SetCurrent(this->GetChildDirection());
		// TODO handle secondary facing in case the turret is idle

		FootClass* pParentAsFoot = abstract_cast<FootClass*, true>(this->Parent);
		FootClass* pChildAsFoot = abstract_cast<FootClass*, true>(this->Child);

		if (pParentAsFoot && pChildAsFoot)
			pChildAsFoot->TubeIndex = pParentAsFoot->TubeIndex;

		if (pType->InheritStateEffects)
		{
			this->Child->IsFallingDown = this->Parent->IsFallingDown;
			this->Child->WasFallingDown = this->Parent->WasFallingDown;
			this->Child->CloakState = this->Parent->CloakState;
			this->Child->WarpingOut = this->Parent->WarpingOut;
			this->Child->unknown_280 = this->Parent->unknown_280; // sth related to teleport
			this->Child->BeingWarpedOut = this->Parent->BeingWarpedOut;
			this->Child->Deactivated = this->Parent->Deactivated;
			this->Child->Flash(this->Parent->Flashing.DurationRemaining);

			this->Child->IronCurtainTimer = this->Parent->IronCurtainTimer;
			this->Child->IdleActionTimer = this->Parent->IdleActionTimer;
			this->Child->IronTintTimer = this->Parent->IronTintTimer;
			this->Child->CloakDelayTimer = this->Parent->CloakDelayTimer;
			this->Child->ChronoLockRemaining = this->Parent->ChronoLockRemaining;
			this->Child->Berzerk = this->Parent->Berzerk;
			this->Child->BerzerkDurationLeft = this->Parent->BerzerkDurationLeft;
			this->Child->ChronoWarpedByHouse = this->Parent->ChronoWarpedByHouse;
			this->Child->EMPLockRemaining = this->Parent->EMPLockRemaining;
			this->Child->ShouldLoseTargetNow = this->Parent->ShouldLoseTargetNow;
		}

		if (pType->InheritOwner)
			this->Child->SetOwningHouse(this->Parent->GetOwningHouse(), false);
	}
}

// Called in Kill_Cargo, handles logics for parent destruction on children
void AttachmentClass::Destroy(TechnoClass* pSource)
{
	if (this->Child)
	{
		auto const pChildExt = TechnoExt::ExtMap.Find(this->Child);
		pChildExt->ParentAttachment = nullptr;

		auto pType = this->GetType();

		if (const auto pWeapon = pType->DestructionWeapon_Child.Get())
			TechnoExt::FireWeaponAtSelf(this->Child, pWeapon);

		if (pType->InheritDestruction && this->Child)
			TechnoExt::Kill(this->Child, pSource);
		else if (!this->Child->InLimbo && pType->ParentDestructionMission != Mission::None)
			this->Child->QueueMission(pType->ParentDestructionMission, false);

		this->Child = nullptr;
	}
}

void AttachmentClass::ChildDestroyed()
{
	if (this->Child)
	{
		if (auto const pChildExt = TechnoExt::ExtMap.Find(this->Child))
			pChildExt->ParentAttachment = nullptr;

		AttachmentTypeClass* pType = this->GetType();

		if (const auto pWeapon = pType->DestructionWeapon_Parent.Get())
			TechnoExt::FireWeaponAtSelf(this->Parent, pWeapon);

		this->Child = nullptr;
	}
}

void AttachmentClass::Unlimbo()
{
	if (this->Child)
	{
		++Unsorted::ScenarioInit;
		this->Child->Unlimbo(this->GetChildLocation(), this->GetChildDirection().GetDir());
		--Unsorted::ScenarioInit;
	}
}

void AttachmentClass::Limbo()
{
	if (this->Child)
		this->Child->Limbo();
}

bool AttachmentClass::AttachChild(TechnoClass* pChild)
{
	if (this->Child)
		return false;

	if (pChild->WhatAmI() != AbstractType::Unit)
		return false;

	if (auto const pChildAsFoot = abstract_cast<FootClass*, true>(pChild))
	{
		if (IPersistPtr pLocoPersist = pChildAsFoot->Locomotor)
		{
			CLSID locoCLSID { };

			if (SUCCEEDED(pLocoPersist->GetClassID(&locoCLSID)) && locoCLSID != __uuidof(AttachmentLocomotionClass))
				LocomotionClass::ChangeLocomotorTo(pChildAsFoot, __uuidof(AttachmentLocomotionClass));
		}
	}

	this->Child = pChild;

	auto pChildExt = TechnoExt::ExtMap.Find(this->Child);
	pChildExt->ParentAttachment = this;

	// bandaid for jitterless drawing. TODO fix properly
	// this->Child->GetTechnoType()->DisableVoxelCache = true;
	// this->Child->GetTechnoType()->DisableShadowCache = true;

	AttachmentTypeClass* pType = this->GetType();

	if (pType->InheritOwner)
	{
		if (auto pController = this->Child->MindControlledBy)
			pController->CaptureManager->FreeUnit(this->Child);
	}

	return true;
}

bool AttachmentClass::DetachChild()
{
	if (this->Child)
	{
		AttachmentTypeClass* pType = this->GetType();

		if (!this->Child->InLimbo && pType->ParentDetachmentMission != Mission::None)
			this->Child->QueueMission(pType->ParentDetachmentMission, false);

		// FIXME this won't work probably
		if (pType->InheritOwner)
			this->Child->SetOwningHouse(this->Parent->GetOriginalOwner(), false);

		// remove the attachment locomotor manually just to be safe
		if (auto const pChildAsFoot = abstract_cast<FootClass*, true>(this->Child))
			LocomotionClass::End_Piggyback(pChildAsFoot->Locomotor);

		auto pChildExt = TechnoExt::ExtMap.Find(this->Child);
		pChildExt->ParentAttachment = nullptr;
		this->Child = nullptr;

		return true;
	}

	return false;
}


void AttachmentClass::InvalidatePointer(void* ptr)
{
	AnnounceInvalidPointer(this->Parent, ptr);
	AnnounceInvalidPointer(this->Child, ptr);
}

#pragma region Save/Load

template <typename T>
bool AttachmentClass::Serialize(T& stm)
{
	return stm
		.Process(this->Type)
		.Process(this->Parent)
		.Process(this->Child)
		.Process(this->RespawnTimer)
		.Process(this->StartFLH)
		.Process(this->DesiredFLH)
		.Process(this->TransFLHTimer)
		.Process(this->StartDir)
		.Process(this->DesiredDir)
		.Process(this->TransDirTimer)
		.Success();
}

bool AttachmentClass::Load(PhobosStreamReader& stm, bool RegisterForChange)
{
	return Serialize(stm);
}

bool AttachmentClass::Save(PhobosStreamWriter& stm) const
{
	return const_cast<AttachmentClass*>(this)->Serialize(stm);
}

#pragma endregion

void AttachmentTransformGroup::Trasform(AttachmentClass* pAttachment, const std::vector<AttachmentTransformGroup>& trasformPairs, HouseClass* pOwner)
{
	for (const auto& [types, affectedHouses, flhTime, flh, rotationTime, rotation] : trasformPairs)
	{
		if (pOwner && !EnumFunctions::CanTargetHouse(affectedHouses, pOwner, pAttachment->Parent->Owner))
			continue;

		for (const auto& type : types)
		{
			if (type != pAttachment->Type)
				continue;

			if (flhTime >= 0)
				pAttachment->TransformChildFLH(flh, flhTime);

			if (rotationTime >= 0)
				pAttachment->TransformChildFace(DirStruct(rotation), rotationTime);

			return;
		}
	}

	return;
}

bool AttachmentTransformGroup::Load(PhobosStreamReader& stm, bool registerForChange)
{
	return this->Serialize(stm);
}

bool AttachmentTransformGroup::Save(PhobosStreamWriter& stm) const
{
	return const_cast<AttachmentTransformGroup*>(this)->Serialize(stm);
}

void AttachmentTransformGroup::Parse(std::vector<AttachmentTransformGroup>& list, INI_EX& exINI, const char* pSection, AffectedHouse defaultAffectHouse)
{
	for (size_t i = 0; ; ++i)
	{
		char tempBuffer[48];
		ValueableVector<AttachmentTypeClass*> types;
		Nullable<AffectedHouse> affectedHouses;
		Valueable<int> flhTime;
		Valueable<CoordStruct> flh;
		Valueable<int> rotationTime;
		Valueable<DirType> rotation;
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "AttachmentTransform%d.Types", i);
		types.Read(exINI, pSection, tempBuffer);
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "AttachmentTransform%d.AffectedHouses", i);
		affectedHouses.Read(exINI, pSection, tempBuffer);
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "AttachmentTransform%d.FLHTime", i);
		flhTime.Read(exINI, pSection, tempBuffer);
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "AttachmentTransform%d.FLH", i);
		flh.Read(exINI, pSection, tempBuffer);
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "AttachmentTransform%d.RotationTime", i);
		rotationTime.Read(exINI, pSection, tempBuffer);
		_snprintf_s(tempBuffer, sizeof(tempBuffer), "AttachmentTransform%d.Rotation", i);
		rotation.Read(exINI, pSection, tempBuffer);

		if (types.empty() || flhTime < 0 && rotationTime < 0)
			break;

		if (!affectedHouses.isset())
			affectedHouses = defaultAffectHouse;

		list.emplace_back(types, affectedHouses, flhTime, flh, rotationTime, rotation);
	}
	ValueableVector<AttachmentTypeClass*> types;
	Nullable<AffectedHouse> affectedHouses;
	Valueable<int> flhTime;
	Valueable<CoordStruct> flh;
	Valueable<int> rotationTime;
	Valueable<DirType> rotation;
	types.Read(exINI, pSection, "AttachmentTransform.Types");
	affectedHouses.Read(exINI, pSection, "AttachmentTransform.AffectedHouses");
	flhTime.Read(exINI, pSection, "AttachmentTransform.FLHTime");
	flh.Read(exINI, pSection, "AttachmentTransform.FLH");
	rotationTime.Read(exINI, pSection, "AttachmentTransform.RotationTime");
	rotation.Read(exINI, pSection, "AttachmentTransform.Rotation");
	if (!types.empty() && (flhTime >= 0 || rotationTime >= 0))
	{
		if (!affectedHouses.isset())
			affectedHouses = defaultAffectHouse;

		if (list.size())
			list[0] = { types, affectedHouses, flhTime, flh, rotationTime, rotation };
		else
			list.emplace_back(types, affectedHouses, flhTime, flh, rotationTime, rotation);
	}
}

template <typename T>
bool AttachmentTransformGroup::Serialize(T& stm)
{
	return stm
		.Process(this->Types)
		.Process(this->AffectedHouses)
		.Process(this->FLHTime)
		.Process(this->FLH)
		.Process(this->RotationTime)
		.Process(this->Rotation)
		.Success();
}
