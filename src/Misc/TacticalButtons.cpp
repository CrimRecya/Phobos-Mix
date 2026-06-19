#include "TacticalButtons.h"

#include <SuperClass.h>
#include <AircraftClass.h>
#include <TacticalClass.h>
#include <MouseClass.h>
#include <WWMouseClass.h>
#include <AITriggerTypeClass.h>
#include <JumpjetLocomotionClass.h>
#include <HoverLocomotionClass.h>
#include <InputManagerClass.h>

#include <Ext/WarheadType/Body.h>
#include <Utilities/TemplateDef.h>
#include <Locomotion/AStar/AStarClass.h>

// TacticalButtonsClass TacticalButtonsClass::Instance;

// Functions
/*
#pragma region PrivateFunctions

int TacticalButtonsClass::CheckMouseOverButtons(const Point2D* pMousePosition)
{
	// TODO New buttons

	if (this->CheckMouseOverBackground(pMousePosition))
		return 0; // Button index 0 : Background

	return -1;
}

bool TacticalButtonsClass::CheckMouseOverBackground(const Point2D* pMousePosition)
{
	// TODO New button backgrounds

	return false;
}

#pragma endregion

#pragma region InlineFunctions

inline bool TacticalButtonsClass::MouseIsOverButtons()
{
	return this->ButtonIndex > 0;
}

inline bool TacticalButtonsClass::MouseIsOverTactical()
{
	return this->ButtonIndex < 0;
}

#pragma endregion

#pragma region CiteFunctions

int TacticalButtonsClass::GetButtonIndex()
{
	return this->ButtonIndex;
}

#pragma endregion

#pragma region GeneralFunctions

void TacticalButtonsClass::SetMouseButtonIndex(const Point2D* pMousePosition)
{
	this->ButtonIndex = this->CheckMouseOverButtons(pMousePosition);

	// TODO New buttons
}

void TacticalButtonsClass::PressDesignatedButton(int triggerIndex)
{
	if (!this->MouseIsOverButtons()) // In buttons background
		return;

	// TODO New buttons
}

#pragma endregion
*/
#pragma region ShowCurrentInfo

