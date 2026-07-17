#include <New/Entity/AttachmentClass.h>

#include <Ext/Scenario/Body.h>
#include <Ext/TerrainType/Body.h>
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

DEFINE_HOOK(0x6B60DE, SmudgeTypeClass_Mark_SetContext, 0x6)
{
	GET(CellClass* const, pCell, EAX);

	ScenarioExt::Global()->Smudges.insert(MapClass::GetCellIndex(pCell->MapCoords));
	const auto pCellExt = CellExt::ExtMap.Find(pCell);
	pCellExt->SmudgeGenerate = Unsorted::CurrentFrame;
	pCellExt->SmudgeState = BlitterFlags::None;

	return 0;
}

DEFINE_HOOK(0x6B56AC, SmudgeTypeClass_DrawIt_DrawTrans, 0x5)
{
	GET(CellClass* const, pCell, ESI);
	REF_STACK(BlitterFlags, flags, STACK_OFFSET(0x3C, -0x3C));

	flags |= CellExt::ExtMap.Find(pCell)->SmudgeState;

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

namespace FoggedObjectHelper
{
	struct FoggedObjectClassFake
	{
		char _[0x40];
		RectangleStruct RenderDimension;
	};
	bool SetVectorCapacity(DynamicVectorClass<FoggedObjectClass*>* pVector, int capacity)
	{
		return reinterpret_cast<bool(__thiscall*)(DynamicVectorClass<FoggedObjectClass*>*, int, FoggedObjectClass **)>(0x45A680)(pVector, capacity, nullptr);
	}
	bool ObjectUnlimbo(ObjectClass* pObject, const CoordStruct& position, DirType face)
	{
		return reinterpret_cast<bool(__thiscall*)(ObjectClass*, const CoordStruct&, DirType)>(0x5F4EC0)(pObject, position, face);
	}
	void AddItemToVector(DynamicVectorClass<FoggedObjectClass*>* pVector, FoggedObjectClass* pItem)
	{
		if(pVector->Count < pVector->Capacity
			|| (pVector->IsAllocated || !pVector->Capacity)
				&& pVector->CapacityIncrement > 0
				&& FoggedObjectHelper::SetVectorCapacity(pVector, pVector->Capacity + pVector->CapacityIncrement))
		{
			pVector->Items[pVector->Count++] = std::move(pItem);
		}
	}
	DynamicVectorClass<FoggedObjectClass*>* CreateGameDynamicVector()
	{
		void* raw = YRMemory::Allocate(0x18u);
		*reinterpret_cast<void**>(raw) = reinterpret_cast<void*>(0x7E44F4u);
		auto pVector = static_cast<DynamicVectorClass<FoggedObjectClass*>*>(raw);
		pVector->Items = nullptr;
		pVector->Capacity = 0;
		pVector->IsInitialized = true;
		pVector->IsAllocated = false;
		pVector->Count = 0;
		pVector->CapacityIncrement = 1;
		FoggedObjectHelper::SetVectorCapacity(pVector, 1);
		return pVector;
	}
	void ClearAllFog()
	{
		MapClass::Instance.CellIteratorReset();
		for (auto pCell = MapClass::Instance.CellIteratorNext(); pCell; pCell = MapClass::Instance.CellIteratorNext())
		{
			pCell->ShroudCounter = 0;
			pCell->GapsCoveringThisCell = 0;
			pCell->AltFlags |= (AltCellFlags::Mapped | AltCellFlags::NoFog);
			pCell->Flags |= (CellFlags::CenterRevealed | CellFlags::EdgeRevealed);
			pCell->Flags &= ~CellFlags::Fogged;
			pCell->ClearFoggedObjects();
		}
		MapClass::Instance.sub_657CE0();
		MapClass::Instance.MarkNeedsRedraw(2);
	}
	static FoggedObjectClass* __fastcall CreateFoggedOverlay(void* pThis, void* _, const CoordStruct& coords, int OverlayTypeIndex, int OverlayData) JMP_THIS(0x4D0980);
	static FoggedObjectClass* __fastcall CreateFoggedSmudge(void* pThis, void* _, const CoordStruct& coords, int SmudgeTypeIndex, int SmudgeData) JMP_THIS(0x4D0C40);
	static FoggedObjectClass* __fastcall CreateFoggedTerrain(void* pThis, void* _, TerrainClass* pTerrain) JMP_THIS(0x4D1370);
	static void __fastcall TechnoClass_RevealLastSight(TechnoClass* pThis, void* _, bool OnlyOutline, bool RevealByHeight, bool specifiedHouse, HouseClass *pHouse)
	{
		pThis->unknown_bool_250 = false;
	}
	class CellClassFake final : public CellClass
	{
	public:
		FoggedObjectClass* FreezeOverlay(DynamicVectorClass<FoggedObjectClass*>* pVector)
		{
			const auto pObject = FoggedObjectHelper::CreateFoggedOverlay(
				YRMemory::Allocate(0x78u),
				0,
				this->GetCoords(),
				this->OverlayTypeIndex,
				static_cast<unsigned int>(this->OverlayData)
			);
			auto rect = reinterpret_cast<FoggedObjectClassFake*>(pObject)->RenderDimension;
			rect.X -= TacticalClass::Instance->TacticalPos.X;
			rect.Y -= TacticalClass::Instance->TacticalPos.Y;
			TacticalClass::Instance->RegisterDirtyArea(rect, 0);
			FoggedObjectHelper::AddItemToVector(pVector, pObject);
			return pObject;
		}
		FoggedObjectClass* FreezeSmudge(DynamicVectorClass<FoggedObjectClass*>* pVector)
		{
			const auto pObject = FoggedObjectHelper::CreateFoggedSmudge(
				YRMemory::Allocate(0x78u),
				0,
				this->GetCoords(),
				this->SmudgeTypeIndex,
				static_cast<unsigned int>(this->SmudgeData)
			);
			auto rect = reinterpret_cast<FoggedObjectClassFake*>(pObject)->RenderDimension;
			rect.X -= TacticalClass::Instance->TacticalPos.X;
			rect.Y -= TacticalClass::Instance->TacticalPos.Y;
			TacticalClass::Instance->RegisterDirtyArea(rect, 0);
			FoggedObjectHelper::AddItemToVector(pVector, pObject);
			return pObject;
		}
	};
	class TerrainClassFake final : public TerrainClass
	{
	public:
		FoggedObjectClass* FreezeTerrain(DynamicVectorClass<FoggedObjectClass*>* pVector)
		{
			const auto pObject = FoggedObjectHelper::CreateFoggedTerrain(
				YRMemory::Allocate(0x78u),
				0,
				this
			);
			auto rect = reinterpret_cast<FoggedObjectClassFake*>(pObject)->RenderDimension;
			rect.X -= TacticalClass::Instance->TacticalPos.X;
			rect.Y -= TacticalClass::Instance->TacticalPos.Y;
			TacticalClass::Instance->RegisterDirtyArea(rect, 0);
			FoggedObjectHelper::AddItemToVector(pVector, pObject);
			return pObject;
		}
		bool TerrainClass_Unlimbo_CheckFog(const CoordStruct& coords, DirType dir)
		{
			const bool result = FoggedObjectHelper::ObjectUnlimbo(this, coords, dir);
			if (result && ScenarioClass::Instance->SpecialFlags.FogOfWar)
			{
				const auto pCell = MapClass::Instance.GetCellAt(coords);
				if (this->IsAlive && (pCell->Flags & CellFlags::Fogged))
				{
					if (!pCell->FoggedObjects)
						pCell->FoggedObjects = FoggedObjectHelper::CreateGameDynamicVector();

					this->FreezeTerrain(pCell->FoggedObjects);
				}
			}
			return result;
		};
	};
	class OverlayClassFake final : public OverlayClass
	{
	public:
		bool OverlayClass_Unlimbo_CheckFog(const CoordStruct& coords, DirType dir)
		{
			const bool result = FoggedObjectHelper::ObjectUnlimbo(this, coords, dir);
			if (result && ScenarioClass::Instance->SpecialFlags.FogOfWar)
			{
				const auto pCell = MapClass::Instance.GetCellAt(coords);
				if (pCell->OverlayTypeIndex != -1 && (pCell->Flags & CellFlags::Fogged))
				{
					if (!pCell->FoggedObjects)
						pCell->FoggedObjects = FoggedObjectHelper::CreateGameDynamicVector();

					static_cast<FoggedObjectHelper::CellClassFake*>(pCell)->FreezeOverlay(pCell->FoggedObjects);
				}
			}
			return result;
		}
	};
	class SmudgeClassFake final : public SmudgeClass
	{
	public:
		bool SmudgeClass_Unlimbo_CheckFog(const CoordStruct& coords, DirType dir)
		{
			const bool result = FoggedObjectHelper::ObjectUnlimbo(this, coords, dir);
			if (result && ScenarioClass::Instance->SpecialFlags.FogOfWar)
			{
				const auto pCell = MapClass::Instance.GetCellAt(coords);
				if (pCell->SmudgeTypeIndex != -1 && (pCell->Flags & CellFlags::Fogged))
				{
					if (!pCell->FoggedObjects)
						pCell->FoggedObjects = FoggedObjectHelper::CreateGameDynamicVector();

					static_cast<FoggedObjectHelper::CellClassFake*>(pCell)->FreezeSmudge(pCell->FoggedObjects);
				}
			}
			return result;
		}
	};
}

// 重新启用功能
DEFINE_HOOK(0x687C56, INIClass_ReadScenario_EnableFog, 0x5)
{
	const bool fog = RulesClass::Instance->FogOfWar;
	GameModeOptionsClass::Instance.FogOfWar = fog;
	ScenarioClass::Instance->SpecialFlags.FogOfWar = fog;
	return 0;
}

// 重新实现核心判定：位置处在迷雾中
DEFINE_HOOK(0x5865E2, MapClass_IsLocationFogged_Reimplement, 0x5)
{
	GET_STACK(const CoordStruct*, pCoords, STACK_OFFSET(0x0, 0x4));

	const int level = pCoords->Z / Unsorted::LevelHeight;
	const int extra = (level & 1) ? ((level >> 1) + 1) : (level >> 1);
	const CellStruct cell { static_cast<short>((pCoords->X >> 8) - extra), static_cast<short>((pCoords->Y >> 8) - extra) };

	R->EAX(!!(MapClass::Instance.GetCellAt(cell)->Flags & CellFlags::Fogged));
	return 0;
}

// 像黑幕那样自动刷新周围格子的绘制状态，防止边缘出现锯齿
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

// 像黑幕刷新那样考虑飞行单位，确保未移动的飞行单位的视野不会被刷新掉，并且友军视野不只看建筑，避免频繁刷新
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
			if (pTechno->WhatAmI() != AbstractType::Building || !notForBuilding)
			{
				const auto pOwner = pTechno->Owner;
				if (pOwner->IsControlledByCurrentPlayer())
				{
					if (pTechno->DiscoveredByCurrentPlayer)
						pTechno->See(false, revealFlag);
				}
				else if (RulesClass::Instance->AllyReveal)
				{
					if (pOwner->IsAlliedWith(HouseClass::CurrentPlayer))
						pTechno->See(revealFlag, false);
				}
			}
		}
	}

	return SkipGameCode;
}

