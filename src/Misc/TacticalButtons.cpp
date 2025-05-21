#include "TacticalButtons.h"

#include <SuperClass.h>
#include <AircraftClass.h>
#include <MessageListClass.h>
#include <TacticalClass.h>
#include <MouseClass.h>
#include <WWMouseClass.h>
#include <AITriggerTypeClass.h>
#include <JumpjetLocomotionClass.h>

#include <Ext/House/Body.h>
#include <Ext/WarheadType/Body.h>
#include <Ext/Scenario/Body.h>
#include <Utilities/TemplateDef.h>

TacticalButtonsClass TacticalButtonsClass::Instance;

// Functions

#pragma region PrivateFunctions

int TacticalButtonsClass::CheckMouseOverButtons(const Point2D* pMousePosition)
{
	if (Phobos::Config::MessageDisplayInCenter)
		this->OnMessages = this->MouseIsOverMessageLists(pMousePosition);

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

#pragma region MessageLists

bool TacticalButtonsClass::MouseIsOverMessageLists(const Point2D* pMousePosition)
{
	const auto pMessages = &ScenarioExt::Global()->NewMessageList;

	if (TextLabelClass* pText = pMessages->MessageList)
	{
		if (pMousePosition->Y >= pMessages->MessagePos.Y && pMousePosition->X >= pMessages->MessagePos.X && pMousePosition->X <= pMessages->MessagePos.X + pMessages->Width)
		{
			const int textHeight = pMessages->Height;
			int height = pMessages->MessagePos.Y;

			for (; pText; pText = static_cast<TextLabelClass*>(pText->GetNext()))
				height += textHeight;

			if (pMousePosition->Y < (height + 2))
				return true;
		}
	}

	return false;
}

#pragma endregion

#pragma region ShowCurrentInfo

void TacticalButtonsClass::CurrentSelectPathDraw()
{
	if (!Phobos::ShowCurrentInfo)
		return;

	if (ObjectClass::CurrentObjects.Count > 0)
	{
		for (const auto& pCurrent : ObjectClass::CurrentObjects)
		{
			if (const auto pTechno = abstract_cast<TechnoClass*>(pCurrent))
			{
				std::vector<CellClass*> pathCells;

				if (const auto pFoot = abstract_cast<FootClass*, true>(pTechno))
				{
					JumpjetLocomotionClass* pJjLoco = nullptr;
					FlyLocomotionClass* pFlyLoco = nullptr;

					if ((pJjLoco = locomotion_cast<JumpjetLocomotionClass*>(pFoot->Locomotor), (pJjLoco && pJjLoco->CurrentSpeed > 0.0))
						|| (pFlyLoco = locomotion_cast<FlyLocomotionClass*>(pFoot->Locomotor), (pFlyLoco && pFlyLoco->CurrentSpeed > 0.0)))
					{
						auto curCoord = Point2D { pFoot->Location.X, pFoot->Location.Y };
						auto pCurCell = MapClass::Instance.GetCellAt(CellStruct { static_cast<short>(curCoord.X >> 8), static_cast<short>(curCoord.Y >> 8) });
						const auto face = pJjLoco ? &pJjLoco->LocomotionFacing : &pFoot->PrimaryFacing;
						const auto checkLength = (face->IsRotating() || !pFoot->Destination) ? Unsorted::LeptonsPerCell
							: Math::min((Unsorted::LeptonsPerCell * 12), pFoot->DistanceFrom(pFoot->Destination));
						const auto angle = -face->Current().GetRadian<65536>();
						const auto checkCoord = Point2D { static_cast<int>(checkLength * cos(angle) + 0.5), static_cast<int>(checkLength * sin(angle) + 0.5) };
						const auto largeStep = Math::max(abs(checkCoord.X), abs(checkCoord.Y));
						const auto checkSteps = (largeStep > Unsorted::LeptonsPerCell) ? (largeStep / Unsorted::LeptonsPerCell + 1) : 1;
						const auto stepCoord = Point2D { (checkCoord.X / checkSteps), (checkCoord.Y / checkSteps) };

						for (int i = 0; i < checkSteps; ++i)
						{
							const auto lastCoord = curCoord;
							curCoord += stepCoord;
							pCurCell = MapClass::Instance.TryGetCellAt(CellStruct { static_cast<short>(curCoord.X >> 8), static_cast<short>(curCoord.Y >> 8) });

							if (!pCurCell)
								break;

							if (std::find(pathCells.begin(), pathCells.end(), pCurCell) == pathCells.end())
								pathCells.push_back(pCurCell);

							if ((curCoord.X >> 8) != (lastCoord.X >> 8) && (curCoord.Y >> 8) != (lastCoord.Y >> 8))
							{
								bool lastX = (abs(stepCoord.X) > abs(stepCoord.Y))
									? (((curCoord.Y - ((stepCoord.X > 0)
										? (curCoord.X & 0XFF)
										: ((curCoord.X & 0XFF) - Unsorted::LeptonsPerCell))
									* checkCoord.Y / checkCoord.X) >> 8) == (curCoord.Y >> 8))
									: (((curCoord.X - ((stepCoord.Y > 0)
										? (curCoord.Y & 0XFF)
										: ((curCoord.Y & 0XFF) - Unsorted::LeptonsPerCell))
									* checkCoord.X / checkCoord.Y) >> 8) != (curCoord.X >> 8));

								if (const auto pCheckCell = MapClass::Instance.TryGetCellAt(lastX
									? CellStruct { static_cast<short>(lastCoord.X >> 8), static_cast<short>(curCoord.Y >> 8) }
									: CellStruct { static_cast<short>(curCoord.X >> 8), static_cast<short>(lastCoord.Y >> 8) }))
								{
									if (std::find(pathCells.begin(), pathCells.end(), pCheckCell) == pathCells.end())
										pathCells.push_back(pCheckCell);
								}
							}
						}
					}
					else if (pFoot->CurrentMapCoords != CellStruct::Empty)
					{
						auto pCell = MapClass::Instance.GetCellAt(pFoot->CurrentMapCoords);

						const auto& pD = pFoot->PathDirections;

						for (int i = 0; i < 24; ++i)
						{
							const auto face = pD[i];

							if (face <= -1 || face >= 8)
								break;

							pCell = pCell->GetNeighbourCell(static_cast<FacingType>(face));
							pathCells.push_back(pCell);
						}
					}
				}
				else if (const auto pBuilding = abstract_cast<BuildingClass*, true>(pTechno))
				{
					if (pBuilding->Type->ConstructionYard)
					{
						const auto pBase = &pBuilding->Owner->Base;

						for (const auto& baseCell : pBase->Cells_24)
							pathCells.push_back(MapClass::Instance.GetCellAt(baseCell));
					}
					else
					{
						const auto baseCell = pBuilding->GetMapCoords();

						for (auto pFoundation = pBuilding->Type->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
							pathCells.push_back(MapClass::Instance.GetCellAt(baseCell + *pFoundation));
					}
				}

				if (const auto cellsSize = pathCells.size())
				{
					std::sort(&pathCells[0], &pathCells[cellsSize],[](CellClass* pCellA, CellClass* pCellB)
					{
						if (pCellA->MapCoords.X != pCellB->MapCoords.X)
							return pCellA->MapCoords.X < pCellB->MapCoords.X;

						return pCellA->MapCoords.Y < pCellB->MapCoords.Y;
					});

					for (const auto& pPathCell : pathCells)
					{
						const auto location = CoordStruct { (pPathCell->MapCoords.X << 8), (pPathCell->MapCoords.Y << 8), 0 };
						const auto height = pPathCell->GetLevel() * 15;
						const auto position = TacticalClass::Instance->CoordsToScreen(location) - TacticalClass::Instance->TacticalPos - Point2D { 0, (1 + height) };

						DSurface::Temp->DrawSHP(
							FileSystem::PALETTE_PAL, Make_Global<SHPStruct*>(0x8A03FC),
							(pPathCell->SlopeIndex + 2), &position, &DSurface::ViewBounds,
							(BlitterFlags::Centered | BlitterFlags::TransLucent50 | BlitterFlags::bf_400 | BlitterFlags::Zero),
							0, (-height - (pPathCell->SlopeIndex ? 12 : 2)), ZGradient::Ground, 1000, 0, 0, 0, 0, 0
						);
					}
				}

				return;
			}
		}
	}

	const auto pCell = MapClass::Instance.GetCellAt(DisplayClass::Instance.CurrentFoundation_CenterCell);
	const auto location = CoordStruct { (pCell->MapCoords.X << 8), (pCell->MapCoords.Y << 8), 0 };
	const auto height = pCell->GetLevel() * 15;
	const auto position = TacticalClass::Instance->CoordsToScreen(location) - TacticalClass::Instance->TacticalPos - Point2D { 0, (1 + height) };

	DSurface::Temp->DrawSHP(
		FileSystem::PALETTE_PAL, Make_Global<SHPStruct*>(0x8A03FC),
		(pCell->SlopeIndex + 2), &position, &DSurface::ViewBounds,
		(BlitterFlags::Centered | BlitterFlags::TransLucent50 | BlitterFlags::bf_400 | BlitterFlags::Zero),
		0, (-height - (pCell->SlopeIndex ? 12 : 2)), ZGradient::Ground, 1000, 0, 0, 0, 0, 0
	);
}

void TacticalButtonsClass::CurrentSelectInfoDraw()
{
	if (!Phobos::ShowCurrentInfo)
		return;

	TechnoClass* pTechno = nullptr;

	if (ObjectClass::CurrentObjects.Count > 0)
	{
		for (const auto& pCurrent : ObjectClass::CurrentObjects)
		{
			if (const auto pCurrentTechno = abstract_cast<TechnoClass*>(pCurrent))
			{
				pTechno = pCurrentTechno;
				break;
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
			DSurface::Composite->DrawLine(&point1, &point2, color);
		};

		const auto thisPoint = TacticalClass::Instance->CoordsToClient(pTechno->GetCoords()).first;
		const auto offset = (0x7FFFFFFF - Unsorted::CurrentFrame) % 15;
		bool pattern[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
		auto drawDashLine = [&thisPoint, &pattern, &offset](AbstractClass* pDest, int color)
		{
			if (pDest)
			{
				auto footPoint = thisPoint;
				auto destPoint = TacticalClass::Instance->CoordsToClient(pDest->GetCoords()).first;

				if (reinterpret_cast<bool(__fastcall*)(Point2D*, Point2D*, RectangleStruct*)>(0x7BC2B0)(&footPoint, &destPoint, &DSurface::ViewBounds))
					DSurface::Composite->DrawDashedLine_(&footPoint, &destPoint, color, pattern, offset, false);

				--footPoint.Y;
				--destPoint.Y;

				if (reinterpret_cast<bool(__fastcall*)(Point2D*, Point2D*, RectangleStruct*)>(0x7BC2B0)(&footPoint, &destPoint, &DSurface::ViewBounds))
					DSurface::Composite->DrawDashedLine_(&footPoint, &destPoint, color, pattern, offset, false);

				--footPoint.Y;
				--destPoint.Y;

				if (reinterpret_cast<bool(__fastcall*)(Point2D*, Point2D*, RectangleStruct*)>(0x7BC2B0)(&footPoint, &destPoint, &DSurface::ViewBounds))
					DSurface::Composite->DrawDashedLine_(&footPoint, &destPoint, color, pattern, offset, false);
			}
		};

		if (const auto pFoot = abstract_cast<FootClass*, true>(pTechno))
		{
			const auto& pD = pFoot->PathDirections;
			auto face = pD[0];

			if (face > -1 && face < 8)
			{
				auto pCell = MapClass::Instance.GetCellAt(pFoot->CurrentMapCoords)->GetNeighbourCell(static_cast<FacingType>(face));

				for (int i = 1; i < 24; ++i)
				{
					const auto thisFace = pD[i];

					if (thisFace <= -1 || thisFace >= 8)
						break;

					face = thisFace;
					pCell = pCell->GetNeighbourCell(static_cast<FacingType>(face));
				}

				TechnoExt::DrawExtraImage(pFoot, pCell, DirStruct(face << 13));
			}

			drawDashLine(pFoot->Target, COLOR_RED);
			drawDashLine(pFoot->ArchiveTarget, COLOR_PURPLE);
			drawDashLine(pFoot->GetNthLink(), COLOR_WHITE);
			drawDashLine(pFoot->QueueUpToEnter, COLOR_BLUE);
			drawDashLine(pFoot->unknown_5A0, COLOR_YELLOW);
			drawDashLine(pFoot->Destination, COLOR_GREEN);
			drawDashLine(pFoot->MegaTarget, COLOR_CYAN);
			drawDashLine(pFoot->MegaDestination, COLOR_CYAN);

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

			if (pTechno->HasTurret())
			{
				auto mtxTur = mtxBase;
				TechnoTypeExt::ApplyTurretOffset(pTechno->GetTechnoType(), &mtxTur, 1.0);

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
		else
		{
			drawDashLine(pTechno->Target, COLOR_RED);
			drawDashLine(pTechno->ArchiveTarget, COLOR_PURPLE);
			drawDashLine(pTechno->GetNthLink(), COLOR_WHITE);
			drawDashLine(pTechno->QueueUpToEnter, COLOR_BLUE);

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

	auto drawText = [&loc, &updateLine, &drawRect, &textLocation](const char* pFormat, ...)
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
		DSurface::Composite->DrawTextA(wBuffer, &drawRect, &textLocation, COLOR_WHITE, 0, printType);
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
				ID = "Cell";
			}

			drawText("%s: %s(%03d,%03d)[%dC]", pInfoName, ID, mapCoords.X, mapCoords.Y, (pCurrent->DistanceFrom(pTarget) / Unsorted::LeptonsPerCell));
		}
		else
		{
			drawText("%s: %s", pInfoName, "N/A");
		}
	};

	auto drawTask = [&drawText](const char* pInfoName, Mission mission)
	{
		drawText("%s: (%02d)[%s]", pInfoName, mission, MissionControlClass::FindName(mission));
	};

	auto drawTime = [&drawText](const char* pInfoName, CDTimerClass& timer)
	{
		const auto timeCeiling = timer.TimeLeft;
		const auto timeCurrent = timeCeiling - timer.GetTimeLeft();
		const auto timePercentage = (timeCeiling > 0) ? (timeCurrent * 100 / timeCeiling) : 0;

		drawText("%s: (%d/%d)[%03d]", pInfoName, timeCurrent, timeCeiling, timePercentage);
	};

	drawText("Current Frame: %d", Unsorted::CurrentFrame);
	{
		const auto mouseXY1 = WWMouseClass::Instance->XY1;
		auto point = mouseXY1 - Point2D { DSurface::ViewBounds.X, DSurface::ViewBounds.Y };
		auto cell = CellStruct::Empty;
		auto coords = CoordStruct::Empty;
		ObjectClass* pObj = nullptr;
		BYTE fogged = 0;
		BYTE shrouded = 0;

		DisplayClass::Instance.ProcessClickCoords(&point, &cell, &coords, &pObj, &fogged, &shrouded);
		const auto pCell = MapClass::Instance.GetCellAt(cell);

		drawText("Address: 0x%X", reinterpret_cast<DWORD>(pCell));
		drawText("Cell: %d", MapClass::Instance.GetCellIndex(pCell->MapCoords));
		drawText("UniqueID: %d", pCell->UniqueID);

		{
			constexpr const char* landTypes[12] = { "Clear", "Road", "Water", "Rock", "Wall", "Tiberium", "Beach", "Rough", "Ice", "Railroad", "Tunnel", "Weeds" };
			const auto landType = static_cast<int>(pCell->LandType);

			drawText("LandType: ( %s )", (landType >= 0 && landType < 12) ? landTypes[landType] : "Unknown");
			drawText("Slope: ( %d )", pCell->SlopeIndex);
		}

		drawText("Location: (%05d,%05d,%05d)[%03d,%03d,%02d]", coords.X, coords.Y, coords.Z, cell.X, cell.Y, pCell->GetLevel());
		updateLine();

		{
			const auto nCF = static_cast<DWORD>(pCell->Flags);

			drawText("CellFlags: %d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d",
				((nCF >> 22) & 0x1), ((nCF >> 21) & 0x1), ((nCF >> 20) & 0x1), ((nCF >> 19) & 0x1), ((nCF >> 18) & 0x1), ((nCF >> 17) & 0x1), ((nCF >> 16) & 0x1),
				((nCF >> 15) & 0x1), ((nCF >> 14) & 0x1), ((nCF >> 13) & 0x1), ((nCF >> 12) & 0x1), ((nCF >> 11) & 0x1), ((nCF >> 10) & 0x1), ((nCF >> 9) & 0x1), ((nCF >> 8) & 0x1),
				((nCF >> 7) & 0x1), ((nCF >> 6) & 0x1), ((nCF >> 5) & 0x1), ((nCF >> 4) & 0x1), ((nCF >> 3) & 0x1), ((nCF >> 2) & 0x1), ((nCF >> 1) & 0x1), (nCF & 0x1));

			updateLine();
		}

		{
			const auto nOF = pCell->OccupationFlags;
			const auto nAF = pCell->AltOccupationFlags;

			drawText("TheOccupationFlags: %d%d%d%d%d%d%d%d", ((nOF >> 7) & 0x1), ((nOF >> 6) & 0x1), ((nOF >> 5) & 0x1), ((nOF >> 4) & 0x1), ((nOF >> 3) & 0x1), ((nOF >> 2) & 0x1), ((nOF >> 1) & 0x1), (nOF & 0x1));
			drawText("AltOccupationFlags: %d%d%d%d%d%d%d%d", ((nAF >> 7) & 0x1), ((nAF >> 6) & 0x1), ((nAF >> 5) & 0x1), ((nAF >> 4) & 0x1), ((nAF >> 3) & 0x1), ((nAF >> 2) & 0x1), ((nAF >> 1) & 0x1), (nAF & 0x1));
		}

		drawText("TubeIndex: %d", pCell->TubeIndex);
		drawText("RadLevel: %.2f", pCell->RadLevel);

		{
			const auto pOwner = HouseClass::Array.GetItemOrDefault(pCell->WallOwnerIndex);

			drawText("WallOwner: %s(%s)", (pOwner ? pOwner->get_ID() : "N/A"), (pOwner ? pOwner->PlainName : "N/A"));
			drawInfo("CurrentJumpjet", pCell, pCell->Jumpjet);
		}

		{
			int count = 0;
			int index = 0;

			for (auto pCellObj = pCell->FirstObject; pCellObj; pCellObj = pCellObj->NextObject)
				++count;

			drawText("TheObjects: (Ground)[%d]", count);
			updateLine();

			for (auto pCellObj = pCell->FirstObject; pCellObj; pCellObj = pCellObj->NextObject, ++index)
				drawText("TheObject(%d)[%s]", index, pCellObj->GetType()->get_ID());

			if (loc)
				updateLine();
		}

		{
			int count = 0;
			int index = 0;

			for (auto pCellObj = pCell->AltObject; pCellObj; pCellObj = pCellObj->NextObject)
				++count;

			drawText("AltObjects: (Bridge)[%d]", count);
			updateLine();

			for (auto pCellObj = pCell->AltObject; pCellObj; pCellObj = pCellObj->NextObject, ++index)
				drawText("AltObject(%d)[%s]", index, pCellObj->GetType()->get_ID());

			if (loc)
				updateLine();
		}

		updateLine();
		updateLine();

		const auto pMouse = &MouseClass::Instance;

		drawText("Mouse: (%04d,%04d)", mouseXY1.X, mouseXY1.Y);
		drawText("RadarScope: (%03d,%03d,%02d,%02d)", pMouse->unknown_rect_14DC.X, pMouse->unknown_rect_14DC.Y, pMouse->unknown_rect_14DC.Width, pMouse->unknown_rect_14DC.Height);
	}

	if (pTechno)
	{
		drawText("Current Select Techno:");
		drawText("Address: 0x%X", reinterpret_cast<DWORD>(pTechno));

		const auto pType = pTechno->GetTechnoType();
		const auto absType = pTechno->WhatAmI();

		if (absType == AbstractType::Unit)
			drawText("%s: %s", "Vehicle", pType->ID);
		else if (absType == AbstractType::Infantry)
			drawText("%s: %s", "Infantry", pType->ID);
		else if (absType == AbstractType::Aircraft)
			drawText("%s: %s", "Aircraft", pType->ID);
		else if (absType == AbstractType::Building)
			drawText("%s: %s", "Building", pType->ID);
		else
			drawText("%s: %s", "Unknown", pType->ID);

		drawText("UniqueID: %d", pTechno->UniqueID);

		const auto pOwner = pTechno->Owner;
		{
			const auto pOrigin = pTechno->GetOriginalOwner();

			drawText("Owner: %s(Player<%d>)", (pOwner ? pOwner->get_ID() : "N/A"), (pOwner ? pOwner->ArrayIndex : -1));
			drawText("Origin: %s(Player<%d>)", (pOrigin ? pOrigin->get_ID() : "N/A"), (pOrigin ? pOrigin->ArrayIndex : -1));
		}

		{
			const auto cell = pTechno->GetMapCoords();
			const auto coords = pTechno->GetCoords();

			drawText("Location: (%05d,%05d,%05d)[%03d,%03d,%02d]", coords.X, coords.Y, coords.Z, cell.X, cell.Y, pTechno->GetCellLevel());
			updateLine();
		}

		const auto facing1 = pTechno->PrimaryFacing.Current();
		const auto primaryFacing = facing1.GetValue<3>();

		{
			constexpr const char* facingTypes[8] = { "North", "NorthEast", "East", "SouthEast", "South", "SouthWest", "West", "NorthWest" };
			const auto facing11 = pTechno->PrimaryFacing.StartFacing;
			const auto facing12 = pTechno->PrimaryFacing.DesiredFacing;

			drawText("PrimaryFacing: (%05d[%02d])[%s]", facing1.Raw, facing1.GetValue<5>(), facingTypes[primaryFacing]);
			updateLine();

			drawText("PriStartFacing: (%05d)", facing11.Raw);
			drawText("PriDesiredFacing: (%05d)", facing12.Raw);

			const auto facing2 = pTechno->SecondaryFacing.Current();
			const auto facing21 = pTechno->SecondaryFacing.StartFacing;
			const auto facing22 = pTechno->SecondaryFacing.DesiredFacing;

			drawText("SecondaryFacing: (%05d[%02d])[%s]", facing2.Raw, facing2.GetValue<5>(), facingTypes[facing2.GetValue<3>()]);
			updateLine();

			drawText("SecStartFacing: (%05d)", facing21.Raw);
			drawText("SecDesiredFacing: (%05d)", facing22.Raw);
		}

		const auto pExt = TechnoExt::ExtMap.Find(pTechno);

		drawText("Ammo: (%d/%d)", pTechno->Ammo, pType->Ammo);
		drawText("Tether: (%s,%s)", (pTechno->IsTether ? "Yes" : "No"), (pTechno->IsAlternativeTether ? "Yes" : "No"));

		drawText("Health: (%d/%d)", pTechno->Health, pType->Strength);
		drawText("Shield: (%d/%d)", (pExt->Shield ? pExt->Shield->GetHP() : -1), (pExt->CurrentShieldType ? pExt->CurrentShieldType->Strength : -1));

		drawTime("Reload", pTechno->ReloadTimer);
		drawTime("Rearm", pTechno->RearmTimer);

		drawTime("Update", pTechno->UpdateTimer);
		drawInfo("Bunker Linked", pTechno, pTechno->BunkerLinkedItem);

		drawInfo("Target", pTechno, pTechno->Target);
		drawText("TargetInRange: %s", (pTechno->IsCloseEnough(pTechno->Target, pTechno->SelectWeapon(pTechno->Target)) ? "Yes" : "No"));

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
			drawText("Passengers: (%d/%d)[%d]", (bySize ? pTechno->Passengers.GetTotalSize() : count), capacity, count);

			if (capacity > 1)
				updateLine();

			for (int i = 0; i < capacity; ++i)
			{
				if (currentSize > i)
				{
					drawText("Passenger(%d)[%s]", i, "---");
					continue;
				}

				if (pPassenger)
				{
					if (const auto pPassengerType = pPassenger->GetTechnoType())
					{
						drawText("Passenger(%d)[%s]", i, pPassengerType->ID);
						currentSize += bySize ? static_cast<int>(pPassengerType->Size) : 1;
						pPassenger = pPassenger->NextObject;
						continue;
					}

					pPassenger = pPassenger->NextObject;
				}

				drawText("Passenger(%d)[%s]", i, "N/A");
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

			drawText("RadioLinks: (%d/%d)", count, capacity);

			if (capacity > 1)
				updateLine();

			for (int i = 0; i < capacity; ++i)
			{
				if (const auto pLink = pTechno->RadioLinks.Items[i])
					drawText("RadioLink(%d)[%s]", i, pLink->GetType()->ID);
				else
					drawText("RadioLink(%d)[%s]", i, "N/A");
			}

			if (loc)
				updateLine();
		}

		drawText("TurretRecoil: %.3f", pTechno->TurretRecoil.TravelSoFar);
		drawText("BarrelRecoil: %.3f", pTechno->BarrelRecoil.TravelSoFar);

		drawTask("Mission", pTechno->CurrentMission);
		drawText("Status: %d , Start: %d", pTechno->MissionStatus, pTechno->CurrentMissionStartTime);

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
			drawInfo("Patrol Target", pFoot, reinterpret_cast<AbstractClass*>(pFoot->unknown_5DC));

			drawInfo("First ArrayItem", pFoot, (pFoot->unknown_abstract_array_588.Count > 0 ? pFoot->unknown_abstract_array_588.GetItem(0) : nullptr));
			drawInfo("First NavQueue", pFoot, (pFoot->NavQueue.Count > 0 ? pFoot->NavQueue.GetItem(0) : nullptr));

			drawText("FootCell: (%03d,%03d)", pFoot->CurrentMapCoords.X, pFoot->CurrentMapCoords.Y);
			drawText("LastCell: (%03d,%03d)", pFoot->LastMapCoords.X, pFoot->LastMapCoords.Y);

			{
				const auto destination = pFoot->Locomotor->Destination();
				const auto headToCoord = pFoot->Locomotor->Head_To_Coord();

				drawText("LocoDest: (%05d,%05d,%05d)", destination.X, destination.Y, destination.Z);
				drawText("LocoHead: (%05d,%05d,%05d)", headToCoord.X, headToCoord.Y, headToCoord.Z);
			}

			{
				constexpr const char* moveTypes[8] = { "Clear", "Cloak", "Move", "Gate", "A-Block", "E-Block", "Temp", "Unable" };
				const auto facingType = static_cast<FacingType>(primaryFacing);
				const auto moveType = static_cast<int>(pFoot->IsCellOccupied(MapClass::Instance.GetCellAt(pFoot->CurrentMapCoords)->GetNeighbourCell(facingType), facingType, pFoot->GetCellLevel(), nullptr, true));

				drawText("PlanningPathIdx: %d", pFoot->PlanningPathIdx);
				drawText("FaceMoveType: (%s)", (moveType >= 0 && moveType < 8) ? moveTypes[moveType] : "N/A");
			}

			const auto& pD = pFoot->PathDirections;

			if (pD[0] == -1)
				drawText("PathDir: N/A");
			else if (pD[1] == -1)
				drawText("PathDir: %d", pD[0]);
			else if (pD[2] == -1)
				drawText("PathDir: %d-%d", pD[0], pD[1]);
			else if (pD[3] == -1)
				drawText("PathDir: %d-%d-%d", pD[0], pD[1], pD[2]);
			else if (pD[4] == -1)
				drawText("PathDir: %d-%d-%d-%d", pD[0], pD[1], pD[2], pD[3]);
			else if (pD[5] == -1)
				drawText("PathDir: %d-%d-%d-%d-%d", pD[0], pD[1], pD[2], pD[3], pD[4]);
			else if (pD[6] == -1)
				drawText("PathDir: %d-%d-%d-%d-%d-%d", pD[0], pD[1], pD[2], pD[3], pD[4], pD[5]);
			else if (pD[7] == -1)
				drawText("PathDir: %d-%d-%d-%d-%d-%d-%d", pD[0], pD[1], pD[2], pD[3], pD[4], pD[5], pD[6]);
			else
				drawText("PathDir: %d-%d-%d-%d-%d-%d-%d-%d", pD[0], pD[1], pD[2], pD[3], pD[4], pD[5], pD[6], pD[7]);

			updateLine();

			drawText("CurrentSpeed: %d", static_cast<int>(pFoot->GetCurrentSpeed()));
			drawText("PercentSpeed: %d", static_cast<int>(pFoot->SpeedPercentage * 100));

			drawText("OnElevatedBridge: %s", (pFoot->OnBridge ? "Yes" : "No"));
			drawText("NearElevatedBridge: %s", (reinterpret_cast<bool(__thiscall*)(FootClass*)>(0x703B10)(pFoot) ? "Yes" : "No"));

			drawText("OnBacklit: %s", (pFoot->vt_entry_2B0() ? "Yes" : "No"));
			drawText("IsCrushing: %s", (pFoot->IsCrushingSomething ? "Yes" : "No"));

			drawText("Scattering: %s", (pExt->ScatteringStopFrame >= Unsorted::CurrentFrame ? "Yes" : "No"));
			drawText("Aggressive: %s", (pExt->AggressiveStance ? "Yes" : "No"));

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

				drawText("Trigger: %s", (pTriggerType ? pTriggerType->ID : "N/A"));
				drawText("Team: %s", pTeamType->ID);

				drawText("Task: %s", pTeamType->TaskForce->ID);
				drawText("Script: %s", pScriptType->get_ID());

				drawText("Weights [Cur,Min,Max] -");

				if (pTriggerType)
					drawText("[%.2f,%.2f,%.2f]", pTriggerType->Weight_Current, pTriggerType->Weight_Minimum, pTriggerType->Weight_Maximum);
				else
					drawText("[%.2f,%.2f,%.2f]", -1.0, -1.0, -1.0);

				drawText("Script [Line=Act,Arg] -");
				drawText("[%d=%d,%d]", mission, (mission >= 0 ? pScriptType->ScriptActions[mission].Action : -1), (mission >= 0 ? pScriptType->ScriptActions[mission].Argument : -1));
			}
			else
			{
				drawText("Trigger: %s", "N/A");
				drawText("Team: %s", "N/A");

				drawText("Task:", "N/A");
				drawText("Script: %s", "N/A");

				drawText("Weights [Cur,Min,Max] -");
				drawText("[%.2f,%.2f,%.2f]", -1.0, -1.0, -1.0);

				drawText("Script [Line=Act,Arg] -");
				drawText("[%d=%d,%d]", -1, -1, -1);
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

				drawText("Occupants: (%d/%d)", count, capacity);

				if (capacity > 1)
					updateLine();

				for (int i = 0; i < capacity; ++i)
				{
					if (i < count)
						drawText("Occupant(%d)[%s]", i, pBuilding->Occupants.GetItem(i)->Type->ID);
					else
						drawText("Occupant(%d)[%s]", i, "N/A");
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

				drawText("Overpowers: (%d/%d)[%d]", count, capacity, overpower);

				if (capacity > 1)
					updateLine();

				int currentOverpower = 0;
				int currentCharger = 0;

				for (int i = 0; i < capacity; ++i)
				{
					if (currentOverpower > i)
					{
						drawText("Overpower(%d)[%s]", i, "---");
						continue;
					}

					if (currentCharger < static_cast<int>(chargers.size()))
					{
						const auto chargerPair = chargers[currentCharger];
						drawText("Overpower(%d)[%s]", i, chargerPair.first->GetType()->ID);
						currentOverpower += chargerPair.second;
						++currentCharger;
						continue;
					}

					drawText("Overpower(%d)[%s]", i, "N/A");
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
					drawText("Product: (%s)[%d]", pProduct->GetTechnoType()->ID, (pFactory->GetProgress() * 100 / 54));
				else
					drawText("Product: (%s)[%d]", "N/A", 0);

				drawTime("RetryProduction", pBuilding->FactoryRetryTimer);
			}

			drawTime("CashProduction", pBuilding->CashProductionTimer);
			drawTime("BuildingGate", pBuilding->GateTimer);

			drawText("BaseNodes: %d", pOwner->Base.BaseNodes.Count);
			drawText("BaseCenter: (%03d,%03d)", pOwner->Base.Center.X, pOwner->Base.Center.Y);

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
					drawText("SuperWeapon: (0/-1)[000]");

				updateLine();
			}

			// Upgrade Status
			if (const auto upgrades = pBuildingType->Upgrades)
			{
				const auto pType1 = pBuilding->Upgrades[0];
				const auto pType2 = pBuilding->Upgrades[1];
				const auto pType3 = pBuilding->Upgrades[2];

				drawText("Upgrades: (%d/%d)", pBuilding->UpgradeLevel, upgrades);
				drawText("Slot-1: %s", (pType1 ? pType1->ID : "N/A"));
				drawText("Slot-2: %s", (pType2 ? pType2->ID : "N/A"));
				drawText("Slot-3: %s", (pType3 ? pType3->ID : "N/A"));
			}
			else
			{
				drawText("Upgrades: (%d/%d)", -1, -1);
				drawText("Slot-1: %s", "N/A");
				drawText("Slot-2: %s", "N/A");
				drawText("Slot-3: %s", "N/A");
			}
		}
	}
}

#pragma endregion

// Hooks

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
	return (pThis->unknown_byte_554A && !TacticalButtonsClass::Instance.PressedInButtonsLayer) ? CheckMousePress : CheckMouseNoPress;
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

#pragma region ButtonsDisplayHooks

DEFINE_HOOK(0x6D462C, TacticalClass_Render_DrawBelowTechno, 0x5)
{
	TacticalButtonsClass::Instance.CurrentSelectPathDraw();
	return 0;
}
/*
DEFINE_HOOK(0x6D4941, TacticalClass_Render_DrawButtonCameo, 0x6)
{
	const auto pButtons = &TacticalButtonsClass::Instance;

	// TODO New buttons (The later draw, the higher layer)

	return 0;
}
*/
DEFINE_HOOK(0x4F4583, GScreenClass_DrawCurrentSelectInfo, 0x6)
{
	TacticalButtonsClass::Instance.NewMsgList = true;
	ScenarioExt::Global()->NewMessageList.Draw();
	TacticalButtonsClass::Instance.NewMsgList = false;

	TacticalButtonsClass::Instance.CurrentSelectInfoDraw();

	return 0;
}

#pragma endregion

#pragma region MessageList

DEFINE_HOOK(0x55DDA0, MainLoop_FrameStep_NewMessageListManage, 0x5)
{
	if (!TacticalButtonsClass::Instance.OnMessages)
		ScenarioExt::Global()->NewMessageList.Manage();

	return 0;
}

DEFINE_HOOK(0x5D3BA0, MessageListClass_AddMessage_InCenter, 0x6)
{
	if (Phobos::Config::MessageDisplayInCenter && *R->ESP<int*>() == 0x6DE127) // TActionClass::Execute
		R->ECX(&ScenarioExt::Global()->NewMessageList);

	return 0;
}

DEFINE_HOOK(0x4A8B9B, DisplayClass_Set_View_Dimensions, 0x6)
{
	if (Phobos::Config::MessageDisplayInCenter)
	{
		const auto& rect = DSurface::ViewBounds;
		const auto sideWidth = rect.Width / 6;
		const auto width = rect.Width - (sideWidth << 1);
		const auto pList = &ScenarioExt::Global()->NewMessageList;
		pList->Init((rect.X + sideWidth), (rect.Height - rect.Height / 8 - 120), 6, 98, 18, -1, -1, 0, 20, 98, width);
		pList->SetWidth(width);
	}

	return 0;
}

DEFINE_HOOK(0x684A9A, UnknownClass_sub_684620_InitMessageList, 0x6)
{
	if (Phobos::Config::MessageDisplayInCenter)
	{
		const auto& rect = DSurface::ViewBounds;
		const auto sideWidth = rect.Width / 6;
		const auto width = rect.Width - (sideWidth << 1);
		const auto pList = &ScenarioExt::Global()->NewMessageList;
		pList->Init((rect.X + sideWidth), (rect.Height - rect.Height / 8 - 120), 6, 98, 18, -1, -1, 0, 20, 98, width);
	}

	return 0;
}

DEFINE_HOOK(0x623A9F, DSurface_sub_623880_DrawBitFontStrings, 0x5)
{
	if (!TacticalButtonsClass::Instance.NewMsgList)
		return 0;

	enum { SkipGameCode = 0x623AAB };

	GET(RectangleStruct* const, pRect, EAX);
	GET(DSurface* const, pSurface, ECX);
	GET(const int, height, EBP);

	pRect->Height = height;
	auto black = ColorStruct { 0, 0, 0 };
	auto trans = (TacticalButtonsClass::Instance.OnMessages || ScenarioClass::Instance->UserInputLocked) ? 80 : 40;
	pSurface->FillRectTrans(pRect, &black, trans);

	return SkipGameCode;
}

#pragma endregion
