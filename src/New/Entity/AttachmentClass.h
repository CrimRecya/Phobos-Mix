#pragma once

#include <algorithm>

#include <GeneralStructures.h>

#include <New/Type/AttachmentTypeClass.h>
#include <Ext/TechnoType/Body.h>

class TechnoClass;

class AttachmentClass
{
public:
	static std::vector<AttachmentClass*> Array;

	AttachmentTypeClass* Type;
	TechnoClass* Parent;
	TechnoClass* Child;
	CDTimerClass RespawnTimer;
	CoordStruct StartFLH;
	CoordStruct DesiredFLH;
	CDTimerClass TransFLHTimer;
	DirStruct StartDir;
	DirStruct DesiredDir;
	CDTimerClass TransDirTimer;

	AttachmentClass(AttachmentTypeClass* pType,
		TechnoClass* pParent, TechnoClass* pChild = nullptr) :
		Type { pType },
		Parent { pParent },
		Child { pChild },
		RespawnTimer {},
		StartFLH { pType->FLH.Get() },
		DesiredFLH { pType->FLH.Get() },
		TransFLHTimer {},
		StartDir { DirStruct(pType->RotationAdjust) },
		DesiredDir { DirStruct(pType->RotationAdjust) },
		TransDirTimer {}
	{
		AttachmentClass::Array.emplace_back(this);
	}

	AttachmentClass() :
		Type {},
		Parent {},
		Child {},
		RespawnTimer {},
		StartFLH {},
		DesiredFLH {},
		TransFLHTimer {},
		StartDir {},
		DesiredDir {},
		TransDirTimer {}
	{
		AttachmentClass::Array.emplace_back(this);
	}

	~AttachmentClass();

	AttachmentTypeClass* GetType() const { return this->Type; }
	TechnoTypeClass* GetChildType() const { return this->Type->TechnoType; }

	void TransformChildFace(const DirStruct& dir, int time);
	DirStruct GetChildFace() const;
	DirStruct GetChildDirection() const;

	void TransformChildFLH(const CoordStruct& flh, int time);
	CoordStruct GetChildFLH() const;
	CoordStruct GetChildLocation() const;

	void Initialize();
	void CreateChild();
	void AI();
	void Destroy(TechnoClass* pSource);
	void UnInit();
	void ChildDestroyed();

	void Unlimbo();
	void Limbo();

	bool AttachChild(TechnoClass* pChild);
	bool DetachChild();

	void InvalidatePointer(void* ptr);

	bool Load(PhobosStreamReader& stm, bool registerForChange);
	bool Save(PhobosStreamWriter& stm) const;

private:
	template <typename T>
	bool Serialize(T& stm);
};

class AttachmentTransformGroup
{
public:
	ValueableVector<AttachmentTypeClass*> Types {};
	Nullable<AffectedHouse> AffectedHouses {};
	Valueable<int> FLHTime { -1 };
	Valueable<CoordStruct> FLH { CoordStruct::Empty };
	Valueable<int> RotationTime { -1 };
	Valueable<DirType> Rotation { DirType::North };

	bool Load(PhobosStreamReader& stm, bool registerForChange);
	bool Save(PhobosStreamWriter& stm) const;

	static void Parse(std::vector<AttachmentTransformGroup>& list, INI_EX& exINI, const char* section, AffectedHouse defaultAffectHouse);

	static void Trasform(AttachmentClass* pAttachment, const std::vector<AttachmentTransformGroup>& trasformPairs, HouseClass* pOwner);

private:
	template <typename T>
	bool Serialize(T& stm);
};