// 增加一种立即刷新迷雾的机制，甚至！？性能提升？！
DEFINE_HOOK(0x4ACBC4, MapClass_FogSpread_SkipWithSpySat, 0x5)
{
	enum { SkipGameCode = 0x4ACC4B, SpreadFogOfWar = 0x4ACBF8 };

	GET(MapClass*, pThis, ECX);

	const auto pPlayer = HouseClass::CurrentPlayer;
	if (pPlayer->Defeated)
		return SkipGameCode;

	pThis->CellIteratorReset();
	if (RulesClass::Instance->ShadowGrow)
	{
		for (auto pCell = pThis->CellIteratorNext(); pCell; pCell = pThis->CellIteratorNext())
			pCell->Flags |= CellFlags::IsPlot;
	}
	else
	{
		for (auto pCell = pThis->CellIteratorNext(); pCell; pCell = pThis->CellIteratorNext())
		{
			const auto flags = pCell->Flags;
			if (!(flags & CellFlags::CenterRevealed) && (flags & CellFlags::EdgeRevealed))
				pCell->Flags = flags | CellFlags::IsPlot;
		}
	}

	return SpreadFogOfWar;
}

// 让单位在更新视野的时候不会多更新一次之前的视野
DEFINE_FUNCTION_JUMP(CALL6, 0x415672, FoggedObjectHelper::TechnoClass_RevealLastSight);
DEFINE_FUNCTION_JUMP(CALL6, 0x4157F7, FoggedObjectHelper::TechnoClass_RevealLastSight);
DEFINE_FUNCTION_JUMP(CALL6, 0x416C6D, FoggedObjectHelper::TechnoClass_RevealLastSight);
DEFINE_FUNCTION_JUMP(CALL6, 0x4CD43F, FoggedObjectHelper::TechnoClass_RevealLastSight);
DEFINE_FUNCTION_JUMP(CALL6, 0x4DA6F7, FoggedObjectHelper::TechnoClass_RevealLastSight);
DEFINE_FUNCTION_JUMP(CALL6, 0x51A8D7, FoggedObjectHelper::TechnoClass_RevealLastSight);
DEFINE_FUNCTION_JUMP(CALL6, 0x54C93C, FoggedObjectHelper::TechnoClass_RevealLastSight);
DEFINE_FUNCTION_JUMP(CALL6, 0x73AC5F, FoggedObjectHelper::TechnoClass_RevealLastSight);