void TacticalButtonsClass::CurrentSelectInfoDraw()
{
	if (!Phobos::ShowCurrentInfo)
		return;

	constexpr COLORREF color_orange = 0xFC00;
	constexpr BlitterFlags blit = BlitterFlags::Centered | BlitterFlags::TransLucent50 | BlitterFlags::bf_400 | BlitterFlags::Zero;

	const auto mouseXY1 = WWMouseClass::Instance->XY1;
	auto cell = CellStruct::Empty;
	auto coords = CoordStruct::Empty;
	ObjectClass* pObj = nullptr;
	{
		auto point = mouseXY1 - Point2D { DSurface::ViewBounds.X, DSurface::ViewBounds.Y };
		BYTE fogged = 0;
		BYTE shrouded = 0;
		DisplayClass::Instance.ProcessClickCoords(&point, &cell, &coords, &pObj, &fogged, &shrouded);
	}

	auto getTechnoForDraw = [&pObj]() -> TechnoClass*
	{
		if (ObjectClass::CurrentObjects.Count > 0)
		{
			for (const auto& pCurrent : ObjectClass::CurrentObjects)
			{
				if (const auto pTechno = abstract_cast<TechnoClass*>(pCurrent))
					return pTechno;
			}
		}

		if (const auto pTechno = abstract_cast<TechnoClass*>(pObj))
			return pTechno;

		return nullptr;
	};
	const auto pTechno = getTechnoForDraw();

	if (pTechno)
	{
		struct TempCellData { const CellClass* Cell; int Level; };
		auto compare = [](const TempCellData DataA, const TempCellData DataB)
			{
				if (DataA.Level != DataB.Level)
					return DataA.Level < DataB.Level;

				if (DataA.Cell->MapCoords.X != DataB.Cell->MapCoords.X)
					return DataA.Cell->MapCoords.X < DataB.Cell->MapCoords.X;

				return DataA.Cell->MapCoords.Y < DataB.Cell->MapCoords.Y;
			};
		std::vector<TempCellData> pathCells;
		pathCells.reserve(30);

		if (const auto pFoot = abstract_cast<FootClass*, true>(pTechno))
		{
			const auto pJjLoco = locomotion_cast<JumpjetLocomotionClass*>(pFoot->Locomotor);
			const auto pFlyLoco = locomotion_cast<FlyLocomotionClass*>(pFoot->Locomotor);

			if (pJjLoco || pFlyLoco)
			{
				if (InputManagerClass::Instance->IsForceFireKeyPressed() && (pJjLoco ? (pJjLoco->CurrentSpeed > 0.0) : (pFlyLoco && pFlyLoco->CurrentSpeed > 0.0)))
				{
					const auto pDestination = pFoot->Destination;
					auto curCoord = Point2D { pFoot->Location.X, pFoot->Location.Y };
					auto pCurCell = MapClass::Instance.GetCellAt(CellStruct { static_cast<short>(curCoord.X >> 8), static_cast<short>(curCoord.Y >> 8) });
					pathCells.emplace_back(pCurCell, pCurCell->GetLevel());
					const auto pFace = pJjLoco ? &pJjLoco->LocomotionFacing : &pFoot->PrimaryFacing;
					const int distance = pFoot->DistanceFrom(pDestination);
					const int checkLength = (pFace->IsRotating() || !pDestination) ? 256 : Math::min((256 * 12), distance);
					const double angle = -pFace->Current().GetRadian<65536>();
					const auto checkCoord = Point2D { static_cast<int>(checkLength * cos(angle)), static_cast<int>(checkLength * sin(angle)) };
					const int largeStep = Math::max(abs(checkCoord.X), abs(checkCoord.Y));
					const int checkSteps = (largeStep > 256) ? (largeStep / 256 + 1) : 1;
					const auto stepCoord = Point2D { (checkCoord.X / checkSteps), (checkCoord.Y / checkSteps) };

					for (int i = 0; i < checkSteps; ++i)
					{
						const auto lastCoord = curCoord;
						curCoord += stepCoord;
						pCurCell = MapClass::Instance.TryGetCellAt(CellStruct { static_cast<short>(curCoord.X >> 8), static_cast<short>(curCoord.Y >> 8) });

						if (!pCurCell)
							break;

						if (std::ranges::find_if(pathCells, [pCurCell](auto data){ return data.Cell == pCurCell; }) == pathCells.end())
							pathCells.emplace_back(pCurCell, pCurCell->GetLevel());

						if ((curCoord.X >> 8) != (lastCoord.X >> 8) && (curCoord.Y >> 8) != (lastCoord.Y >> 8))
						{
							bool lastX = false;

							if (std::abs(stepCoord.X) > std::abs(stepCoord.Y))
							{
								const int offsetX = curCoord.X & 0xFF;
								const int deltaX = (stepCoord.X > 0) ? offsetX : (offsetX - Unsorted::LeptonsPerCell);
								const int projectedY = curCoord.Y - deltaX * checkCoord.Y / checkCoord.X;
								lastX = (projectedY ^ curCoord.Y) >> 8 == 0;
							}
							else
							{
								const int offsetY = curCoord.Y & 0xFF;
								const int deltaY = (stepCoord.Y > 0) ? offsetY : (offsetY - Unsorted::LeptonsPerCell);
								const int projectedX = curCoord.X - deltaY * checkCoord.X / checkCoord.Y;
								lastX = (projectedX ^ curCoord.X) >> 8 != 0;
							}

							if (const auto pCheckCell = MapClass::Instance.TryGetCellAt(lastX
								? CellStruct { static_cast<short>(lastCoord.X >> 8), static_cast<short>(curCoord.Y >> 8) }
								: CellStruct { static_cast<short>(curCoord.X >> 8), static_cast<short>(lastCoord.Y >> 8) }))
							{
								if (std::ranges::find_if(pathCells, [pCheckCell](auto data){ return data.Cell == pCheckCell; }) == pathCells.end())
									pathCells.emplace_back(pCheckCell, pCheckCell->GetLevel());
							}
						}
					}

					if (pCurCell && checkSteps > 1)
					{
						const int height = pJjLoco ? pJjLoco->Height : pFoot->GetTechnoType()->GetFlightLevel();
						CoordStruct drawCoords { curCoord.X, curCoord.Y, (height + pCurCell->Level * 104) };

						if (checkLength == distance)
						{
							const auto pAircraft = abstract_cast<AircraftClass*, true>(pFoot);

							if (!pAircraft || !TechnoTypeExt::ExtMap.Find(pAircraft->Type)->ExtendedAircraftMissions_RearApproach.Get(RulesExt::Global()->ExtendedAircraftMissions)
								|| !pDestination || (pAircraft->DockNowHeadingTo != pDestination && pAircraft->SpawnOwner != pDestination))
							{
								const auto destination = pFoot->Locomotor->Destination();

								if (destination != CoordStruct::Empty)
								{
									drawCoords.X = destination.X;
									drawCoords.Y = destination.Y;
								}
							}
						}

						TechnoExt::DrawExtraImage(pFoot, pCurCell, drawCoords, pFace->DesiredFacing);
					}
				}
				else
				{
					const auto pFootCell = pFoot->GetCell();
					pathCells.emplace_back(pFootCell, pFootCell->GetLevel());
				}
			}
			else
			{
				const auto pFootCell = pFoot->GetCell();
				int cellLevel = (pFootCell->ContainsBridge() && pFoot->OnBridge) ? (pFootCell->Level + 4) : pFootCell->Level;
				pathCells.emplace_back(pFootCell, cellLevel);

				if (InputManagerClass::Instance->IsForceFireKeyPressed())
				{
					auto pCell = MapClass::Instance.GetCellAt(pFoot->Locomotor->Head_To_Coord());
					cellLevel = (pCell->ContainsBridge() && cellLevel == (pCell->Level + 4)) ? (pCell->Level + 4) : pCell->Level;

					if (pCell != pFootCell)
						pathCells.emplace_back(pCell, cellLevel);

					const auto& pD = pFoot->PathDirections;
					int face = pD[0];

					if (face > -1 && face < 8)
					{
						pCell = pCell->GetNeighbourCell(static_cast<FacingType>(face));
						cellLevel = (pCell->ContainsBridge() && cellLevel == (pCell->Level + 4)) ? cellLevel : pCell->Level;
						pathCells.emplace_back(pCell, cellLevel);

						for (int i = 1; i < 24; ++i)
						{
							if (pCell->Flags & CellFlags::Tube)
								break;

							const int thisFace = pD[i];

							if (thisFace <= -1 || thisFace >= 8)
								break;

							face = thisFace;
							pCell = pCell->GetNeighbourCell(static_cast<FacingType>(face));
							cellLevel = (pCell->ContainsBridge() && cellLevel == (pCell->Level + 4)) ? cellLevel : pCell->Level;
							pathCells.emplace_back(pCell, cellLevel);
						}

						int height = (pCell->ContainsBridge() && cellLevel == (pCell->Level + 4)) ? CellClass::BridgeHeight : 0;

						if (locomotion_cast<HoverLocomotionClass*>(pFoot->Locomotor))
						{
							height += RulesClass::Instance->HoverHeight;
						}
						else if (locomotion_cast<AdvancedDriveLocomotionClass*>(pFoot->Locomotor))
						{
							const auto pTypeExt = TechnoExt::ExtMap.Find(pFoot)->TypeExtData;

							if (pTypeExt->AdvancedDrive_Hover)
								height += pTypeExt->AdvancedDrive_Hover_Height.Get(RulesClass::Instance->HoverHeight);
						}

						CoordStruct drawCoords = pCell->GetCoords();
						drawCoords.Z += height;
						TechnoExt::DrawExtraImage(pFoot, pCell, drawCoords, DirStruct(face << 13));
					}
				}
				else if (!InputManagerClass::Instance->IsForceMoveKeyPressed())
				{
					auto pCell = MapClass::Instance.GetCellAt(pFoot->Locomotor->Head_To_Coord());
					cellLevel = (pCell->ContainsBridge() && cellLevel == (pCell->Level + 4)) ? (pCell->Level + 4) : pCell->Level;

					if (pCell != pFootCell)
						pathCells.emplace_back(pCell, cellLevel);
				}
			}
		}
		else if (const auto pBuilding = abstract_cast<BuildingClass*, true>(pTechno))
		{
			const auto pType = pBuilding->Type;

			if (InputManagerClass::Instance->IsForceFireKeyPressed())
			{
				const auto pBase = &pBuilding->Owner->Base;

				for (const auto& baseCell : pBase->Cells_24)
				{
					if (baseCell == CellStruct::Empty)
						continue;

					const auto pBaseCell = MapClass::Instance.GetCellAt(baseCell);
					pathCells.emplace_back(pBaseCell, pBaseCell->GetLevel());
				}
			}
			else if (InputManagerClass::Instance->IsForceMoveKeyPressed())
			{
				const auto baseCell = pBuilding->GetMapCoords();

				for (auto pFoundation = pType->GetFoundationData(true); *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
				{
					const auto pBaseCell = MapClass::Instance.GetCellAt(baseCell + *pFoundation);
					pathCells.emplace_back(pBaseCell, pBaseCell->GetLevel());
				}
			}
			else if (InputManagerClass::Instance->IsForceSelectKeyPressed())
			{
				const auto pHouse = pBuilding->Owner;
				const auto pBase = &pHouse->Base;

				for (const auto& baseNode : pBase->BaseNodes)
				{
					if (baseNode.MapCoords == CellStruct::Empty)
						continue;

					const auto pBaseCell = MapClass::Instance.GetCellAt(baseNode.MapCoords);
					pathCells.emplace_back(pBaseCell, pBaseCell->GetLevel());

					const auto pBaseType = BuildingTypeClass::Array.GetItemOrDefault(baseNode.BuildingTypeIndex, nullptr);

					if (!pBaseType)
						continue;

					const auto pCellBuilding = pBaseCell->GetBuilding();

					if (pCellBuilding && pCellBuilding->Type == pBaseType)
						continue;

					auto pImage = pBaseType->LoadBuildup();
					int frame = 0;

					if (pImage)
						frame = ((pImage->Frames / 2) - 1);
					else if (pImage = pBaseType->GetImage(), !pImage)
						continue;

					const int height = 1 + (pBaseCell->Level * Unsorted::LevelHeight);
					const auto pair = TacticalClass::Instance->CoordsToClient(CellClass::Cell2Coord(pBaseCell->MapCoords, height));

					if (!pair.second)
						continue;

					auto point = pair.first - Point2D { 0, 15 };
					constexpr auto blitFlags = BlitterFlags::TransLucent50 | BlitterFlags::Centered | BlitterFlags::Nonzero | BlitterFlags::MultiPass;

					const int index = pHouse->ColorSchemeIndex;
					const auto pPalette = pBaseType->Palette ? pBaseType->Palette->GetItem(index)->LightConvert : ColorScheme::Array.GetItem(index)->LightConvert;

					DSurface::Temp->DrawSHP(pPalette, pImage, frame, &point, &DSurface::ViewBounds, blitFlags, 0, 0, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
				}
			}
			else
			{
				const auto baseCell = pBuilding->GetMapCoords();

				for (auto pFoundation = pType->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
				{
					const auto pBaseCell = MapClass::Instance.GetCellAt(baseCell + *pFoundation);
					pathCells.emplace_back(pBaseCell, pBaseCell->GetLevel());
				}
			}
		}

		if (const auto cellsSize = pathCells.size())
		{
			std::sort(&pathCells[0], &pathCells[cellsSize], compare);

			for (const auto& data : pathCells)
			{
				const auto location = CoordStruct { (data.Cell->MapCoords.X << 8), (data.Cell->MapCoords.Y << 8), 0 };
				const int height = data.Level * 15;
				const auto position = TacticalClass::Instance->CoordsToScreen(location) - TacticalClass::Instance->TacticalPos - Point2D { 0, (1 + height) };
				const bool notOnBridge = data.Level == data.Cell->Level;
				const int frameIndex = (notOnBridge && data.Cell->SlopeIndex)
					? (data.Cell->SlopeIndex + 2)
					: ((notOnBridge ? (data.Cell->FirstObject || (data.Cell->OccupationFlags & 0xFF)) : (data.Cell->AltObject || (data.Cell->AltOccupationFlags & 0xFF))) ? 1 : 0);
				const int zAdjust = -height - ((notOnBridge && data.Cell->SlopeIndex) ? 12 : 2) - 16384;

				DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, Make_Global<SHPStruct*>(0x8A03FC), frameIndex, &position,
					&DSurface::ViewBounds, blit, 0, zAdjust, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
			}
		}
	}

	const auto pCell = MapClass::Instance.GetCellAt(DisplayClass::Instance.CurrentFoundation_CenterCell);
	{
		auto centerCell = pCell->MapCoords;
		{
			const auto location = CoordStruct { (centerCell.X << 8), (centerCell.Y << 8), 0 };

			if (pCell->ContainsBridge())
			{
				const int height = (pCell->Level + 4) * 15;
				const auto position = TacticalClass::Instance->CoordsToScreen(location) - TacticalClass::Instance->TacticalPos - Point2D { 0, (1 + height) };
				const int zAdjust = -height - (pCell->SlopeIndex ? 12 : 2) - 16384;

				DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, Make_Global<SHPStruct*>(0x8A03FC), 2, &position,
					&DSurface::ViewBounds, blit, 0, zAdjust, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
			}

			const int height = pCell->Level * 15;
			const auto position = TacticalClass::Instance->CoordsToScreen(location) - TacticalClass::Instance->TacticalPos - Point2D { 0, (1 + height) };
			const int frameIndex = pCell->SlopeIndex ? (pCell->SlopeIndex + 2) : ((pCell->FirstObject || (pCell->OccupationFlags & 0xFF)) ? 1 : 0);
			const int zAdjust = -height - (pCell->SlopeIndex ? 12 : 2) - 16384;

			DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, Make_Global<SHPStruct*>(0x8A03FC), frameIndex, &position,
				&DSurface::ViewBounds, blit, 0, zAdjust, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		}

		struct TempCellData { const CellClass* Cell; int Level; };
		auto compare = [](const TempCellData DataA, const TempCellData DataB)
		{
			if (DataA.Level != DataB.Level)
				return DataA.Level < DataB.Level;

			if (DataA.Cell->MapCoords.X != DataB.Cell->MapCoords.X)
				return DataA.Cell->MapCoords.X < DataB.Cell->MapCoords.X;

			return DataA.Cell->MapCoords.Y < DataB.Cell->MapCoords.Y;
		};
		std::vector<TempCellData> checkCells;
		checkCells.reserve(30);

		if (InputManagerClass::Instance->IsForceMoveKeyPressed())
		{
			centerCell += CellStruct { 12, 12 };

			auto checkInvisibleBarrier = [](CellClass* pCheckCell, bool alt) -> bool
			{
				if (alt ? pCheckCell->AltObject : pCheckCell->FirstObject)
					return false;

				const DWORD flags = alt ? pCheckCell->AltOccupationFlags : pCheckCell->OccupationFlags;

				if (!(0xFF & flags))
					return false;

				if (0xC0 & flags)
					return true;

				auto checkCell = pCheckCell->MapCoords + CellStruct { 2, 2 };

				for (short checkX = checkCell.X - 4; checkX <= checkCell.X; ++checkX)
				{
					for (short checkY = checkCell.Y - 4; checkY <= checkCell.Y; ++checkY)
					{
						const auto pAdjCheckCell = MapClass::Instance.GetCellAt(CellStruct { checkX, checkY });

						for (auto pObject = alt ? pAdjCheckCell->AltObject : pAdjCheckCell->FirstObject; pObject; pObject = pObject->NextObject)
						{
							const auto absType = pObject->WhatAmI();

							if (absType == AbstractType::Infantry || absType == AbstractType::Unit)
							{
								if (CellClass::Coord2Cell(static_cast<FootClass*>(pObject)->Locomotor->Head_To_Coord()) == pCheckCell->MapCoords)
									return false;
							}
						}
					}
				}

				return true;
			};

			for (short checkX = centerCell.X - 24; checkX <= centerCell.X; ++checkX)
			{
				for (short checkY = centerCell.Y - 24; checkY <= centerCell.Y; ++checkY)
				{
					if (const auto pCheckCell = MapClass::Instance.TryGetCellAt(CellStruct { checkX, checkY }))
					{
						if (checkInvisibleBarrier(pCheckCell, false))
							checkCells.emplace_back(pCheckCell, pCheckCell->Level);

						if (pCheckCell->ContainsBridge() && checkInvisibleBarrier(pCheckCell, true))
							checkCells.emplace_back(pCheckCell, pCheckCell->Level + CellClass::BridgeLevels);
					}
				}
			}
		}
		else if (InputManagerClass::Instance->IsForceSelectKeyPressed())
		{
			const int baseIndex = MapClass::Instance.GetCellPathIndex(centerCell);
			const auto& baseData = MapClass::Instance.LevelAndPassabilityStruct2pointer_70[baseIndex];
			centerCell += CellStruct { 7, 7 };

			auto checkSameSubzoneIndex = [&baseData](CellClass* pCheckCell, int level) -> bool
			{
				const int pathIndex = MapClass::Instance.GetCellPathIndex(pCheckCell->MapCoords);
				return baseData.word_0[level] == MapClass::Instance.LevelAndPassabilityStruct2pointer_70[pathIndex].word_0[level];
			};

			for (short checkX = centerCell.X - 14; checkX <= centerCell.X; ++checkX)
			{
				for (short checkY = centerCell.Y - 14; checkY <= centerCell.Y; ++checkY)
				{
					if (const auto pCheckCell = MapClass::Instance.TryGetCellAt(CellStruct { checkX, checkY }))
					{
						if (checkSameSubzoneIndex(pCheckCell, 0))
							checkCells.emplace_back(pCheckCell, pCheckCell->Level);

						if (checkSameSubzoneIndex(pCheckCell, 1))
							checkCells.emplace_back(pCheckCell, pCheckCell->Level);

						if (checkSameSubzoneIndex(pCheckCell, 2))
							checkCells.emplace_back(pCheckCell, pCheckCell->Level);
					}
				}
			}
		}

		if (const auto cellsSize = checkCells.size())
		{
			std::sort(&checkCells[0], &checkCells[cellsSize], compare);

			for (const auto& data : checkCells)
			{
				const auto location = CoordStruct { (data.Cell->MapCoords.X << 8), (data.Cell->MapCoords.Y << 8), 0 };
				const int height = data.Level * 15;
				const auto position = TacticalClass::Instance->CoordsToScreen(location) - TacticalClass::Instance->TacticalPos - Point2D { 0, (1 + height) };
				const int frameIndex = data.Cell->SlopeIndex ? (data.Cell->SlopeIndex + 2) : 1;
				const int zAdjust = -height - (data.Cell->SlopeIndex ? 12 : 2) - 16384;

				DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, Make_Global<SHPStruct*>(0x8A03FC), frameIndex, &position,
					&DSurface::ViewBounds, blit, 0, zAdjust, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
			}
		}
	}

	if (pTechno)
	{
		const auto pTactical = TacticalClass::Instance;
		const auto technoCoord = pTechno->GetRenderCoords();
		const auto point = pTactical->CoordsToScreen(technoCoord) - pTactical->TacticalPos;
		auto drawMtxLine = [pTactical, &technoCoord](const Matrix3D& mtx, const Point2D& point, const COLORREF color)
		{
			const auto result = mtx.GetTranslation();
			const auto location = CoordStruct { (int)result.X, -(int)result.Y, (int)result.Z };
			auto point1 = point;
			auto point2 = pTactical->CoordsToScreen(technoCoord + location) - pTactical->TacticalPos;
			auto rect = DSurface::ViewBounds;
			rect.Height -= 32;
			DSurface::Composite->DrawLineEx(&rect, &point1, &point2, color);
		};

		const auto thisPoint = TacticalClass::Instance->CoordsToClient(pTechno->GetCoords()).first;
		const auto offset = (0x7FFFFFFF - Unsorted::CurrentFrame) % 15;
		bool pattern[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
		auto drawDashLine = [&thisPoint, &pattern, &offset](AbstractClass* pDest, int color)
		{
			if (pDest)
			{
				auto coords = pDest->GetCoords();
				const auto pCell = MapClass::Instance.TryGetCellAt(coords);

				if (!pCell)
					return;

				if (pCell->ContainsBridge())
					coords.Z += CellClass::BridgeHeight;

				auto footPoint = thisPoint;
				auto destPoint = TacticalClass::Instance->CoordsToClient(coords).first;
				DSurface::Composite->DrawDashed(&footPoint, &destPoint, color, offset, pattern);
				--footPoint.Y;
				--destPoint.Y;
				DSurface::Composite->DrawDashed(&footPoint, &destPoint, color, offset, pattern);
				--footPoint.Y;
				--destPoint.Y;
				DSurface::Composite->DrawDashed(&footPoint, &destPoint, color, offset, pattern);
			}
		};

		if (const auto pFoot = abstract_cast<FootClass*, true>(pTechno))
		{
			drawDashLine(pFoot->Target, COLOR_RED);
			drawDashLine(pFoot->ArchiveTarget, COLOR_PURPLE);
			drawDashLine(pFoot->GetNthLink(), COLOR_WHITE);
			drawDashLine(pFoot->QueueUpToEnter, COLOR_BLUE);
			drawDashLine(pFoot->unknown_5A0, COLOR_YELLOW);
			drawDashLine(pFoot->Destination, COLOR_GREEN);
			drawDashLine(pFoot->MegaTarget, COLOR_CYAN);
			drawDashLine(pFoot->MegaDestination, COLOR_CYAN);

			if (InputManagerClass::Instance->IsForceSelectKeyPressed())
			{
				const auto mtxBase = pFoot->Locomotor ? pFoot->Locomotor->Draw_Matrix(nullptr) : Matrix3D::GetIdentity();
				const auto rotateRadian = pTechno->PrimaryFacing.Current().GetRadian<32>();

				auto mtx = mtxBase;
				mtx.RotateZ((float)(pTechno->PrimaryFacing.StartFacing.GetRadian<32>() - rotateRadian));
				mtx.TranslateX(512.0f);
				drawMtxLine(mtx, point, COLOR_PURPLE);

				mtx = mtxBase;
				mtx.RotateZ((float)(pTechno->PrimaryFacing.DesiredFacing.GetRadian<32>() - rotateRadian));
				mtx.TranslateX(512.0f);
				drawMtxLine(mtx, point, COLOR_RED);

				mtx = mtxBase;
				// mtx.RotateZ((float)rotateRadian); // No need to rotate again
				mtx.TranslateX(512.0f);
				drawMtxLine(mtx, point, COLOR_GREEN);

				const auto absType = pTechno->WhatAmI();
				const auto pTechnoType = pTechno->GetTechnoType();

				if (absType == AbstractType::Unit && pTechnoType->Turret || absType == AbstractType::Aircraft)
				{
					auto mtxTur = mtxBase;
					TechnoTypeExt::ApplyTurretOffset(pTechnoType, &mtxTur, 1.0);

					const auto turret = mtxTur.GetTranslation();
					const auto turretPoint = pTactical->CoordsToScreen(technoCoord + CoordStruct{(int)turret.X,-(int)turret.Y,(int)turret.Z}) - pTactical->TacticalPos;

					mtx = mtxTur;
					mtx.RotateZ((float)(pTechno->SecondaryFacing.StartFacing.GetRadian<32>() - rotateRadian));
					mtx.TranslateX(512.0f);
					drawMtxLine(mtx, turretPoint, COLOR_BLUE);

					mtx = mtxTur;
					mtx.RotateZ((float)(pTechno->SecondaryFacing.DesiredFacing.GetRadian<32>() - rotateRadian));
					mtx.TranslateX(512.0f);
					drawMtxLine(mtx, turretPoint, COLOR_YELLOW);

					mtx = mtxTur;
					mtx.RotateZ((float)(pTechno->SecondaryFacing.Current().GetRadian<32>() - rotateRadian));
					mtx.TranslateX(512.0f);
					drawMtxLine(mtx, turretPoint, COLOR_WHITE);
				}
			}
		}
		else
		{
			drawDashLine(pTechno->Target, COLOR_RED);
			drawDashLine(pTechno->ArchiveTarget, COLOR_PURPLE);
			drawDashLine(pTechno->GetNthLink(), COLOR_WHITE);
			drawDashLine(pTechno->QueueUpToEnter, COLOR_BLUE);

			if (InputManagerClass::Instance->IsForceSelectKeyPressed())
			{
				const auto mtxBase = Matrix3D::GetIdentity();
				const auto rotateRadian = pTechno->PrimaryFacing.Current().GetRadian<32>();

				auto mtx = mtxBase;
				mtx.RotateZ((float)rotateRadian);
				mtx.TranslateX(512.0f);
				drawMtxLine(mtx, point, COLOR_WHITE);

				mtx = mtxBase;
				mtx.RotateZ((float)(pTechno->PrimaryFacing.StartFacing.GetRadian<32>() - rotateRadian));
				mtx.TranslateX(512.0f);
				drawMtxLine(mtx, point, COLOR_BLUE);

				mtx = mtxBase;
				mtx.RotateZ((float)(pTechno->PrimaryFacing.DesiredFacing.GetRadian<32>() - rotateRadian));
				mtx.TranslateX(512.0f);
				drawMtxLine(mtx, point, COLOR_YELLOW);
			}
		}
	}

	RectangleStruct drawRect { 0, 0, 360, DSurface::Composite->GetHeight() - 32 };
	{
		ColorStruct fillColor { 0, 0, 0 };
		DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 30);
	}
	Point2D textLocation { 15, 5 };
	bool loc = false;

	auto updateLine = [&loc, &textLocation]()
	{
		loc = !loc;

		if (!loc)
			textLocation.Y += 12;
	};

	auto drawText = [&loc, &updateLine, &drawRect, &textLocation](COLORREF color, const char* pFormat, ...)
	{
		char buffer[0x60] = {0};
		va_list args;
		va_start(args, pFormat);
		vsprintf_s(buffer, pFormat, args);
		va_end(args);
		wchar_t wBuffer[0x60] = {0};
		CRT::mbstowcs(wBuffer, buffer, strlen(buffer));
		constexpr TextPrintType printType = TextPrintType::FullShadow | TextPrintType::Point8;
		textLocation.X = loc ? 188 : 8;
		DSurface::Composite->DrawTextA(wBuffer, &drawRect, &textLocation, color, 0, printType);
		updateLine();
	};

	auto drawInfo = [&drawText](const char* pInfoName, AbstractClass* pCurrent, AbstractClass* pTarget)
	{
		if (pTarget)
		{
			auto mapCoords = CellStruct::Empty;
			auto ID = "N/A";

			if (auto const pObject = abstract_cast<ObjectClass*, true>(pTarget))
			{
				mapCoords = pObject->GetMapCoords();
				ID = pObject->GetType()->get_ID();
			}
			else if (auto const pCell = abstract_cast<CellClass*, true>(pTarget))
			{
				mapCoords = pCell->MapCoords;
				ID = "CELL";
			}

			drawText(COLOR_GREEN, "%s: %s(%03d,%03d)[%dC]", pInfoName, ID, mapCoords.X, mapCoords.Y, (pCurrent->DistanceFrom(pTarget) / 256));
		}
		else
		{
			drawText(COLOR_RED, "%s: %s", pInfoName, "N/A");
		}
	};

	auto drawTask = [&drawText](const char* pInfoName, Mission mission)
	{
		drawText(COLOR_YELLOW, "%s: (%02d)[%s]", pInfoName, mission, MissionControlClass::FindName(mission));
	};

	auto drawTime = [&drawText](const char* pInfoName, CDTimerClass& timer)
	{
		const auto timeCeiling = timer.TimeLeft;
		const auto timeCurrent = timeCeiling - timer.GetTimeLeft();
		const auto timePercentage = (timeCeiling > 0) ? (timeCurrent * 100 / timeCeiling) : 0;

		drawText(COLOR_PURPLE, "%s: (%d/%d)[%03d]", pInfoName, timeCurrent, timeCeiling, timePercentage);
	};

	drawText(COLOR_WHITE, "Current Frame: %d", Unsorted::CurrentFrame);
	{
		drawText(COLOR_WHITE, "Address: 0x%08X", reinterpret_cast<DWORD>(pCell));
		drawText(COLOR_WHITE, "Cell: %d", MapClass::Instance.GetCellIndex(pCell->MapCoords));
		drawText(COLOR_WHITE, "UniqueID: %d", pCell->UniqueID);

		{
			constexpr const char* landTypes[12] = { "Clear", "Road", "Water", "Rock", "Wall", "Tiberium", "Beach", "Rough", "Ice", "Railroad", "Tunnel", "Weeds" };
			const auto landType = static_cast<int>(pCell->LandType);

			drawText(COLOR_WHITE, "LandType: ( %s )", (landType >= 0 && landType < 12) ? landTypes[landType] : "Unknown");
			drawText(COLOR_WHITE, "Slope: ( %d )", pCell->SlopeIndex);
		}

		drawText(color_orange, "Location: (%05d,%05d,%05d)[%03d,%03d,%02d]", coords.X, coords.Y, coords.Z, cell.X, cell.Y, pCell->GetLevel());
		updateLine();

		{
			const auto nCF = static_cast<DWORD>(pCell->Flags);

			drawText(COLOR_WHITE, "CellFlags: %d%d%d %d%d%d%d %d%d%d%d %d%d%d%d %d%d%d%d %d%d%d%d",
				((nCF >> 22) & 0x1), ((nCF >> 21) & 0x1), ((nCF >> 20) & 0x1), ((nCF >> 19) & 0x1), ((nCF >> 18) & 0x1), ((nCF >> 17) & 0x1), ((nCF >> 16) & 0x1),
				((nCF >> 15) & 0x1), ((nCF >> 14) & 0x1), ((nCF >> 13) & 0x1), ((nCF >> 12) & 0x1), ((nCF >> 11) & 0x1), ((nCF >> 10) & 0x1), ((nCF >> 9) & 0x1), ((nCF >> 8) & 0x1),
				((nCF >> 7) & 0x1), ((nCF >> 6) & 0x1), ((nCF >> 5) & 0x1), ((nCF >> 4) & 0x1), ((nCF >> 3) & 0x1), ((nCF >> 2) & 0x1), ((nCF >> 1) & 0x1), (nCF & 0x1));

			updateLine();
		}

		{
			const auto nOF = pCell->OccupationFlags;
			const auto nAF = pCell->AltOccupationFlags;

			drawText(COLOR_WHITE, "TheOccupationFlags: %d%d%d%d %d%d%d%d", ((nOF >> 7) & 0x1), ((nOF >> 6) & 0x1), ((nOF >> 5) & 0x1), ((nOF >> 4) & 0x1), ((nOF >> 3) & 0x1), ((nOF >> 2) & 0x1), ((nOF >> 1) & 0x1), (nOF & 0x1));
			drawText(COLOR_WHITE, "AltOccupationFlags: %d%d%d%d %d%d%d%d", ((nAF >> 7) & 0x1), ((nAF >> 6) & 0x1), ((nAF >> 5) & 0x1), ((nAF >> 4) & 0x1), ((nAF >> 3) & 0x1), ((nAF >> 2) & 0x1), ((nAF >> 1) & 0x1), (nAF & 0x1));
		}

		drawText(COLOR_WHITE, "TubeIndex: %d", pCell->TubeIndex);
		drawText(COLOR_WHITE, "Passability: %d", static_cast<int>(pCell->Passability));
		drawText(COLOR_WHITE, "BlockedNearby: %d", pCell->BlockedNeighbours);

		{
			const int pathIndex = MapClass::Instance.GetCellPathIndex(pCell->MapCoords);
			const auto& passabilityData1 = MapClass::Instance.LevelAndPassability[pathIndex];
			const auto& passabilityData2 = MapClass::Instance.LevelAndPassabilityStruct2pointer_70[pathIndex];
			const int level0SubzoneIndex = static_cast<unsigned short>(passabilityData2.word_0[0]);
			const int level1SubzoneIndex = static_cast<unsigned short>(passabilityData2.word_0[1]);
			const int level2SubzoneIndex = static_cast<unsigned short>(passabilityData2.word_0[2]);
			const int zoneIndex = passabilityData1.ZoneArrayIndex;

			drawText(COLOR_WHITE, "PathData: %d", pathIndex);
			drawText(COLOR_WHITE, "PathIndexes: {%d},{%d},{%d},{%d}", level0SubzoneIndex, level1SubzoneIndex, level2SubzoneIndex, zoneIndex);
			updateLine();

			const int searchID = AStarClass::Instance.SearchID;

			drawText(COLOR_WHITE, "PathSearchCost: [%d,%d]%.4f , [%d,%d]%.4f , [%d,%d]%.4f",
				((AStarClass::Instance.LevelVisitedMarkers[0][level0SubzoneIndex] == searchID) ? 1 : 0), ((AStarClass::Instance.OpenSetMarkers[0][level0SubzoneIndex] == searchID) ? 1 : 0), AStarClass::Instance.GCostArray[0][level0SubzoneIndex],
				((AStarClass::Instance.LevelVisitedMarkers[1][level1SubzoneIndex] == searchID) ? 1 : 0), ((AStarClass::Instance.OpenSetMarkers[1][level1SubzoneIndex] == searchID) ? 1 : 0), AStarClass::Instance.GCostArray[1][level1SubzoneIndex],
				((AStarClass::Instance.LevelVisitedMarkers[2][level2SubzoneIndex] == searchID) ? 1 : 0), ((AStarClass::Instance.OpenSetMarkers[2][level2SubzoneIndex] == searchID) ? 1 : 0), AStarClass::Instance.GCostArray[2][level2SubzoneIndex]);
			updateLine();
		}

		drawText(COLOR_WHITE, "OccupyHeights: %d", pCell->OccupyHeightsCoveringMe);
		drawText(COLOR_WHITE, "RadLevel: %.2f", pCell->RadLevel);

		{
			const auto pOwner = HouseClass::Array.GetItemOrDefault(pCell->WallOwnerIndex);

			drawText(COLOR_WHITE, "WallOwner: %s(%s)", (pOwner ? pOwner->get_ID() : "N/A"), (pOwner ? pOwner->PlainName : "N/A"));
			drawInfo("CurrentJumpjet", pCell, pCell->Jumpjet);
		}

		{
			int count = 0;
			int index = 0;

			for (auto pCellObj = pCell->FirstObject; pCellObj; pCellObj = pCellObj->NextObject)
				++count;

			drawText(COLOR_WHITE, "TheObjects: (Ground)[%d]", count);

			if (!count)
				updateLine();

			for (auto pCellObj = pCell->FirstObject; pCellObj; pCellObj = pCellObj->NextObject, ++index)
				drawText(COLOR_GREEN, "TheObject(%d)[%s]", index, pCellObj->GetType()->get_ID());

			if (loc)
				updateLine();
		}

		{
			int count = 0;
			int index = 0;

			for (auto pCellObj = pCell->AltObject; pCellObj; pCellObj = pCellObj->NextObject)
				++count;

			drawText(COLOR_WHITE, "AltObjects: (Bridge)[%d]", count);

			if (!count)
				updateLine();

			for (auto pCellObj = pCell->AltObject; pCellObj; pCellObj = pCellObj->NextObject, ++index)
				drawText(COLOR_GREEN, "AltObject(%d)[%s]", index, pCellObj->GetType()->get_ID());

			if (loc)
				updateLine();
		}

		updateLine();
		updateLine();

		const auto pMouse = &MouseClass::Instance;

		drawText(COLOR_WHITE, "Mouse: (%04d,%04d)", mouseXY1.X, mouseXY1.Y);
		drawText(COLOR_WHITE, "RadarScope: (%03d,%03d,%02d,%02d)", pMouse->RadarScopeRect.X, pMouse->RadarScopeRect.Y, pMouse->RadarScopeRect.Width, pMouse->RadarScopeRect.Height);
	}

	if (pTechno)
	{
		drawText(COLOR_WHITE, "Current Select Techno:");
		drawText(COLOR_WHITE, "Address: 0x%08X", reinterpret_cast<DWORD>(pTechno));

		const auto pType = pTechno->GetTechnoType();
		const auto absType = pTechno->WhatAmI();

		if (absType == AbstractType::Unit)
			drawText(COLOR_WHITE, "%s: %s", "Vehicle", pType->ID);
		else if (absType == AbstractType::Infantry)
			drawText(COLOR_WHITE, "%s: %s", "Infantry", pType->ID);
		else if (absType == AbstractType::Aircraft)
			drawText(COLOR_WHITE, "%s: %s", "Aircraft", pType->ID);
		else if (absType == AbstractType::Building)
			drawText(COLOR_WHITE, "%s: %s", "Building", pType->ID);
		else
			drawText(COLOR_WHITE, "%s: %s", "Unknown", pType->ID);

		drawText(COLOR_WHITE, "UniqueID: %d", pTechno->UniqueID);

		const auto pOwner = pTechno->Owner;
		{
			const auto pOrigin = pTechno->GetOriginalOwner();

			drawText(COLOR_WHITE, "Owner: %s(Player<%d>)", (pOwner ? pOwner->get_ID() : "N/A"), (pOwner ? pOwner->ArrayIndex : -1));
			drawText(COLOR_WHITE, "Origin: %s(Player<%d>)", (pOrigin ? pOrigin->get_ID() : "N/A"), (pOrigin ? pOrigin->ArrayIndex : -1));
		}

		{
			const auto cellT = pTechno->GetMapCoords();
			const auto coordsT = pTechno->GetCoords();

			drawText(color_orange, "Location: (%05d,%05d,%05d)[%03d,%03d,%02d]", coordsT.X, coordsT.Y, coordsT.Z, cellT.X, cellT.Y, pTechno->GetCellLevel());
			updateLine();
		}

		{
			constexpr const char* facingTypes[8] = { "North", "NorthEast", "East", "SouthEast", "South", "SouthWest", "West", "NorthWest" };
			const auto facing1 = pTechno->PrimaryFacing.Current();
			const auto facing11 = pTechno->PrimaryFacing.StartFacing;
			const auto facing12 = pTechno->PrimaryFacing.DesiredFacing;

			drawText(COLOR_CYAN, "PrimaryFacing: (%05d[%02d])[%s]", facing1.Raw, facing1.GetValue<5>(), facingTypes[facing1.GetValue<3>()]);
			updateLine();

			drawText(COLOR_CYAN, "PriStartFacing: (%05d)", facing11.Raw);
			drawText(COLOR_CYAN, "PriDesiredFacing: (%05d)", facing12.Raw);

			const auto facing2 = pTechno->SecondaryFacing.Current();
			const auto facing21 = pTechno->SecondaryFacing.StartFacing;
			const auto facing22 = pTechno->SecondaryFacing.DesiredFacing;

			drawText(COLOR_CYAN, "SecondaryFacing: (%05d[%02d])[%s]", facing2.Raw, facing2.GetValue<5>(), facingTypes[facing2.GetValue<3>()]);
			updateLine();

			drawText(COLOR_CYAN, "SecStartFacing: (%05d)", facing21.Raw);
			drawText(COLOR_CYAN, "SecDesiredFacing: (%05d)", facing22.Raw);
		}

		const auto pExt = TechnoExt::ExtMap.Find(pTechno);

		drawText(COLOR_WHITE, "Ammo: (%d/%d)", pTechno->Ammo, pType->Ammo);
		drawText(COLOR_WHITE, "Tether: (%s,%s)", (pTechno->IsTether ? "Yes" : "No"), (pTechno->IsAlternativeTether ? "Yes" : "No"));

		drawText(COLOR_WHITE, "Health: (%d/%d)", pTechno->Health, pType->Strength);
		drawText(COLOR_WHITE, "Shield: (%d/%d)", (pExt->Shield ? pExt->Shield->GetHP() : -1), (pExt->CurrentShieldType ? pExt->CurrentShieldType->Strength : -1));

		drawTime("Reload", pTechno->ReloadTimer);
		drawTime("Rearm", pTechno->RearmTimer);

		drawTime("Update", pTechno->UpdateTimer);
		drawInfo("Bunker Linked", pTechno, pTechno->BunkerLinkedItem);

		drawInfo("Target", pTechno, pTechno->Target);
		drawText(COLOR_WHITE, "TargetInRange: %s", (pTechno->IsCloseEnough(pTechno->Target, pTechno->SelectWeapon(pTechno->Target)) ? "Yes" : "No"));

		drawInfo("First CurTarget", pTechno, (pTechno->CurrentTargets.Count > 0 ? pTechno->CurrentTargets.GetItem(0) : nullptr));
		drawInfo("First OldTarget", pTechno, (pTechno->AttackedTargets.Count > 0 ? pTechno->AttackedTargets.GetItem(0) : nullptr));

		drawInfo("Last Target", pTechno, pTechno->LastTarget);
		drawInfo("Enter Target", pTechno, pTechno->QueueUpToEnter);

		drawInfo("Archive Target", pTechno, pTechno->ArchiveTarget);
		drawInfo("Transporter", pTechno, pTechno->Transporter);

		{
			const bool bySize = pExt->TypeExtData->Passengers_BySize;
			const int count = pTechno->Passengers.NumPassengers;
			const int capacity = pType->Passengers;
			ObjectClass* pPassenger = pTechno->Passengers.GetFirstPassenger();
			int currentSize = 0;
			drawText(COLOR_WHITE, "Passengers: (%d/%d)[%d]", (bySize ? pTechno->Passengers.GetTotalSize() : count), capacity, count);

			if (capacity > 1)
				updateLine();

			for (int i = 0; i < capacity; ++i)
			{
				if (currentSize > i)
				{
					drawText(COLOR_WHITE, "Passenger(%d)[%s]", i, "---");
					continue;
				}

				if (pPassenger)
				{
					if (const auto pPassengerType = pPassenger->GetTechnoType())
					{
						drawText(COLOR_GREEN, "Passenger(%d)[%s]", i, pPassengerType->ID);
						currentSize += bySize ? static_cast<int>(pPassengerType->Size) : 1;
						pPassenger = pPassenger->NextObject;
						continue;
					}

					pPassenger = pPassenger->NextObject;
				}

				drawText(COLOR_RED, "Passenger(%d)[%s]", i, "N/A");
			}

			if (loc)
				updateLine();
		}

		{
			const int capacity = pTechno->RadioLinks.Capacity;
			int count = 0;

			for (int i = 0; i < capacity; ++i)
			{
				if (pTechno->RadioLinks.Items[i])
					++count;
			}

			drawText(COLOR_WHITE, "RadioLinks: (%d/%d)", count, capacity);

			if (capacity > 1)
				updateLine();

			for (int i = 0; i < capacity; ++i)
			{
				if (const auto pLink = pTechno->RadioLinks.Items[i])
					drawText(COLOR_GREEN, "RadioLink(%d)[%s]", i, pLink->GetType()->ID);
				else
					drawText(COLOR_RED, "RadioLink(%d)[%s]", i, "N/A");
			}

			if (loc)
				updateLine();
		}

		drawText(COLOR_CYAN, "Cache: [1](%d/%d),[2](%d/%d),[3](%d/%d),[4](%d/%d)", pType->VoxelMainCache.IndexCount, pType->VoxelMainCache.IndexSize, pType->VoxelTurretWeaponCache.IndexCount, pType->VoxelTurretWeaponCache.IndexSize, pType->VoxelShadowCache.IndexCount, pType->VoxelShadowCache.IndexSize, pType->VoxelTurretBarrelCache.IndexCount, pType->VoxelTurretBarrelCache.IndexSize);
		updateLine();

		drawText(COLOR_WHITE, "TurretRecoil: %.3f", pTechno->TurretRecoil.TravelSoFar);
		drawText(COLOR_WHITE, "BarrelRecoil: %.3f", pTechno->BarrelRecoil.TravelSoFar);

		drawTask("Mission", pTechno->CurrentMission);
		drawText(COLOR_YELLOW, "Status: %d , Start: %d", pTechno->MissionStatus, pTechno->CurrentMissionStartTime);

		drawTask("Suspend", pTechno->SuspendedMission);
		drawTask("Queued", pTechno->QueuedMission);

		if (const auto pFoot = abstract_cast<FootClass*, true>(pTechno))
		{
			drawTask("Mega", pFoot->MegaMission);
			drawInfo("Parasite", pFoot, pFoot->ParasiteEatingMe);

			drawInfo("Destination", pFoot, pFoot->Destination);
			drawInfo("Last Destination", pFoot, pFoot->LastDestination);

			drawInfo("Mega Destination", pFoot, pFoot->MegaDestination);
			drawInfo("Mega Target", pFoot, pFoot->MegaTarget);

			drawInfo("Follow Target", pFoot, pFoot->unknown_5A0);
			drawInfo("Patrol Target", pFoot, pFoot->unknown_5DC);

			drawInfo("First ArrayItem", pFoot, (pFoot->unknown_abstract_array_588.Count > 0 ? pFoot->unknown_abstract_array_588.GetItem(0) : nullptr));
			drawInfo("First NavQueue", pFoot, (pFoot->NavQueue.Count > 0 ? pFoot->NavQueue.GetItem(0) : nullptr));

			drawText(color_orange, "FootCell: (%03d,%03d)", pFoot->CurrentMapCoords.X, pFoot->CurrentMapCoords.Y);
			drawText(color_orange, "LastCell: (%03d,%03d)", pFoot->LastMapCoords.X, pFoot->LastMapCoords.Y);

			{
				const auto destination = pFoot->Locomotor->Destination();
				const auto headToCoord = pFoot->Locomotor->Head_To_Coord();

				drawText(color_orange, "LocoDest: (%05d,%05d,%05d)", destination.X, destination.Y, destination.Z);
				drawText(color_orange, "LocoHead: (%05d,%05d,%05d)", headToCoord.X, headToCoord.Y, headToCoord.Z);
			}

			drawText(color_orange, "MovingState: (%s,%s,%s)", (pFoot->Locomotor->Is_Moving() ? "Yes" : "No"), (pFoot->Locomotor->Is_Moving_Now() ? "Yes" : "No"), (pFoot->Locomotor->Is_Really_Moving_Now() ? "Yes" : "No"));
			drawText(color_orange, "LocoPowered: %s", (pFoot->Locomotor->Is_Powered() ? "Yes" : "No"));

			{
				constexpr const char* moveTypes[8] = { "Clear", "Cloak", "Move", "Gate", "A-Block", "E-Block", "Temp", "Unable" };
				const auto pLastCell = MapClass::Instance.GetCellAt(pFoot->LastMapCoords);
				const auto faceType = pFoot->PrimaryFacing.Current().GetValue<3>();
				const auto pNextCell = MapClass::Instance.GetCellAt(Unsorted::AdjacentCell[faceType] + pLastCell->MapCoords);
				const auto moveType = static_cast<int>(pFoot->IsCellOccupied(pNextCell, static_cast<FacingType>(faceType), pLastCell->Level + (pFoot->OnBridge ? 4 : 0), pLastCell, true));

				drawText(COLOR_WHITE, "PlanningPathIdx: %d", pFoot->PlanningPathIdx);
				drawText(COLOR_WHITE, "FaceMoveType: (%s)", (moveType >= 0 && moveType < 8) ? moveTypes[moveType] : "N/A");
			}

			const auto& pD = pFoot->PathDirections;

			if (pD[0] == -1)
				drawText(COLOR_CYAN, "PathDir: N/A");
			else if (pD[1] == -1)
				drawText(COLOR_CYAN, "PathDir: %d", pD[0]);
			else if (pD[2] == -1)
				drawText(COLOR_CYAN, "PathDir: %d-%d", pD[0], pD[1]);
			else if (pD[3] == -1)
				drawText(COLOR_CYAN, "PathDir: %d-%d-%d", pD[0], pD[1], pD[2]);
			else if (pD[4] == -1)
				drawText(COLOR_CYAN, "PathDir: %d-%d-%d-%d", pD[0], pD[1], pD[2], pD[3]);
			else if (pD[5] == -1)
				drawText(COLOR_CYAN, "PathDir: %d-%d-%d-%d-%d", pD[0], pD[1], pD[2], pD[3], pD[4]);
			else if (pD[6] == -1)
				drawText(COLOR_CYAN, "PathDir: %d-%d-%d-%d-%d-%d", pD[0], pD[1], pD[2], pD[3], pD[4], pD[5]);
			else if (pD[7] == -1)
				drawText(COLOR_CYAN, "PathDir: %d-%d-%d-%d-%d-%d-%d", pD[0], pD[1], pD[2], pD[3], pD[4], pD[5], pD[6]);
			else
				drawText(COLOR_CYAN, "PathDir: %d-%d-%d-%d-%d-%d-%d-%d", pD[0], pD[1], pD[2], pD[3], pD[4], pD[5], pD[6], pD[7]);

			updateLine();

			drawText(COLOR_WHITE, "CurrentSpeed: %d", static_cast<int>(pFoot->GetCurrentSpeed()));
			drawText(COLOR_WHITE, "PercentSpeed: %d", static_cast<int>(pFoot->SpeedPercentage * 100));

			drawText(COLOR_WHITE, "OnElevatedBridge: %s", (pFoot->OnBridge ? "Yes" : "No"));
			drawText(COLOR_WHITE, "NearElevatedBridge: %s", (pFoot->IsNearBridge() ? "Yes" : "No"));

			drawText(COLOR_WHITE, "OnBacklit: %s", (pFoot->vt_entry_2B0() ? "Yes" : "No"));
			drawText(COLOR_WHITE, "IsCrushing: %s", (pFoot->IsCrushingSomething ? "Yes" : "No"));

			drawText(COLOR_WHITE, "Scattering: %s", (pExt->ScatteringStopFrame >= Unsorted::CurrentFrame ? "Yes" : "No"));
			drawText(COLOR_WHITE, "Aggressive: %s", (pExt->AggressiveStance ? "Yes" : "No"));

			if (pFoot->BelongsToATeam())
			{
				const auto pTeam = pFoot->Team;
				const auto pTeamType = pTeam->Type;
				AITriggerTypeClass* pTriggerType = nullptr;

				for (int i = 0; i < AITriggerTypeClass::Array.Count; i++)
				{
					const auto pAITT = AITriggerTypeClass::Array.GetItem(i);

					if (pTeamType && (pAITT->Team1 == pTeamType || pAITT->Team2 == pTeamType))
					{
						pTriggerType = pAITT;
						break;
					}
				}

				const auto pScriptType = pTeam->CurrentScript->Type;
				const auto mission = pTeam->CurrentScript->CurrentMission;

				drawText(COLOR_YELLOW, "Trigger: %s", (pTriggerType ? pTriggerType->ID : "N/A"));
				drawText(COLOR_YELLOW, "Team: %s", pTeamType->ID);

				drawText(COLOR_YELLOW, "Task: %s", pTeamType->TaskForce->ID);
				drawText(COLOR_YELLOW, "Script: %s", pScriptType->get_ID());

				drawText(COLOR_YELLOW, "Weights [Cur,Min,Max] -");

				if (pTriggerType)
					drawText(COLOR_YELLOW, "[%.2f,%.2f,%.2f]", pTriggerType->Weight_Current, pTriggerType->Weight_Minimum, pTriggerType->Weight_Maximum);
				else
					drawText(COLOR_YELLOW, "[%.2f,%.2f,%.2f]", -1.0, -1.0, -1.0);

				drawText(COLOR_YELLOW, "Script [Line=Act,Arg] -");
				drawText(COLOR_YELLOW, "[%d=%d,%d]", mission, (mission >= 0 ? pScriptType->ScriptActions[mission].Action : -1), (mission >= 0 ? pScriptType->ScriptActions[mission].Argument : -1));
			}
			else
			{
				drawText(COLOR_YELLOW, "Trigger: %s", "N/A");
				drawText(COLOR_YELLOW, "Team: %s", "N/A");

				drawText(COLOR_YELLOW, "Task:", "N/A");
				drawText(COLOR_YELLOW, "Script: %s", "N/A");

				drawText(COLOR_YELLOW, "Weights [Cur,Min,Max] -");
				drawText(COLOR_YELLOW, "[%.2f,%.2f,%.2f]", -1.0, -1.0, -1.0);

				drawText(COLOR_YELLOW, "Script [Line=Act,Arg] -");
				drawText(COLOR_YELLOW, "[%d=%d,%d]", -1, -1, -1);
			}

			if (absType == AbstractType::Unit)
			{
				const auto pUnit = static_cast<UnitClass*>(pTechno);

				drawInfo("Follower", pUnit, pUnit->FollowerCar);
			}
			else if (absType == AbstractType::Infantry)
			{
				const auto pInfantry = static_cast<InfantryClass*>(pTechno);

				drawTime("UnknownTimer", pInfantry->unknown_Timer_6C8);
			}
			else if (absType == AbstractType::Aircraft)
			{
				const auto pAircraft = static_cast<AircraftClass*>(pTechno);

				drawInfo("Dock Building", pAircraft, pAircraft->DockNowHeadingTo);
			}
		}
		else if (const auto pBuilding = abstract_cast<BuildingClass*, true>(pTechno))
		{
			const auto pBuildingType = pBuilding->Type;
			const auto pBuildingTypeExt = BuildingTypeExt::ExtMap.Find(pBuildingType);

			{
				const int capacity = pBuildingType->MaxNumberOccupants;
				const int count = pBuilding->Occupants.Count;

				drawText(COLOR_WHITE, "Occupants: (%d/%d)", count, capacity);

				if (capacity > 1)
					updateLine();

				for (int i = 0; i < capacity; ++i)
				{
					if (i < count)
						drawText(COLOR_GREEN, "Occupant(%d)[%s]", i, pBuilding->Occupants.GetItem(i)->Type->ID);
					else
						drawText(COLOR_RED, "Occupant(%d)[%s]", i, "N/A");
				}

				if (loc)
					updateLine();
			}

			{
				const int capacity = pBuildingType->Overpowerable ? (pBuildingTypeExt->Overpower_ChargeWeapon + (pOwner->Is_Powered() ? 0 : pBuildingTypeExt->Overpower_KeepOnline)) : 0;
				const int overpower = pBuilding->Overpowerers.Count;
				std::vector<std::pair<TechnoClass*, int>> chargers;
				chargers.reserve(overpower);
				int count = 0;

				for (int i = 0; i < overpower; ++i)
				{
					const auto pCharger = pBuilding->Overpowerers.GetItem(i);

					if (pCharger->Target == pBuilding)
					{
						if (const auto pWeapon = pBuilding->Overpowerers.GetItem(i)->GetWeapon(1)->WeaponType)
						{
							if (const auto pWH = pWeapon->Warhead)
							{
								if (pWH->ElectricAssault)
								{
									const int charge = WarheadTypeExt::ExtMap.Find(pWH)->ElectricAssaultLevel;
									count += charge;
									chargers.emplace_back(pCharger, charge);
								}
							}
						}
					}
				}

				drawText(COLOR_WHITE, "Overpowers: (%d/%d)[%d]", count, capacity, overpower);

				if (capacity > 1)
					updateLine();

				int currentOverpower = 0;
				int currentCharger = 0;

				for (int i = 0; i < capacity; ++i)
				{
					if (currentOverpower > i)
					{
						drawText(COLOR_WHITE, "Overpower(%d)[%s]", i, "---");
						continue;
					}

					if (currentCharger < static_cast<int>(chargers.size()))
					{
						const auto chargerPair = chargers[currentCharger];
						drawText(COLOR_GREEN, "Overpower(%d)[%s]", i, chargerPair.first->GetType()->ID);
						currentOverpower += chargerPair.second;
						++currentCharger;
						continue;
					}

					drawText(COLOR_RED, "Overpower(%d)[%s]", i, "N/A");
				}

				if (loc)
					updateLine();
			}

			{
				FactoryClass* pFactory = nullptr;
				TechnoClass* pProduct = nullptr;

				if (!pOwner->IsControlledByHuman())
				{
					if (pFactory = pBuilding->Factory, pFactory)
						pProduct = pFactory->Object;
				}
				else if (pBuilding->IsPrimaryFactory)
				{
					if (pFactory = pOwner->GetPrimaryFactory(pBuildingType->Factory, pType->Naval, BuildCat::DontCare), pFactory)
						pProduct = pFactory->Object;

					if ((!pFactory || !pProduct) && pBuildingType->Factory == AbstractType::BuildingType && (pFactory = pOwner->Primary_ForDefenses, pFactory))
						pProduct = pFactory->Object;
				}

				if (pFactory && pProduct)
					drawText(COLOR_PURPLE, "Product: (%s)[%d] {%d}", pProduct->GetTechnoType()->ID, (pFactory->GetProgress() * 100 / 54), (pFactory->QueuedObjects.Count + (pProduct ? 1 : 0)));
				else
					drawText(COLOR_PURPLE, "Product: (%s)[%d] {%d}", "N/A", 0, (pFactory ? pFactory->QueuedObjects.Count : 0));

				drawTime("RetryProduction", pBuilding->FactoryRetryTimer);
			}

			drawTime("CashProduction", pBuilding->CashProductionTimer);
			drawTime("BuildingGate", pBuilding->GateTimer);

			drawText(COLOR_WHITE, "BaseNodes: %d", pOwner->Base.BaseNodes.Count);
			drawText(COLOR_WHITE, "BaseCenter: (%03d,%03d)", pOwner->Base.Center.X, pOwner->Base.Center.Y);

			{
				SuperClass* pSuper = nullptr;

				if (pBuildingType->SuperWeapon != -1)
					pSuper = pOwner->Supers.GetItem(pBuildingType->SuperWeapon);
				else if (pBuildingType->SuperWeapon2 != -1)
					pSuper = pOwner->Supers.GetItem(pBuildingType->SuperWeapon2);
				else if (pBuildingTypeExt->SuperWeapons.size() > 0)
					pSuper = pOwner->Supers.GetItem(pBuildingTypeExt->SuperWeapons[0]);

				if (pSuper)
					drawTime(pSuper->Type->ID, pSuper->RechargeTimer);
				else
					drawText(COLOR_PURPLE, "SuperWeapon: (0/-1)[000]");

				updateLine();
			}

			// Upgrade Status
			if (const auto upgrades = pBuildingType->Upgrades)
			{
				const auto pType1 = pBuilding->Upgrades[0];
				const auto pType2 = pBuilding->Upgrades[1];
				const auto pType3 = pBuilding->Upgrades[2];

				drawText(COLOR_WHITE, "Upgrades: (%d/%d)", pBuilding->UpgradeLevel, upgrades);
				drawText(COLOR_WHITE, "Slot-1: %s", (pType1 ? pType1->ID : "N/A"));
				drawText(COLOR_WHITE, "Slot-2: %s", (pType2 ? pType2->ID : "N/A"));
				drawText(COLOR_WHITE, "Slot-3: %s", (pType3 ? pType3->ID : "N/A"));
			}
			else
			{
				drawText(COLOR_WHITE, "Upgrades: (%d/%d)", -1, -1);
				drawText(COLOR_WHITE, "Slot-1: %s", "N/A");
				drawText(COLOR_WHITE, "Slot-2: %s", "N/A");
				drawText(COLOR_WHITE, "Slot-3: %s", "N/A");
			}
		}
	}
}

#pragma endregion

// Hooks
/*
#pragma region MouseTriggerHooks

DEFINE_HOOK(0x6931A5, ScrollClass_WindowsProcedure_PressLeftMouseButton, 0x6)
{
	enum { SkipGameCode = 0x6931B4 };

	const auto pButtons = &TacticalButtonsClass::Instance;

	if (!pButtons->MouseIsOverTactical())
	{
		pButtons->PressedInButtonsLayer = true;
		pButtons->PressDesignatedButton(0);

		R->Stack(STACK_OFFSET(0x28, 0x8), 0);
		R->EAX(Action::None);
		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x693268, ScrollClass_WindowsProcedure_ReleaseLeftMouseButton, 0x5)
{
	enum { SkipGameCode = 0x693276 };

	const auto pButtons = &TacticalButtonsClass::Instance;

	if (pButtons->PressedInButtonsLayer)
	{
		pButtons->PressedInButtonsLayer = false;
		pButtons->PressDesignatedButton(1);

		R->Stack(STACK_OFFSET(0x28, 0x8), 0);
		R->EAX(Action::None);
		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x69330E, ScrollClass_WindowsProcedure_PressRightMouseButton, 0x6)
{
	enum { SkipGameCode = 0x69334A };

	const auto pButtons = &TacticalButtonsClass::Instance;

	if (!pButtons->MouseIsOverTactical())
	{
		pButtons->PressedInButtonsLayer = true;
		pButtons->PressDesignatedButton(2);

		return SkipGameCode;
	}

	return 0;
}

DEFINE_HOOK(0x693397, ScrollClass_WindowsProcedure_ReleaseRightMouseButton, 0x6)
{
	enum { SkipGameCode = 0x6933CB };

	const auto pButtons = &TacticalButtonsClass::Instance;

	if (pButtons->PressedInButtonsLayer)
	{
		pButtons->PressedInButtonsLayer = false;
		pButtons->PressDesignatedButton(3);

		return SkipGameCode;
	}

	return 0;
}

#pragma endregion

#pragma region MouseSuspendHooks

DEFINE_HOOK(0x692F85, ScrollClass_MouseUpdate_SkipMouseLongPress, 0x7)
{
	enum { CheckMousePress = 0x692F8E, CheckMouseNoPress = 0x692FDC };

	GET(ScrollClass*, pThis, EBX);

	// 555A: AnyMouseButtonDown
	return (pThis->AnyMouseButtonDown && !TacticalButtonsClass::Instance.PressedInButtonsLayer) ? CheckMousePress : CheckMouseNoPress;
}

DEFINE_HOOK(0x69300B, ScrollClass_MouseUpdate_SkipMouseActionUpdate, 0x6)
{
	enum { SkipGameCode = 0x69301A };

	const auto mousePosition = WWMouseClass::Instance->XY1;
	const auto pButtons = &TacticalButtonsClass::Instance;
	pButtons->SetMouseButtonIndex(&mousePosition);

	if (pButtons->MouseIsOverTactical())
		return 0;

	R->Stack(STACK_OFFSET(0x30, -0x24), 0);
	R->EAX(Action::None);
	return SkipGameCode;
}

#pragma endregion
*/
#pragma region ButtonsDisplayHooks
/*
DEFINE_HOOK(0x6D462C, TacticalClass_Render_DrawBelowTechno, 0x5)
{
	return 0;
}

DEFINE_HOOK(0x6D4941, TacticalClass_Render_DrawButtonCameo, 0x6)
{
	const auto pButtons = &TacticalButtonsClass::Instance;

	// TODO New buttons (The later draw, the higher layer)

	return 0;
}
*/
DEFINE_HOOK(0x4F4583, GScreenClass_DrawCurrentSelectInfo, 0x6)
{
	TacticalButtonsClass::CurrentSelectInfoDraw();
	return 0;
}

#pragma endregion

//	Game::SpecialDialog = 0; // 游戏画面
//	Game::SpecialDialog = 1; // 暂停页面
//	Game::SpecialDialog = 2; // 投降页面
//	Game::SpecialDialog = 3; // 退出页面
//	Game::SpecialDialog = 4; // 快捷键设置页面
//	Game::SpecialDialog = 5; // 游戏控制页面
//	Game::SpecialDialog = 6; // 音效控制页面
//	Game::SpecialDialog = 7; // 传送讯息页面
//	Game::SpecialDialog = 8; // 盟友页面
//	Game::SpecialDialog = 9; // 任务简介页面

/*
union VoxelIndexKey
{
	struct MainKey
	{
		uint32_t mainFrame  : 5; // 移动帧号（0-4）
		uint32_t mainFace   : 5; // 车体朝向（5-9）
		uint32_t slopeIndex : 6; // 斜坡索引（10-15）
		uint32_t isSpawnAlt : 1; // SpawnAlt（16）
		uint32_t reserved   : 15;// 空保留位（17-31）
	}
	Main;

	struct TurretKey
	{
		uint32_t turretFace : 5; // 炮塔朝向（0-4）
		uint32_t mainFace   : 5; // 车体朝向（5-9），如果 TurretOffset=0 ，则此段归零，因为从中心点开始画不需要有偏移
		uint32_t slopeIndex : 6; // 斜坡索引（10-15）
		uint32_t turretFrame: 8; // 炮塔帧号（16-23）
		uint32_t turretNum  : 8; // 炮塔编号（24-31）
	}
	Turret;

	struct ShadowKey
	{
		uint32_t offsetY    : 5; // 垂直偏移（0-4）
		uint32_t mainFace   : 5; // 车体朝向（5-9）
		uint32_t slopeIndex : 5; // 斜坡索引（10-14）
		uint32_t reserved   : 16;// 空保留位（15-30）
		uint32_t onGround   : 1; // 位于地面（31）
	}
	Shadow;

	uint32_t Value;
};
*/
