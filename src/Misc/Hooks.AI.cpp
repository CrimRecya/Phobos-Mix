#include <New/Entity/AttachmentClass.h>

#include <Ext/Scenario/Body.h>
#include <Utilities/Macro.h>

#pragma region SmudgeUpdate

static bool __forceinline ShouldRemoveSmudgeCell(const int index, const int time, const int current)
{
	const auto cell = CellStruct{static_cast<short>(index & 511), static_cast<short>(index >> 9) };

	if (const auto pCell = MapClass::Instance.TryGetCellAt(cell))
	{
		if (pCell->SmudgeTypeIndex != -1)
		{
			const auto pCellExt = CellExt::ExtMap.Find(pCell);

			if ((pCellExt->SmudgeGenerate + time) > current)
				return false;

			const auto state = pCellExt->SmudgeState;

			if (state != BlitterFlags::TransLucent75)
			{
				pCellExt->SmudgeGenerate = current;
				pCellExt->SmudgeState = static_cast<BlitterFlags>(static_cast<size_t>(state) + 2u);
				pCell->MarkForRedraw();
				return false;
			}

			pCell->SmudgeTypeIndex = -1;
			pCell->MarkForRedraw();
		}
	}

	return true;
}

DEFINE_HOOK(0x55B6B3, LogicClass_AI_After, 0x5)
{
	for (auto const& attachment : AttachmentClass::Array)
		attachment->AI();

	const int time = RulesExt::Global()->SmudgeUpdateTime;

	if (time > 0)
	{
		auto& s = ScenarioExt::Global()->Smudges;

		if (!s.empty())
		{
			const int current = Unsorted::CurrentFrame;

			for (auto it = s.begin(); it != s.end(); )
			{
				if (ShouldRemoveSmudgeCell(*it, time, current))
					it = s.erase(it);
				else
					++it;
			}
		}
	}

	return 0;
}

#pragma endregion

#pragma region AirBarrier