// 迷雾遮蔽时增加地形对象、覆盖物、弹坑
DEFINE_HOOK(0x486B21, CellClass_FogCell_FreezeObjects, 0x6)
{
	enum { SkipGameCode = 0x486BAB };

	GET(FoggedObjectHelper::CellClassFake*, pCell, EBP);
	GET(DynamicVectorClass<FoggedObjectClass*>*, pVector, EDI);

    pCell->Flags |= CellFlags::Fogged;
	for (auto pObject = pCell->FirstObject; pObject; pObject = pObject->NextObject)
	{
		const auto absType = pObject->WhatAmI();
		if (absType == AbstractType::Building)
		{
			const auto& pBuilding = static_cast<BuildingClass*>(pObject);
			if (pBuilding->IsAllFogged())
				pBuilding->FreezeInFog(pVector, pCell, (!pBuilding->IsStrange() && pBuilding->Translucency != 15));
		}
		else if (absType == AbstractType::Unit
			|| absType == AbstractType::Aircraft
			|| absType == AbstractType::Infantry)
		{
			pObject->Deselect();
		}
		else if (absType == AbstractType::Terrain)
		{
			static_cast<FoggedObjectHelper::TerrainClassFake*>(pObject)->FreezeTerrain(pVector);
		}
	}

	if (pCell->OverlayTypeIndex != -1)
		pCell->FreezeOverlay(pVector);

	if (pCell->SmudgeTypeIndex != -1)
		pCell->FreezeSmudge(pVector);

	return SkipGameCode;
}

