#pragma once

#include <Utilities/Enum.h>
#include <Utilities/Enumerable.h>
#include <Utilities/Template.h>

#include <TechnoTypeClass.h>

class AttachmentTypeClass final : public Enumerable<AttachmentTypeClass>
{
public:
	Valueable<TechnoTypeClass*> TechnoType;
	Valueable<CoordStruct> FLH;
	Valueable<bool> IsOnTurret;
	Valueable<DirType> RotationAdjust;
	Valueable<bool> RespawnAtCreation; // whether to spawn the attachment initially
	Valueable<int> RespawnDelay;
	Valueable<bool> InheritCommands;
	Valueable<bool> InheritCommands_StopCommand;
	Valueable<bool> InheritCommands_DeployCommand;
	Valueable<bool> InheritOwner; // aka mind control inheritance
	Valueable<bool> InheritStateEffects; // phasing out, stealth etc.
	Valueable<bool> InheritDestruction;
	Valueable<bool> InheritHeightStatus;
	Valueable<bool> OccupiesCell;
	Valueable<bool> LowSelectionPriority;
	Valueable<bool> TransparentToMouse;
	Valueable<AttachmentYSortPosition> YSortPosition;
	Valueable<WeaponTypeClass*> DestructionWeapon_Child;
	Valueable<WeaponTypeClass*> DestructionWeapon_Parent;
	Valueable<Mission> ParentDestructionMission;
	Valueable<Mission> ParentDetachmentMission;

	AttachmentTypeClass(const char* pTitle = NONE_STR) : Enumerable<AttachmentTypeClass>(pTitle)
		, TechnoType { nullptr }
		, FLH { CoordStruct::Empty }
		, IsOnTurret { false }
		, RotationAdjust { DirType::North }
		, RespawnAtCreation { true }
		, RespawnDelay { -1 }
		, InheritCommands { true }
		, InheritCommands_StopCommand { true }
		, InheritCommands_DeployCommand { true }
		, InheritOwner { true }
		, InheritStateEffects { true }
		, OccupiesCell { true }
		, InheritDestruction { true }
		, InheritHeightStatus { true }
		, LowSelectionPriority { true }
		, TransparentToMouse { false }
		, YSortPosition { AttachmentYSortPosition::Default }
		, DestructionWeapon_Child { nullptr }
		, DestructionWeapon_Parent { nullptr }
		, ParentDestructionMission { Mission::None }
		, ParentDetachmentMission { Mission::None }
	{ }

	virtual ~AttachmentTypeClass() = default;

	virtual void LoadFromINI(CCINIClass* pINI);
	virtual void LoadFromStream(PhobosStreamReader& Stm);
	virtual void SaveToStream(PhobosStreamWriter& Stm);

private:
	template <typename T>
	void Serialize(T& Stm);
};
