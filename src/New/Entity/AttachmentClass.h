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
	size_t DataIndex; // For save/load fix
	AttachmentTypeClass* Type; // Quick get type
	TechnoClass* Parent;
	TechnoClass* Child;
	CDTimerClass RespawnTimer;

	AttachmentClass(TechnoTypeExt::ExtData::AttachmentDataEntry* pData,
		TechnoClass* pParent, TechnoClass* pChild = nullptr) :
		Data { pData },
		DataIndex { pData->DataIndex },
		Type { AttachmentTypeClass::Array[pData->Type].get() },
		Parent { pParent },
		Child { pChild },
		RespawnTimer { }
	{
		AttachmentClass::Array.emplace_back(this);
	}

	AttachmentClass() :
		Data { },
		DataIndex { },
		Type { },
		Parent { },
		Child { },
		RespawnTimer { }
	{
		AttachmentClass::Array.emplace_back(this);
	}

	~AttachmentClass();

	AttachmentTypeClass* GetType() const;
	TechnoTypeClass* GetChildType() const;
	CoordStruct GetChildLocation() const;

	void Initialize();
	void LinkDataAfterLoad();

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