void __fastcall FindMovingInfOrVeh(CellClass* const pCell, const AbstractType findType)
{
	const auto flag = pCell->OccupationFlags;
	pCell->OccupationFlags = 0;
	auto checkCell = pCell->MapCoords + CellStruct { 2, 2 };

	for (short checkX = checkCell.X - 4; checkX <= checkCell.X; ++checkX)
	{
		for (short checkY = checkCell.Y - 4; checkY <= checkCell.Y; ++checkY)
		{
			const auto pAdjCheckCell = MapClass::Instance.GetCellAt(CellStruct { checkX, checkY });

			for (auto pObject = pAdjCheckCell->FirstObject; pObject; pObject = pObject->NextObject)
			{
				if (pObject->WhatAmI() == findType && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
				{
					pCell->OccupationFlags = flag;
					return;
				}
			}
		}
	}
}

void __fastcall FindMovingInfAndVeh(CellClass* const pCell)
{
	const auto flag = pCell->OccupationFlags;
	pCell->OccupationFlags = 0;
	bool inf = false;
	bool veh = false;
	auto checkCell = pCell->MapCoords + CellStruct { 2, 2 };

	for (short checkX = checkCell.X - 4; checkX <= checkCell.X; ++checkX)
	{
		for (short checkY = checkCell.Y - 4; checkY <= checkCell.Y; ++checkY)
		{
			const auto pAdjCheckCell = MapClass::Instance.GetCellAt(CellStruct { checkX, checkY });

			for (auto pObject = pAdjCheckCell->FirstObject; pObject; pObject = pObject->NextObject)
			{
				const auto absType = pObject->WhatAmI();

				if (absType == AbstractType::Infantry)
				{
					if (!inf && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
					{
						pCell->OccupationFlags |= (flag & 0x1F);

						if (veh)
							return;

						inf = true;
					}
				}
				else if (absType == AbstractType::Unit)
				{
					if (!veh && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
					{
						pCell->OccupationFlags |= (flag & 0x20);

						if (inf)
							return;

						veh = true;
					}
				}
			}
		}
	}
}

void __fastcall FindAltMovingInfOrVeh(CellClass* const pCell, const AbstractType findType)
{
	const auto flag = pCell->AltOccupationFlags;
	pCell->AltOccupationFlags = 0;
	auto checkCell = pCell->MapCoords + CellStruct { 2, 2 };

	for (short checkX = checkCell.X - 4; checkX <= checkCell.X; ++checkX)
	{
		for (short checkY = checkCell.Y - 4; checkY <= checkCell.Y; ++checkY)
		{
			const auto pAdjCheckCell = MapClass::Instance.GetCellAt(CellStruct { checkX, checkY });

			for (auto pObject = pAdjCheckCell->AltObject; pObject; pObject = pObject->NextObject)
			{
				if (pObject->WhatAmI() == findType && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
				{
					pCell->AltOccupationFlags = flag;
					return;
				}
			}
		}
	}
}

void __fastcall FindAltMovingInfAndVeh(CellClass* const pCell)
{
	const auto flag = pCell->AltOccupationFlags;
	pCell->AltOccupationFlags = 0;
	bool inf = false;
	bool veh = false;
	auto checkCell = pCell->MapCoords + CellStruct { 2, 2 };

	for (short checkX = checkCell.X - 4; checkX <= checkCell.X; ++checkX)
	{
		for (short checkY = checkCell.Y - 4; checkY <= checkCell.Y; ++checkY)
		{
			const auto pAdjCheckCell = MapClass::Instance.GetCellAt(CellStruct { checkX, checkY });

			for (auto pObject = pAdjCheckCell->AltObject; pObject; pObject = pObject->NextObject)
			{
				const auto absType = pObject->WhatAmI();

				if (absType == AbstractType::Infantry)
				{
					if (!inf && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
					{
						pCell->AltOccupationFlags |= (flag & 0x1F);

						if (veh)
							return;

						inf = true;
					}
				}
				else if (absType == AbstractType::Unit)
				{
					if (!veh && CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCell->MapCoords)
					{
						pCell->AltOccupationFlags |= (flag & 0x20);

						if (inf)
							return;

						veh = true;
					}
				}
			}
		}
	}
}

DEFINE_HOOK(0x55B4E1, LogicClass_Update_UnmarkCellOccupationFlags, 0x5)
{
	const auto delay = RulesExt::Global()->CleanUpAirBarrier.Get();

	if (delay > 0 && !(Unsorted::CurrentFrame % delay))
	{
		auto& pMap = MapClass::Instance;
		pMap.CellIteratorReset();

		for (auto pCell = pMap.CellIteratorNext(); pCell; pCell = pMap.CellIteratorNext())
		{
			if ((0xFF & pCell->OccupationFlags) && !pCell->FirstObject)
			{
				pCell->OccupationFlags &= 0x3F; // ~(Aircraft | Building)

				if (pCell->OccupationFlags & 0x1F)
				{
					if (pCell->OccupationFlags & 0x20)
						FindMovingInfAndVeh(pCell);
					else
						FindMovingInfOrVeh(pCell, AbstractType::Infantry);
				}
				else if (pCell->OccupationFlags & 0x20)
				{
					FindMovingInfOrVeh(pCell, AbstractType::Unit);
				}
			}

			if ((0xFF & pCell->AltOccupationFlags) && !pCell->AltObject)
			{
				pCell->AltOccupationFlags &= 0x3F; // ~(Aircraft | Building)

				if (pCell->AltOccupationFlags & 0x1F)
				{
					if (pCell->AltOccupationFlags & 0x20)
						FindAltMovingInfAndVeh(pCell);
					else
						FindAltMovingInfOrVeh(pCell, AbstractType::Infantry);
				}
				else if (pCell->AltOccupationFlags & 0x20)
				{
					FindAltMovingInfOrVeh(pCell, AbstractType::Unit);
				}
			}
		}
	}

	return 0;
}

#pragma endregion

#pragma region DetectionLogic

DEFINE_HOOK(0x687C56, INIClass_ReadScenario_EnableFog, 0x5)
{
	const bool fog = RulesClass::Instance->FogOfWar;
	GameModeOptionsClass::Instance.FogOfWar = fog;
	ScenarioClass::Instance->SpecialFlags.FogOfWar = fog;
	return 0;
}

DEFINE_HOOK(0x5865E2, MapClass_IsLocationFogged_Reimplement, 0x5)
{
	GET_STACK(const CoordStruct*, pCoords, STACK_OFFSET(0x0, 0x4));

	const int level = pCoords->Z / Unsorted::LevelHeight;
	const int extra = (level & 1) ? ((level >> 1) + 1) : (level >> 1);
	const CellStruct cell { static_cast<short>((pCoords->X >> 8) - extra), static_cast<short>((pCoords->Y >> 8) - extra) };

	R->EAX(!!(MapClass::Instance.GetCellAt(cell)->Flags & CellFlags::Fogged));
	return 0;
}

DEFINE_HOOK(0x4A9CA0, DisplayClass_RevealFogShroud_Reimplement, 0x8)
{
	enum { SkipGameCode = 0x4A9DC6 };

	GET(DisplayClass*, pThis, ECX);
	GET_STACK(CellStruct*, pCellStruct, STACK_OFFSET(0x0, 0x4));
	GET_STACK(HouseClass*, pHouse, STACK_OFFSET(0x0, 0x8));
	GET_STACK(bool, increase, STACK_OFFSET(0x0, 0xC));

	const auto pCell = pThis->GetCellAt(*pCellStruct);
	const auto flags = pCell->Flags;
	const bool edgeNotRevealed = !(flags & CellFlags::EdgeRevealed);
	const bool shouldRadarRedraw = edgeNotRevealed || !(pCell->AltFlags & AltCellFlags::Mapped);
	bool shouldRedraw = shouldRadarRedraw;
	pCell->Flags = flags & ~CellFlags::IsPlot | CellFlags::EdgeRevealed;

	if (increase)
		pCell->IncreaseShroudCounter();
	else
		pCell->ReduceShroudCounter();

	const char newVisibility = TacticalClass::Instance->GetOcclusion(*pCellStruct, false);
	if (newVisibility != pCell->Visibility)
	{
		shouldRedraw = true;
		pCell->Visibility = newVisibility;
	}

	if (pCell->Visibility == -1)
		pCell->AltFlags |= AltCellFlags::NoFog;

	const char newFoggedness = TacticalClass::Instance->GetOcclusion(*pCellStruct, true);
	if (newFoggedness != pCell->Foggedness)
	{
		shouldRedraw = true;
		pCell->Foggedness = newFoggedness;
	}

	if (pCell->Foggedness == -1)
		pCell->Flags |= CellFlags::CenterRevealed;

	if (shouldRedraw)
		TacticalClass::Instance->RegisterCellAsVisible(pCell);

	if ((pCell->AltFlags & AltCellFlags::Mapped) && ScenarioClass::Instance->SpecialFlags.FogOfWar)
	{
		for (size_t i = 0; i < 8; ++i)
		{
			auto adjCell = Unsorted::AdjacentCell[i & 7] + *pCellStruct;
			const auto pAdjCell = pThis->GetCellAt(adjCell);

			if (adjCell != *pCellStruct && !(pAdjCell->Flags & CellFlags::CenterRevealed))
			{
				const char adjNewFoggedness = TacticalClass::Instance->GetOcclusion(adjCell, true);
				if (adjNewFoggedness == -1)
				{
					if (pAdjCell->Flags & CellFlags::EdgeRevealed)
					{
						pAdjCell->Flags |= CellFlags::CenterRevealed;
						TacticalClass::Instance->RegisterCellAsVisible(pAdjCell);

						for (size_t j = 0; j < 8; ++j)
						{
							const auto nextAdjCell = Unsorted::AdjacentCell[j & 7] + adjCell;
							const auto pNextAdjCell = pThis->GetCellAt(nextAdjCell);

							const char nextAdjNewFoggedness = TacticalClass::Instance->GetOcclusion(nextAdjCell, true);
							if (nextAdjNewFoggedness != pNextAdjCell->Foggedness)
							{
								pNextAdjCell->Foggedness = nextAdjNewFoggedness;
								TacticalClass::Instance->RegisterCellAsVisible(pNextAdjCell);
							}
						}

						continue;
					}
				}
				else
				{
					if (adjNewFoggedness == -2 || (pAdjCell->Flags & CellFlags::EdgeRevealed))
					{
						if (adjNewFoggedness >= 0 && adjNewFoggedness != pAdjCell->Foggedness)
						{
							pAdjCell->Foggedness = adjNewFoggedness;
							TacticalClass::Instance->RegisterCellAsVisible(pAdjCell);
						}

						continue;
					}
				}

				pThis->MapCellFoggedness(&adjCell, pHouse);
				continue;
			}
		}
	}

	if (shouldRedraw)
		MapClass::Instance.RevealCheck(pCell, pHouse, shouldRadarRedraw);

	if ((pCell->Flags & CellFlags::EdgeRevealed) && edgeNotRevealed && ScenarioClass::Instance->SpecialFlags.FogOfWar)
		pCell->CleanFog();

	return SkipGameCode;
}

DEFINE_HOOK(0x4ADFF0, MapClass_AllToSee_Reimplement, 0x5)
{
	enum { SkipGameCode = 0x4AE0A5 };

	GET_STACK(bool, notForBuilding, STACK_OFFSET(0x0, 0x4));
	GET_STACK(bool, revealFlag, STACK_OFFSET(0x0, 0x8));

	const auto& vec = TechnoClass::Array;
	for (int i = 0; i < vec.Count; ++i)
	{
		const auto pTechno = vec.Items[i];
		if (pTechno && !pTechno->InLimbo && pTechno->IsOnMap)
		{
			const bool notBuilding = pTechno->WhatAmI() != AbstractType::Building;
			if (notBuilding || !notForBuilding)
			{
				const auto pOwner = pTechno->Owner;
				if (pOwner->IsControlledByCurrentPlayer())
				{
					if (pTechno->DiscoveredByCurrentPlayer)
						pTechno->See(false, revealFlag);
				}
				else if (!notBuilding
					&& RulesClass::Instance->AllyReveal
					&& pOwner->IsAlliedWith(HouseClass::CurrentPlayer))
				{
					pTechno->See(revealFlag, false);
				}
			}
		}
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x4ACBC4, MapClass_FogSpread_SkipWithSpySat, 0x5)
{
	enum { SkipGameCode = 0x4ACC4B, SpreadFogOfWar = 0x4ACBC9 };

	GET(MapClass*, pThis, ECX);

	const auto pPlayer = HouseClass::CurrentPlayer;
	if (pPlayer->Defeated || pPlayer->SpySatActive)
		return SkipGameCode;

	pThis->CellIteratorReset();
	return SpreadFogOfWar;
}

#pragma endregion
