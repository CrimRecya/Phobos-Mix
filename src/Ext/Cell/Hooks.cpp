#include "Body.h"

#include <Ext/Techno/Body.h>


// because 🦅💣 takes over, we have to do reimpl even more of the func and replicate Ares code

void __fastcall UnitClass_SetOccupyBit_Reimpl(UnitClass* pThis, discard_t, CoordStruct* pCrd)
{
	if (TechnoExt::DoesntOccupyCellAsChild(pThis))
		return;

	const auto pCell = MapClass::Instance.GetCellAt(*pCrd);
	const auto pCellExt = CellExt::ExtMap.Find(pCell);

	if (pCrd->Z >= (MapClass::Instance.GetCellFloorHeight(*pCrd) + CellClass::BridgeHeight) && pCell->ContainsBridge())
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

void __fastcall UnitClass_ClearOccupyBit_Reimpl(UnitClass* pThis, discard_t, CoordStruct* pCrd)
{
	if (TechnoExt::DoesntOccupyCellAsChild(pThis))
		return;

	const auto pCell = MapClass::Instance.GetCellAt(*pCrd);

	if (pCrd->Z >= (MapClass::Instance.GetCellFloorHeight(*pCrd) + CellClass::BridgeHeight))
		pCell->AltOccupationFlags &= ~0x20;
	else
		pCell->OccupationFlags &= ~0x20;

	const auto pCellExt = CellExt::ExtMap.Find(pCell);

	// Phobos addition: clear incoming unit tracker
	if (pCellExt->IncomingUnitAlt == pThis)
		pCellExt->IncomingUnitAlt = nullptr;

	if (pCellExt->IncomingUnit == pThis)
		pCellExt->IncomingUnit = nullptr;
}

DEFINE_FUNCTION_JUMP(VTABLE, 0x7F5D64, UnitClass_ClearOccupyBit_Reimpl);

// TODO ^ same for TA for non-UnitClass, not needed so cba for now
