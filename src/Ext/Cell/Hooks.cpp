#include "Body.h"

#include <Ext/Techno/Body.h>

void __fastcall UnitClass_SetOccupyBit_Reimpl(UnitClass* pThis, void*, CoordStruct* pCrd)
{
	if (TechnoExt::DoesntOccupyCellAsChild(pThis))
		return;

	const auto pCell = MapClass::Instance.GetCellAt(*pCrd);
	const auto pCellExt = CellExt::ExtMap.Find(pCell);
	const int height = MapClass::Instance.GetCellFloorHeight(*pCrd) + CellClass::BridgeHeight;
	const bool alt = (pCrd->Z >= height && pCell->ContainsBridge());

	// remember which occupation bit we set
	auto pExt = TechnoExt::ExtMap.Find(pThis);
	pExt->AltOccupation = alt;

	if (const auto pLastCell = pExt->LastOccupationCell)
	{
		auto const pLastCellExt = CellExt::ExtMap.Find(pLastCell);

		if (pLastCellExt->IncomingUnitAlt == pThis)
		{
			pLastCellExt->IncomingUnitAlt = nullptr;
			pLastCell->AltOccupationFlags &= ~0x20;
		}

		if (pLastCellExt->IncomingUnit == pThis)
		{
			pLastCellExt->IncomingUnit = nullptr;
			pLastCell->OccupationFlags &= ~0x20;
		}
	}

	pExt->LastOccupationCell = pExt->ThisOccupationCell;
	pExt->ThisOccupationCell = pCell;

	if (alt)
	{
		pCell->AltOccupationFlags |= 0x20;
		// Phobos addition: set incoming unit tracker
		pCellExt->IncomingUnitAlt = pThis;
	}
	else
	{
		pCell->OccupationFlags |= 0x20;
		// Phobos addition: set incoming unit tracker
		pCellExt->IncomingUnit = pThis;
	}
}
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F5D60, UnitClass_SetOccupyBit_Reimpl);

void __fastcall UnitClass_ClearOccupyBit_Reimpl(UnitClass* pThis, void*, CoordStruct* pCrd)
{
	if (TechnoExt::DoesntOccupyCellAsChild(pThis))
		return;

	enum { obNormal = 1, obAlt = 2 };

	const auto pCell = MapClass::Instance.GetCellAt(*pCrd);
	const auto pCellExt = CellExt::ExtMap.Find(pCell);
	const int height = MapClass::Instance.GetCellFloorHeight(*pCrd) + CellClass::BridgeHeight;
	int alt = (pCrd->Z >= height) ? obAlt : obNormal;

	// also clear the last occupation bit, if set
	const auto pExt = TechnoExt::ExtMap.Find(pThis);

	if (pExt->AltOccupation.has_value())
	{
		int lastAlt = pExt->AltOccupation.value() ? obAlt : obNormal;
		alt |= lastAlt;
		pExt->AltOccupation.reset();
	}

	if (alt & obAlt)
		pCell->AltOccupationFlags &= ~0x20;

	if (alt & obNormal)
		pCell->OccupationFlags &= ~0x20;

	// Phobos addition: clear incoming unit tracker
	if (pCellExt->IncomingUnitAlt == pThis)
		pCellExt->IncomingUnitAlt = nullptr;

	if (pCellExt->IncomingUnit == pThis)
		pCellExt->IncomingUnit = nullptr;
}
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F5D64, UnitClass_ClearOccupyBit_Reimpl);

// TODO ^ same for TA for non-UnitClass, not needed so cba for now

DEFINE_HOOK(0x480E27, CellClass_DamageWall_DamageWallRecursivly, 0x5)
{
	enum { SkipGameCode = 0x480EBC };
	return RulesExt::Global()->DamageWallRecursivly ? 0 : SkipGameCode;
}