// 地形对象被迷雾遮蔽时跳过绘制
DEFINE_HOOK(0x6D9313, TacticalClass_DrawObjectsInLayers_SkipTerrainInFog, 0x6)
{
	enum { SkipDraw = 0x6D940C };

	GET(const CoordStruct*, pCoord, EAX);

	return (ScenarioClass::Instance->SpecialFlags.FogOfWar && MapClass::Instance.IsLocationFogged(*pCoord)) ? SkipDraw : 0;
}

// 覆盖物被迷雾遮蔽时跳过绘制
DEFINE_HOOK(0x6D70BC, TacticalClass_DrawCellOverlay_SkipOverlayInFog, 0xA)
{
	enum { Draw = 0x6D70C6, SkipDraw = 0x6D71A4 };

	GET(const CellClass*, pCell, EBX);

	return (pCell->OverlayTypeIndex != -1 && (!ScenarioClass::Instance->SpecialFlags.FogOfWar || !(pCell->Flags & CellFlags::Fogged))) ? Draw : SkipDraw;
}

// 弹坑被迷雾遮蔽时跳过绘制
DEFINE_HOOK(0x48049E, CellClass_DrawCellSmudge_SkipSmudgeInFog, 0x6)
{
	enum { Draw = 0x4804A4, SkipDraw = 0x4804FB };

	GET(const CellClass*, pThis, ESI);

	return (pThis->SmudgeTypeIndex != -1 && (!ScenarioClass::Instance->SpecialFlags.FogOfWar || !(pThis->Flags & CellFlags::Fogged))) ? Draw : SkipDraw;
}

