#include "Body.h"

// Passable TerrainTypes Hook #1 - Do not set occupy bits.
DEFINE_HOOK(0x71C110, TerrainClass_SetOccupyBit_PassableTerrain, 0x6)
{
	enum { Skip = 0x71C1A0 };

	GET(TerrainClass*, pThis, ECX);

	auto const pTypeExt = TerrainTypeExt::ExtMap.Find(pThis->Type);

	if (pTypeExt->IsPassable)
		return Skip;

	return 0;
}

// Passable TerrainTypes Hook #2 - Do not display attack cursor unless force-firing.
DEFINE_HOOK(0x7002E9, TechnoClass_WhatAction_PassableTerrain, 0x5)
{
	enum { ReturnAction = 0x70020E };

	GET(TechnoClass*, pThis, ESI);
	GET(ObjectClass*, pTarget, EDI);
	GET_STACK(const bool, isForceFire, STACK_OFFSET(0x1C, 0x8));

	if (!pThis->Owner->IsControlledByCurrentPlayer() || !pThis->IsControllable())
		return 0;

	if (const auto pTerrain = abstract_cast<TerrainClass*, true>(pTarget))
	{
		if (!isForceFire && TerrainTypeExt::ExtMap.Find(pTerrain->Type)->IsPassable)
		{
			R->EBP(Action::Move);
			return ReturnAction;
		}
	}

	return 0;
}

// Passable TerrainTypes Hook #3 - Count passable TerrainTypes as completely passable.
DEFINE_HOOK(0x483DDF, CellClass_CheckPassability_PassableTerrain, 0x6)
{
	enum { ReturnFromFunction = 0x483E25 };

	GET(CellClass*, pThis, EDI);
	GET(TerrainClass*, pTerrain, ESI);

	auto const pTypeExt = TerrainTypeExt::ExtMap.Find(pTerrain->Type);

	if (pTypeExt->IsPassable)
	{
		pThis->Passability = PassabilityType::Passable;
		return ReturnFromFunction;
	}

	return 0;
}

// Passable TerrainTypes Hook #4 - Make passable for vehicles.
DEFINE_HOOK(0x73FB71, UnitClass_CanEnterCell_PassableTerrain, 0x6)
{
	enum { SkipTerrainChecks = 0x73FA7C };

	GET(AbstractClass*, pTarget, ESI);

	if (auto const pTerrain = abstract_cast<TerrainClass*>(pTarget))
	{
		auto const pTypeExt = TerrainTypeExt::ExtMap.Find(pTerrain->Type);

		if (pTypeExt->IsPassable)
			return SkipTerrainChecks;
	}

	return 0;
}

#pragma region FindBuildLocation

namespace FindBuildLocationTemp
{
	bool EvaluatingBuildLocation = false;
}

// Set the global flag when calling this from evaluating building locations for AI.
static bool __fastcall MapClass_IsAreaFree_Wrapper(MapClass* pThis, void* _, RectangleStruct* pRect, int houseID)
{
	FindBuildLocationTemp::EvaluatingBuildLocation = true;
	bool result = pThis->IsAreaFree(pRect, houseID);
	FindBuildLocationTemp::EvaluatingBuildLocation = false;
	return result;
}

DEFINE_FUNCTION_JUMP(CALL, 0x5069DB, MapClass_IsAreaFree_Wrapper);

// Ignore buildable terrain when evaluating building locations for AI. Replaces the vanilla function.
DEFINE_HOOK(0x586780, MapClass_IsAreaFree, 0x7)
{
	enum { ReturnFromFunction = 0x586887 };

	GET(MapClass*, pThis, ECX);
	GET_STACK(RectangleStruct*, pRect, 0x4);
	GET_STACK(int, houseID, 0x8);

	int mask = houseID >= 0 ? 1 << houseID : 0;

	for (int x = pRect->X; x < pRect->X + pRect->Width; x++)
	{
		for (int y = pRect->Y; y < pRect->Y + pRect->Height; y++)
		{
			CellClass* pCell = pThis->GetCellAt(CellStruct { static_cast<short>(x), static_cast<short>(y) });
			auto const pTerrain = pCell->GetTerrain(false);
			bool altPassability = false;

			if (pTerrain)
			{
				if (!FindBuildLocationTemp::EvaluatingBuildLocation || !TerrainTypeExt::ExtMap.Find(pTerrain->Type)->CanBeBuiltOn)
				{
					R->EAX(false);
					return ReturnFromFunction;
				}

				altPassability = true;
			}

			// If we're evaluating a cell with buildable TerrainType on it, passability check needs some alterations.
			const bool invalidPassability = altPassability
				? (pCell->Passability != PassabilityType::Passable && pCell->Passability != PassabilityType::HasFreeSpots)
				: (pCell->Passability != PassabilityType::Passable);

			if ((pCell->BaseSpacerOfHouses & mask) != 0
				|| pCell->OverlayTypeIndex != -1
				|| invalidPassability
				|| pCell->SlopeIndex
				|| pCell->GetBuilding())
			{
				R->EAX(false);
				return ReturnFromFunction;
			}
		}
	}

	R->EAX(pThis->InLocalRadar(pRect, true));
	return ReturnFromFunction;
}

#pragma endregion
