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

	TechnoTypeExt::ExtData::AttachmentDataEntry* Data;
	TechnoClass* Parent;
	TechnoClass* Child;
	CDTimerClass RespawnTimer;

	AttachmentClass(TechnoTypeExt::ExtData::AttachmentDataEntry* data,
		TechnoClass* pParent, TechnoClass* pChild = nullptr) :
		Data { data },
		Parent { pParent },
		Child { pChild },
		RespawnTimer { }
	{
		AttachmentClass::Array.emplace_back(this);
	}

	AttachmentClass() :
		Data { },
		Parent { },
		Child { },
		RespawnTimer { }
	{
		AttachmentClass::Array.emplace_back(this);
	}

	~AttachmentClass();

	AttachmentTypeClass* GetType()
	{
		return AttachmentTypeClass::Array[this->Data->Type].get();
	}
	TechnoTypeClass* GetChildType()
	{
		return this->Data->TechnoType.isset() ? TechnoTypeClass::Array.GetItem(this->Data->TechnoType) : nullptr;
	}
	CoordStruct GetChildLocation()
	{
		return TechnoExt::GetFLHAbsoluteCoords(this->Parent, this->Data->FLH.Get(), this->Data->IsOnTurret);
	}

	void Initialize();
	void CreateChild();
	void AI();
	void Destroy(TechnoClass* pSource);
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
