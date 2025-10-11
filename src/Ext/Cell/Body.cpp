#include "Body.h"

#include <Helpers/Macro.h>

#include <Utilities/TemplateDef.h>

CellExt::ExtContainer CellExt::ExtMap;

void CellExt::ExtData::Initialize() { }

// =============================
// load / save

template <typename T>
void CellExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->RadSites)
		.Process(this->RadLevels)
		.Process(this->InfantryCount)
		.Process(this->IncomingUnit)
		.Process(this->IncomingUnitAlt)
		.Process(this->SmudgeGenerate)
		.Process(this->SmudgeState)
		;
}

void CellExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<CellClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void CellExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<CellClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool CellExt::RadLevel::Load(PhobosStreamReader& stm, bool registerForChange)
{
	return this->Serialize(stm);
}

bool CellExt::RadLevel::Save(PhobosStreamWriter& stm) const
{
	return const_cast<CellExt::RadLevel*>(this)->Serialize(stm);
}

template <typename T>
bool CellExt::RadLevel::Serialize(T& stm)
{
	return stm
		.Process(this->Rad)
		.Process(this->Level)
		.Success();
}

// =============================
// container

CellExt::ExtContainer::ExtContainer() : Container("CellClass") { }
CellExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x47BDA1, CellClass_CTOR, 0x5)
{
	GET(CellClass*, pItem, ESI);

	CellExt::ExtMap.Allocate(pItem);

	return 0;
}

DEFINE_HOOK(0x47BB60, CellClass_DTOR, 0x6)
{
	GET(CellClass*, pItem, ECX);

	CellExt::ExtMap.Remove(pItem);

	return 0;
}

DEFINE_HOOK_AGAIN(0x483C10, CellClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x4839F0, CellClass_SaveLoad_Prefix, 0x7)
{
	GET_STACK(CellClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	CellExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x483C00, CellClass_Load_Suffix, 0x5)
{
	CellExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x483C79, CellClass_Save_Suffix, 0x6)
{
	CellExt::ExtMap.SaveStatic();
	return 0;
}