DEFINE_HOOK(0x4FC200, HouseClass_AcceptDefeat_RevealFog, 0x5)
{
	if (ScenarioClass::Instance->SpecialFlags.FogOfWar)
		FoggedObjectHelper::ClearAllFog();

	return 0;
}

// 地形对象、覆盖物、弹坑出现时检查是否被迷雾遮蔽
DEFINE_FUNCTION_JUMP(CALL, 0x5FC4B1, FoggedObjectHelper::OverlayClassFake::OverlayClass_Unlimbo_CheckFog);
DEFINE_FUNCTION_JUMP(CALL, 0x6B4B14, FoggedObjectHelper::SmudgeClassFake::SmudgeClass_Unlimbo_CheckFog);
DEFINE_FUNCTION_JUMP(CALL, 0x71D012, FoggedObjectHelper::TerrainClassFake::TerrainClass_Unlimbo_CheckFog);

DEFINE_HOOK(0x4D1B2E, FoggedObjectClass_DrawFoggedObjects_DrawTerrain, 0x6)
{
	enum { SkipGameCode = 0x4D2311 };

	GET(SHPStruct*, pImage, EAX);
	GET(const TerrainTypeClass*, pType, ECX);
	GET(const CoordStruct*, pCoord, ESI);
	GET(const RectangleStruct*, pBounds, EDI);
	GET(CellClass*, pCell, EBP);
	GET_STACK(int, frame, STACK_OFFSET(0x154, -0x134));

	if (pImage)
	{
		auto point = TacticalClass::Instance->CoordsToClient(*pCoord).first;
		point.X += (DSurface::ViewBounds.X - pBounds->X);
		point.Y += (DSurface::ViewBounds.Y - pBounds->Y);
		int zAdjust = -TacticalClass::Instance->AdjustForZ(pCoord->Z);

		if (!pCell->LightConvert)
			pCell->InitLightConvert();

		ConvertClass* pPalette = pCell->LightConvert;
		int intensity = static_cast<int>(pCell->Intensity_Terrain);
		if (const auto pPalettes = TerrainTypeExt::ExtMap.Find(pType)->Palette)
		{
			const int wallOwnerIndex = pCell->WallOwnerIndex;
			int colorSchemeIndex = HouseClass::CurrentPlayer->ColorSchemeIndex;

			if (wallOwnerIndex >= 0)
				colorSchemeIndex = HouseClass::Array[wallOwnerIndex]->ColorSchemeIndex;

			pPalette = pPalettes->Items[colorSchemeIndex]->LightConvert;
			intensity = static_cast<int>(pCell->Intensity_Normal);

			if (pType->SpawnsTiberium)
				point.Y -= 16;
		}
		else if (pType->SpawnsTiberium)
		{
			pPalette = FileSystem::GRFTXT_TIBERIUM_PAL;
			intensity = static_cast<int>(pCell->Intensity_Normal);
			point.Y -= 16;
		}

		DSurface::Temp->DrawSHP(pPalette, pImage, frame, &point, pBounds, BlitterFlags(0x4E00), 0, zAdjust - 4, ZGradient::Deg90, intensity, 0, 0, 0, 0, 0);
		if (Game::bDrawShadow)
			DSurface::Temp->DrawSHP(pPalette, pImage, frame + (pImage->Frames / 2), &point, pBounds, BlitterFlags(0x4601), 0, zAdjust - 2, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}

	return SkipGameCode;
}

// 矿石不更新
// 修复Alpha光亮度异常
// 波动效果要受影响
// 光照强度要受影响
// 线尾迹要受影响
// 激光要受影响
// 电弧要受影响
// 辐射波要受影响
// 波要受影响
// 飞碟激光要受影响

#pragma endregion
