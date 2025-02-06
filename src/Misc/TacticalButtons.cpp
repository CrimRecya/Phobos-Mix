#include "TacticalButtons.h"

#include <GameOptionsClass.h>
#include <SuperClass.h>
#include <AircraftClass.h>
#include <MessageListClass.h>
#include <TacticalClass.h>
#include <WWMouseClass.h>
#include <InputManagerClass.h>
#include <FPSCounter.h>
#include <AITriggerTypeClass.h>
#include <JumpjetLocomotionClass.h>

#include <Ext/House/Body.h>
#include <Ext/Scenario/Body.h>
#include <Utilities/TemplateDef.h>

#include <sstream>
#include <iomanip>

TacticalButtonsClass TacticalButtonsClass::Instance;

// Functions

#pragma region PrivateFunctions

int TacticalButtonsClass::CheckMouseOverButtons(const Point2D* pMousePosition)
{
	if (Phobos::Config::MessageDisplayInCenter)
	{
		if (this->MouseIsOverMessageLists(pMousePosition))
		{
			if (!Make_Global<int>(0xABCD40)) // Frame index in this second
			{
				for (auto pText = MessageListClass::Instance->MessageList; pText; pText = static_cast<TextLabelClass*>(pText->GetNext()))
					pText->UserData1 = reinterpret_cast<void*>(reinterpret_cast<int>(pText->UserData1) + 60);
			}

			this->OnMessages = true;
		}
		else
		{
			this->OnMessages = false;
		}
	}

	if (this->HeroVisible) // Button index 1-8 : Heros buttons
	{
		auto& vec = ScenarioExt::Global()->OwnedHeros;

		if (const int counts = Math::min(8, static_cast<int>(vec.size())))
		{
			const int width = DSurface::Composite->GetWidth() - 65;

			if (pMousePosition->X >= width && pMousePosition->X < (width + 60))
			{
				int checkHight = 35;

				for (int i = 0; i < counts; ++i)
				{
					if (pMousePosition->Y < checkHight)
						break;

					checkHight += 48;

					if (pMousePosition->Y < checkHight)
						return i + 1;

					checkHight += 2;
				}
			}
		}
	}

	if (Phobos::Config::SelectedDisplay_Enable) // Button index 11-100 : Select buttons
	{
		if (const int currentCounts = Math::min(30, static_cast<int>(this->CurrentSelectCameo.size())))
		{
			const int height = DSurface::Composite->GetHeight() - 80;

			if (pMousePosition->Y >= (this->IndexInSelectButtons() ? (height - 10) : height) && pMousePosition->Y < (height + 48))
			{
				const int gap = Math::min(60, ((DSurface::Composite->GetWidth() << 1) / (5 * currentCounts)));

				if (pMousePosition->X < (gap * (currentCounts - 1) + 60))
				{
					const int index = currentCounts + 11;
					int checkWidth = 0;

					for (int i = 11; i < index; ++i)
					{
						if (this->RecordIndex == i)
						{
							checkWidth += 60;

							if (this->ButtonIndex == this->RecordIndex)
							{
								if (pMousePosition->X < checkWidth)
									return i;

								break;
							}
							else if (pMousePosition->X < checkWidth && pMousePosition->Y >= height)
							{
								return i;
							}
						}
						else
						{
							checkWidth += gap;

							if (pMousePosition->X < checkWidth)
							{
								if (pMousePosition->Y >= height)
									return i;
								else
									break;
							}
						}
					}
				}
			}
		}
	}

	// TODO New buttons

	if (this->CheckMouseOverBackground(pMousePosition))
		return 0; // Button index 0 : Background

	return -1;
}

bool TacticalButtonsClass::CheckMouseOverBackground(const Point2D* pMousePosition)
{
	// TODO New button backgrounds
	if (Phobos::Config::SelectedDisplay_Enable)
	{
		const int currentCounts = this->CurrentSelectCameo.size();

		if (currentCounts == 1 && this->CurrentSelectCameo[0].Count == 1)
		{
			if (pMousePosition->X < 180 && pMousePosition->X >= 0)
			{
				const int height = DSurface::Composite->GetHeight() - 80;

				if (pMousePosition->Y >= height && pMousePosition->Y < (height + 48))
					return true;
			}
		}
	}

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

	// Hero ToolTip And Button Record
	if (this->IndexInHerosButtons())
	{
		const wchar_t* name = ScenarioExt::Global()->OwnedHeros[this->ButtonIndex - 1]->TypeExtData->OwnerObject()->UIName;

		if (!name)
			this->HoveredHero = nullptr;
		else if (name != this->HoveredHero)
			this->HoveredHero = name;
	}
	else if (this->HoveredHero)
	{
		this->HoveredHero = nullptr;
	}

	// Select ToolTip And Button Record
	if (this->IndexInSelectButtons())
	{
		this->RecordIndex = this->ButtonIndex;
		const int currentCounts = this->CurrentSelectCameo.size();

		if (currentCounts > 1 || (currentCounts && this->CurrentSelectCameo[0].Count > 1))
		{
			const wchar_t* name = this->CurrentSelectCameo[this->ButtonIndex - 11].TypeExt->OwnerObject()->UIName;

			if (!name)
				this->HoveredSelected = nullptr;
			else if (name != this->HoveredSelected)
				this->HoveredSelected = name;
		}
		else if (this->HoveredSelected)
		{
			this->HoveredSelected = nullptr;
		}
	}
	else if (this->HoveredSelected)
	{
		this->HoveredSelected = nullptr;
	}

	// TODO New buttons
}

void TacticalButtonsClass::PressDesignatedButton(int triggerIndex)
{
	if (!this->MouseIsOverButtons()) // In buttons background
		return;

	if (this->IndexInHerosButtons())
	{
		if (!triggerIndex)
			this->HeroSelect(this->ButtonIndex);
	}
	else if (this->IndexInSelectButtons())
	{
		if (!triggerIndex)
			this->SelectedTrigger(this->ButtonIndex, true);
		else if (triggerIndex == 2)
			this->SelectedTrigger(this->ButtonIndex, false);
	}
	// TODO New buttons
}

#pragma endregion

#pragma region MessageLists

bool TacticalButtonsClass::MouseIsOverMessageLists(const Point2D* pMousePosition)
{
	const auto pMessages = &MessageListClass::Instance();

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

#pragma region FPSCounter

void TacticalButtonsClass::FPSCounterDraw()
{
	if (!Phobos::Config::FPSCounter_Enable)
		return;

	const auto fps = FPSCounter::CurrentFrameRate();
	wchar_t fpsBuffer[0x20];
	swprintf_s(fpsBuffer, L"FPS: %u", fps);
	RectangleStruct fpsDim = Drawing::GetTextDimensions(fpsBuffer, Point2D::Empty, 0);
	wchar_t avgBuffer[0x20];
	swprintf_s(avgBuffer, L"Avg: %.2lf", FPSCounter::GetAverageFrameRate());
	RectangleStruct avgDim = Drawing::GetTextDimensions(avgBuffer, Point2D::Empty, 0);

	const auto gameSpeed = GameOptionsClass::Instance->GameSpeed;
	COLORREF color = 0x67EC;

	if (!gameSpeed || fps < static_cast<unsigned int>(60 / gameSpeed))
	{
		if (fps < 10)
			color = 0xF986;
		else if (fps < 20)
			color = 0xFC05;
		else if (fps < 30)
			color = 0xFCE5;
		else if (fps < 45)
			color = 0xFFEC;
		else if (fps < 60)
			color = 0x9FEC;
	}

	const int trans = InputManagerClass::Instance->IsForceMoveKeyPressed() ? 80 : 40;
	const auto height = DSurface::Composite->GetHeight() - ((Phobos::Config::SelectedDisplay_Enable && this->CurrentSelectCameo.size()) ? 80 : 32);
	ColorStruct fillColor { 0, 0, 0 };
	RectangleStruct rect { 0, (height - avgDim.Height), avgDim.Width + 4, avgDim.Height };
	DSurface::Composite->FillRectTrans(&rect, &fillColor, trans);
	auto location = Point2D { 2, rect.Y };
	DSurface::Composite->DrawText(avgBuffer, &location, color);

	rect.Y -= fpsDim.Height;
	rect.Width = fpsDim.Width + 4;
	rect.Height = fpsDim.Height;
	DSurface::Composite->FillRectTrans(&rect, &fillColor, trans);
	location.Y = rect.Y;
	DSurface::Composite->DrawText(fpsBuffer, &location, color);
}

#pragma endregion

#pragma region ShowCurrentInfo

void TacticalButtonsClass::CurrentSelectPathDraw()
{
	if (!Phobos::ShowCurrentInfo)
		return;

	if (ObjectClass::CurrentObjects->Count > 0)
	{
		for (const auto& pCurrent : ObjectClass::CurrentObjects())
		{
			if (const auto pTechno = abstract_cast<TechnoClass*>(pCurrent))
			{
				std::vector<CellClass*> pathCells;

				if (const auto pFoot = abstract_cast<FootClass*>(pTechno))
				{
					JumpjetLocomotionClass* pJjLoco = nullptr;
					FlyLocomotionClass* pFlyLoco = nullptr;

					if ((pJjLoco = locomotion_cast<JumpjetLocomotionClass*>(pFoot->Locomotor), (pJjLoco && pJjLoco->CurrentSpeed > 0.0))
						|| (pFlyLoco = locomotion_cast<FlyLocomotionClass*>(pFoot->Locomotor), (pFlyLoco && pFlyLoco->CurrentSpeed > 0.0)))
					{
						auto curCoord = Point2D { pFoot->Location.X, pFoot->Location.Y };
						auto pCurCell = MapClass::Instance->GetCellAt(CellStruct { static_cast<short>(curCoord.X >> 8), static_cast<short>(curCoord.Y >> 8) });
						const auto face = pJjLoco ? &pJjLoco->LocomotionFacing : &pFoot->PrimaryFacing;
						const auto checkLength = face->IsRotating() ? 256 : Math::min((Unsorted::LeptonsPerCell * 12), pFoot->DistanceFrom(pFoot->Destination));
						const auto angle = -face->Current().GetRadian<65536>();
						const auto checkCoord = Point2D { static_cast<int>(checkLength * cos(angle) + 0.5), static_cast<int>(checkLength * sin(angle) + 0.5) };
						const auto largeStep = Math::max(abs(checkCoord.X), abs(checkCoord.Y));
						const auto checkSteps = (largeStep > Unsorted::LeptonsPerCell) ? (largeStep / Unsorted::LeptonsPerCell + 1) : 1;
						const auto stepCoord = Point2D { (checkCoord.X / checkSteps), (checkCoord.Y / checkSteps) };

						for (int i = 0; i < checkSteps; ++i)
						{
							const auto lastCoord = curCoord;
							curCoord += stepCoord;
							pCurCell = MapClass::Instance->TryGetCellAt(CellStruct { static_cast<short>(curCoord.X >> 8), static_cast<short>(curCoord.Y >> 8) });

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

								if (const auto pCheckCell = MapClass::Instance->TryGetCellAt(lastX
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
						auto pCell = MapClass::Instance->GetCellAt(pFoot->CurrentMapCoords);

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
				else if (const auto pBuilding = abstract_cast<BuildingClass*>(pTechno))
				{
					if (pBuilding->Type->ConstructionYard)
					{
						const auto pBase = &pBuilding->Owner->Base;

						for (const auto& baseCell : pBase->Cells_24)
							pathCells.push_back(MapClass::Instance->GetCellAt(baseCell));
					}
					else
					{
						const auto baseCell = pBuilding->GetMapCoords();

						for (auto pFoundation = pBuilding->Type->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
							pathCells.push_back(MapClass::Instance->GetCellAt(baseCell + *pFoundation));
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

	const auto pCell = MapClass::Instance->GetCellAt(DisplayClass::Instance->CurrentFoundation_CenterCell);
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

	if (ObjectClass::CurrentObjects->Count > 0)
	{
		for (const auto& pCurrent : ObjectClass::CurrentObjects())
		{
			if (const auto pCurrentTechno = abstract_cast<TechnoClass*>(pCurrent))
			{
				pTechno = pCurrentTechno;
				break;
			}
		}
	}

	ColorStruct fillColor { 0, 0, 0 };
	RectangleStruct drawRect { 0, 0, 360, DSurface::Composite->GetHeight() - 32 };
	DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 30);
	Point2D textLocation { 15, 5 };

	auto drawText = [&drawRect, &textLocation](const char* pFormat, ...)
	{
		char buffer[0x60] = {0};
		va_list args;
		va_start(args, pFormat);
		vsprintf_s(buffer, pFormat, args);
		va_end(args);
		wchar_t wBuffer[0x60] = {0};
		CRT::mbstowcs(wBuffer, buffer, strlen(buffer));
		constexpr TextPrintType printType = TextPrintType::FullShadow | TextPrintType::Point8;
		DSurface::Composite->DrawTextA(wBuffer, &drawRect, &textLocation, COLOR_WHITE, 0, printType);
		textLocation.Y += 12;
	};

	auto drawInfo = [&drawText](const char* pInfoName, AbstractClass* pCurrent, AbstractClass* pTarget)
	{
		if (pTarget)
		{
			auto mapCoords = CellStruct::Empty;
			auto ID = "N/A";

			if (auto const pObject = abstract_cast<ObjectClass*>(pTarget))
			{
				mapCoords = pObject->GetMapCoords();
				ID = pObject->GetType()->get_ID();
			}
			else if (auto const pCell = abstract_cast<CellClass*>(pTarget))
			{
				mapCoords = pCell->MapCoords;
				ID = "Cell";
			}

			const auto distance = (pCurrent->DistanceFrom(pTarget) / Unsorted::LeptonsPerCell);
			drawText("%s: %s , At( %d , %d ) , %d apart", pInfoName, ID, mapCoords.X, mapCoords.Y, distance);
		}
		else
		{
			drawText("%s: %s", pInfoName, "N/A");
		}
	};

	auto drawTask = [&drawText](const char* pInfoName, Mission mission)
	{
		drawText("%s = %d ( %s )", pInfoName, mission, MissionControlClass::FindName(mission));
	};

	auto drawTime = [&drawText](const char* pInfoName, CDTimerClass& timer)
	{
		const auto timeCeiling = timer.TimeLeft;
		const auto timeCurrent = timeCeiling - timer.GetTimeLeft();
		const auto timePercentage = (timeCeiling > 0) ? (timeCurrent * 100 / timeCeiling) : 0;

		drawText("%s = %d / %d ( %d )", pInfoName, timeCurrent, timeCeiling, timePercentage);
	};

	drawText("Current Frame: %d", Unsorted::CurrentFrame());

	if (pTechno)
	{
		const auto pType = pTechno->GetTechnoType();
		const auto absType = pTechno->WhatAmI();

		if (absType == AbstractType::Unit)
			drawText("%s: %s , UniqueID: %d", "Vehicle", pType->ID, pTechno->UniqueID);
		else if (absType == AbstractType::Infantry)
			drawText("%s: %s , UniqueID: %d", "Infantry", pType->ID, pTechno->UniqueID);
		else if (absType == AbstractType::Aircraft)
			drawText("%s: %s , UniqueID: %d", "Aircraft", pType->ID, pTechno->UniqueID);
		else if (absType == AbstractType::Building)
			drawText("%s: %s , UniqueID: %d", "Building", pType->ID, pTechno->UniqueID);
		else
			drawText("%s: %s , UniqueID: %d", "Unknown", pType->ID, pTechno->UniqueID);

		const auto pOwner = pTechno->Owner;

		if (pOwner)
			drawText("Owner = %s ( %s )", pOwner->get_ID(), pOwner->PlainName);
		else
			drawText("Owner = %s ( %s )", "N/A", "N/A");

		const auto cell = pTechno->GetMapCoords();
		const auto coords = pTechno->GetCoords();

		drawText("Location = [ %d , %d , %d ]( %d , %d , %d )", coords.X, coords.Y, coords.Z, cell.X, cell.Y, pTechno->GetCellLevel());

		constexpr const char* facingTypes[8] = { "North", "NorthEast", "East", "SouthEast", "South", "SouthWest", "West", "NorthWest" };
		const auto facing1 = pTechno->PrimaryFacing.Current();
		const auto facing2 = pTechno->SecondaryFacing.Current();
		const auto primaryFacing = facing1.GetValue<3>();

		drawText("PriFacing = [ %d ( %d )]( %s )", facing1.Raw, facing1.GetValue<5>(), facingTypes[primaryFacing]);
		drawText("SecFacing = [ %d ( %d )]( %s )", facing2.Raw, facing2.GetValue<5>(), facingTypes[facing2.GetValue<3>()]);

		drawText("Tether = ( %s , %s )", (pTechno->IsTether ? "Yes" : "No"), (pTechno->IsAlternativeTether ? "Yes" : "No"));
		drawText("Health = ( %d / %d )", pTechno->Health, pType->Strength);

		const auto pExt = TechnoExt::ExtMap.Find(pTechno);

		drawText("Shield = ( %d / %d )", (pExt->Shield ? pExt->Shield->GetHP() : -1), (pExt->CurrentShieldType ? pExt->CurrentShieldType->Strength : -1));
		drawText("Ammo = ( %d / %d )( %s )", pTechno->Ammo, pType->Ammo, (pType->ManualReload ? "Yes" : "No"));

		drawTime("ReloadTimer", pTechno->ReloadTimer);
		drawTime("RearmTimer", pTechno->RearmTimer);

		drawText("TurretRecoil = %.3f , BarrelRecoil = %.3f", pTechno->TurretRecoil.TravelSoFar, pTechno->BarrelRecoil.TravelSoFar);

		if (pTechno->Passengers.NumPassengers > 0)
		{
			drawText("%d Passengers", pTechno->Passengers.NumPassengers);
			drawText("First Passenger = %s", pTechno->Passengers.FirstPassenger->GetTechnoType()->ID);
		}
		else
		{
			drawText("%d Passengers", 0);
			drawText("First Passenger = %s", "N/A");
		}

		drawInfo("Target", pTechno, pTechno->Target);
		drawInfo("Last Target", pTechno, pTechno->LastTarget);
		drawInfo("Nth Link", pTechno, pTechno->GetNthLink());
		drawInfo("Archive Target", pTechno, pTechno->ArchiveTarget);
		drawInfo("Transporter", pTechno, pTechno->Transporter);
		drawInfo("Enter Target", pTechno, pTechno->QueueUpToEnter);

		if (pTechno->CurrentTargets.Count > 0)
			drawInfo("First CurTarget", pTechno, pTechno->CurrentTargets.GetItem(0));
		else
			drawInfo("First CurTarget", pTechno, nullptr);

		if (pTechno->AttackedTargets.Count > 0)
			drawInfo("First OldTarget", pTechno, pTechno->AttackedTargets.GetItem(0));
		else
			drawInfo("First OldTarget", pTechno, nullptr);

		drawText("Status = %d , StartFrame = %d", pTechno->MissionStatus, pTechno->CurrentMissionStartTime);
		drawTask("CurrentMission", pTechno->CurrentMission);
		drawTask("SuspendMission", pTechno->SuspendedMission);
		drawTask("TheNextMission", pTechno->QueuedMission);

		if (pTechno->AbstractFlags & AbstractFlags::Foot)
		{
			const auto pFoot = static_cast<FootClass*>(pTechno);

			drawTask("TheMegaMission", pFoot->MegaMission);
			drawText("PlanningPathIdx = %d", pFoot->PlanningPathIdx);
			drawText("FootCell = ( %d , %d )", pFoot->CurrentMapCoords.X, pFoot->CurrentMapCoords.Y);
			drawText("LastCell = ( %d , %d )", pFoot->LastMapCoords.X, pFoot->LastMapCoords.Y);

			drawText("PathWaitTime = %d , WaitingBlockage = %s", pFoot->unknown_int_64C, (pFoot->unknown_bool_6B7 ? "Yes" : "No"));
			drawTime("PathFindTimer", pFoot->PathDelayTimer);
			drawTime("BlockageTimer", pFoot->BlockagePathTimer);

			constexpr const char* moveTypes[8] = { "Clear", "Cloak", "Move", "Gate", "A-Block", "E-Block", "Temp", "Unable" };
			const auto facingType = static_cast<FacingType>(primaryFacing);
			const auto moveType = static_cast<int>(pFoot->IsCellOccupied(pFoot->GetCell()->GetNeighbourCell(facingType), facingType, pFoot->GetCellLevel(), nullptr, true));

			drawText("FaceMoveType = ( %s )", (moveType >= 0 && moveType < 8) ? moveTypes[moveType] : "N/A");

			const auto& pD = pFoot->PathDirections;

			if (pD[0] == -1)
				drawText("PathDir = N/A");
			else if (pD[1] == -1)
				drawText("PathDir = %d", pD[0]);
			else if (pD[2] == -1)
				drawText("PathDir = %d , %d", pD[0], pD[1]);
			else if (pD[3] == -1)
				drawText("PathDir = %d , %d , %d", pD[0], pD[1], pD[2]);
			else if (pD[4] == -1)
				drawText("PathDir = %d , %d , %d , %d", pD[0], pD[1], pD[2], pD[3]);
			else if (pD[5] == -1)
				drawText("PathDir = %d , %d , %d , %d , %d", pD[0], pD[1], pD[2], pD[3], pD[4]);
			else if (pD[6] == -1)
				drawText("PathDir = %d , %d , %d , %d , %d , %d", pD[0], pD[1], pD[2], pD[3], pD[4], pD[5]);
			else if (pD[7] == -1)
				drawText("PathDir = %d , %d , %d , %d , %d , %d , %d", pD[0], pD[1], pD[2], pD[3], pD[4], pD[5], pD[6]);
			else
				drawText("PathDir = %d , %d , %d , %d , %d , %d , %d , %d", pD[0], pD[1], pD[2], pD[3], pD[4], pD[5], pD[6], pD[7]);

			drawText("CurrentSpeed = %d", static_cast<int>(pFoot->GetCurrentSpeed()));
			drawText("PercentSpeed = %d", static_cast<int>(pFoot->SpeedPercentage * 100));

			drawInfo("Destination", pFoot, pFoot->Destination);
			drawInfo("Last Destination", pFoot, pFoot->LastDestination);
			drawInfo("Follow Target", pFoot, pFoot->unknown_5A0);
			drawInfo("Patrol Target", pFoot, reinterpret_cast<AbstractClass*>(pFoot->unknown_5DC));
			drawInfo("Mega Target", pFoot, pFoot->MegaTarget);
			drawInfo("Mega Destination", pFoot, pFoot->MegaDestination);
			drawInfo("Parasite", pFoot, pFoot->ParasiteEatingMe);

			if (pFoot->unknown_abstract_array_588.Count > 0)
				drawInfo("First ArrayItem", pFoot, pFoot->unknown_abstract_array_588.GetItem(0));
			else
				drawInfo("First ArrayItem", pFoot, nullptr);

			if (pFoot->NavQueue.Count > 0)
				drawInfo("First Nav-Queue", pFoot, pFoot->NavQueue.GetItem(0));
			else
				drawInfo("First Nav-Queue", pFoot, nullptr);

			drawText("Scattering = %s , Aggressive = %s", (pExt->ScatteringStopFrame >= Unsorted::CurrentFrame() ? "Yes" : "No"), (pExt->AggressiveStance ? "Yes" : "No"));

			if (pFoot->BelongsToATeam())
			{
				const auto pTeam = pFoot->Team;
				const auto pTeamType = pTeam->Type;
				bool found = false;

				for (int i = 0; i < AITriggerTypeClass::Array->Count; i++)
				{
					const auto pTriggerType = AITriggerTypeClass::Array->GetItem(i);

					if (pTeamType && (pTriggerType->Team1 == pTeamType || pTriggerType->Team2 == pTeamType))
					{
						found = true;
						drawText("Trigger = %s", pTriggerType->ID);
						drawText("weights [ Cur , Min , Max ]:");
						drawText(" [ %.2f , %.2f , %.2f ]", pTriggerType->Weight_Current, pTriggerType->Weight_Minimum, pTriggerType->Weight_Maximum);
						break;
					}
				}

				if (!found)
				{
					drawText("Trigger = %s", "N/A");
					drawText("weights [ Cur , Min , Max ]:");
					drawText(" [ %.2f , %.2f , %.2f ]", -1, -1, -1);
				}

				const auto pScriptType = pTeam->CurrentScript->Type;
				const auto mission = pTeam->CurrentScript->CurrentMission;

				drawText("Team    = %s", pTeamType->ID);
				drawText("Task    = %s", pTeamType->TaskForce->ID);
				drawText("Script  = %s", pScriptType->get_ID());
				drawText(" [ Line = Action , Argument ]:");

				if (mission >= 0)
					drawText(" [ %d = %d , %d ]", mission, pScriptType->ScriptActions[mission].Action, pScriptType->ScriptActions[mission].Argument);
				else
					drawText(" [ %d = %d , %d ]", mission, -1, -1);
			}
			else
			{
				drawText("Trigger = %s", "N/A");
				drawText("weights [ Cur , Min , Max ]:");
				drawText(" [ %.2f , %.2f , %.2f ]", -1, -1, -1);
				drawText("Team    = %s", "N/A");
				drawText("Task    = %s", "N/A");
				drawText("Script  = %s", "N/A");
				drawText(" [ Line = Action , Argument ]:");
				drawText(" [ %d = %d , %d ]", -1, -1, -1);
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
		else if (absType == AbstractType::Building)
		{
			const auto pBuilding = static_cast<BuildingClass*>(pTechno);
			const auto pBuildingType = pBuilding->Type;
			const auto pBuildingTypeExt = BuildingTypeExt::ExtMap.Find(pBuildingType);

			if (pBuilding->Occupants.Count > 0)
			{
				drawText("%d Occupants", pBuilding->Occupants.Count);
				drawText("First Occupant = %s", pBuilding->Occupants.GetItem(0)->Type->ID);
			}
			else
			{
				drawText("%d Occupants", 0);
				drawText("First Occupant = %s", "N/A");
			}

			if (pBuilding->Overpowerers.Count > 0)
			{
				drawText("%d Overpowerers", pBuilding->Overpowerers.Count);
				drawText("First Overpowerer = %s", pBuilding->Overpowerers.GetItem(0)->Type->ID);
			}
			else
			{
				drawText("%d Overpowerers", 0);
				drawText("First Overpowerer = %s", "N/A");
			}

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
				drawText("Product: %s ( %d )", pProduct->GetTechnoType()->ID, (pFactory->GetProgress() * 100 / 54));
			else
				drawText("Product: %s ( %d )", "N/A", 0);

			drawTime("RetryProduction", pBuilding->FactoryRetryTimer);
			drawTime("CashProduction", pBuilding->CashProductionTimer);
			drawTime("BuildingGate", pBuilding->GateTimer);

			drawText("BaseNodes: %d , Center = ( %d , %d )", pOwner->Base.BaseNodes.Count, pOwner->Base.Center.X, pOwner->Base.Center.Y);

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
				drawText("SuperWeapon: 0 / -1 ( 0.0 )");

			// Upgrade Status
			if (const auto upgrades = pBuildingType->Upgrades)
			{
				const auto pType1 = pBuilding->Upgrades[0];
				const auto pType2 = pBuilding->Upgrades[1];
				const auto pType3 = pBuilding->Upgrades[2];

				drawText("Upgrades ( %d / %d ):", pBuilding->UpgradeLevel, upgrades);
				drawText("Slot 1 = %s", (pType1 ? pType1->get_ID() : "N/A"));
				drawText("Slot 2 = %s", (pType2 ? pType2->get_ID() : "N/A"));
				drawText("Slot 3 = %s", (pType3 ? pType3->get_ID() : "N/A"));
			}
			else
			{
				drawText("Upgrades ( %d / %d ):", -1, -1);
				drawText("Slot 1 = %s", "N/A");
				drawText("Slot 2 = %s", "N/A");
				drawText("Slot 3 = %s", "N/A");
			}
		}
	}
	else
	{
		const auto mouseXY1 = WWMouseClass::Instance->XY1;
		auto point = mouseXY1 - Point2D { DSurface::ViewBounds->X, DSurface::ViewBounds->Y };
		auto cell = CellStruct::Empty;
		auto coords = CoordStruct::Empty;
		ObjectClass* pObj = nullptr;
		BYTE fogged = 0;
		BYTE shrouded = 0;

		DisplayClass::Instance->ProcessClickCoords(&point, &cell, &coords, &pObj, &fogged, &shrouded);
		const auto pCell = MapClass::Instance->GetCellAt(cell);

		drawText("Cell: slope %d , UniqueID: %d", pCell->SlopeIndex, pCell->UniqueID);

		constexpr const char* landTypes[12] = { "Clear", "Road", "Water", "Rock", "Wall", "Tiberium", "Beach", "Rough", "Ice", "Railroad", "Tunnel", "Weeds" };
		const auto landType = static_cast<int>(pCell->LandType);

		drawText("LandType = ( %s )", (landType >= 0 && landType < 12) ? landTypes[landType] : "Unknown");

		drawText("Location = [ %d , %d , %d ]( %d , %d , %d )", coords.X, coords.Y, coords.Z, cell.X, cell.Y, pCell->GetLevel());

		drawText("CellFlags:");
		drawText("  CenterRevealed --------- %s", (pCell->Flags & CellFlags::CenterRevealed ? "Yes" : "No"));
		drawText("  EdgeRevealed ----------- %s", (pCell->Flags & CellFlags::EdgeRevealed ? "Yes" : "No"));
		drawText("  IsWaypoint ------------- %s", (pCell->Flags & CellFlags::IsWaypoint ? "Yes" : "No"));
		drawText("  Explored --------------- %s", (pCell->Flags & CellFlags::Explored ? "Yes" : "No"));
		drawText("  FlagPresent ------------ %s", (pCell->Flags & CellFlags::FlagPresent ? "Yes" : "No"));
		drawText("  FlagToShroud ----------- %s", (pCell->Flags & CellFlags::FlagToShroud ? "Yes" : "No"));
		drawText("  IsPlot ----------------- %s", (pCell->Flags & CellFlags::IsPlot ? "Yes" : "No"));
		drawText("  BridgeOwner ------------ %s", (pCell->Flags & CellFlags::BridgeOwner ? "Yes" : "No"));
		drawText("  BridgeHead ------------- %s", (pCell->Flags & CellFlags::BridgeHead ? "Yes" : "No"));
		drawText("  Unknown_200 ------------ %s", (pCell->Flags & CellFlags::Unknown_200 ? "Yes" : "No"));
		drawText("  BridgeBody ------------- %s", (pCell->Flags & CellFlags::BridgeBody ? "Yes" : "No"));
		drawText("  BridgeDir -------------- %s", (pCell->Flags & CellFlags::BridgeDir ? "Yes" : "No"));
		drawText("  PixelFX ---------------- %s", (pCell->Flags & CellFlags::PixelFX ? "Yes" : "No"));
		drawText("  Unknown_2000 ----------- %s", (pCell->Flags & CellFlags::Unknown_2000 ? "Yes" : "No"));
		drawText("  Unknown_4000 ----------- %s", (pCell->Flags & CellFlags::Unknown_4000 ? "Yes" : "No"));
		drawText("  Veinhole --------------- %s", (pCell->Flags & CellFlags::Veinhole ? "Yes" : "No"));
		drawText("  DrawDarkenIfInAir ------ %s", (pCell->Flags & CellFlags::DrawDarkenIfInAir ? "Yes" : "No"));
		drawText("  AnimAttached ----------- %s", (pCell->Flags & CellFlags::AnimAttached ? "Yes" : "No"));
		drawText("  Tube ------------------- %s", (pCell->Flags & CellFlags::Tube ? "Yes" : "No"));
		drawText("  EMPPresent ------------- %s", (pCell->Flags & CellFlags::EMPPresent ? "Yes" : "No"));
		drawText("  HorizontalLineEventTag - %s", (pCell->Flags & CellFlags::HorizontalLineEventTag ? "Yes" : "No"));
		drawText("  VerticalLineEventTag --- %s", (pCell->Flags & CellFlags::VerticalLineEventTag ? "Yes" : "No"));
		drawText("  Fogged ----------------- %s", (pCell->Flags & CellFlags::Fogged ? "Yes" : "No"));

		drawText("AltCellFlags:");
		drawText("  Unknown_1 -------------- %s", (pCell->AltFlags & AltCellFlags::Unknown_1 ? "Yes" : "No"));
		drawText("  ContainsBuilding ------- %s", (pCell->AltFlags & AltCellFlags::ContainsBuilding ? "Yes" : "No"));
		drawText("  ContainsBuildingBuffer - %s", (pCell->AltFlags & AltCellFlags::Unknown_4 ? "Yes" : "No"));
		drawText("  Mapped ----------------- %s", (pCell->AltFlags & AltCellFlags::Mapped ? "Yes" : "No"));
		drawText("  NoFog ------------------ %s", (pCell->AltFlags & AltCellFlags::NoFog ? "Yes" : "No"));
		drawText("  Unknown_20 ------------- %s", (pCell->AltFlags & AltCellFlags::Unknown_20 ? "Yes" : "No"));
		drawText("  Unknown_40 ------------- %s", (pCell->AltFlags & AltCellFlags::Unknown_40 ? "Yes" : "No"));
		drawText("  Unknown_80 ------------- %s", (pCell->AltFlags & AltCellFlags::Unknown_80 ? "Yes" : "No"));
		drawText("  Unknown_100 ------------ %s", (pCell->AltFlags & AltCellFlags::Unknown_100 ? "Yes" : "No"));

		drawText("TheOccupationFlags: %u", pCell->OccupationFlags);
		drawText("AltOccupationFlags: %u", pCell->AltOccupationFlags);

		drawInfo("TheFirstObject", pCell, pCell->FirstObject);
		drawInfo("AltFirstObject", pCell, pCell->AltObject);
		drawInfo("CurrentJumpjet", pCell, pCell->Jumpjet);

		drawText("TubeIndex = %d", pCell->TubeIndex);
		drawText("Rad Level = %.2f", pCell->RadLevel);

		drawText(" ");

		drawText("Mouse: ( %d , %d )", mouseXY1.X, mouseXY1.Y);
	}
}

#pragma endregion

#pragma region HerosButtons

inline bool TacticalButtonsClass::IndexInHerosButtons()
{
	return this->ButtonIndex > 0 && this->ButtonIndex <= 8;
}

BSurface* TacticalButtonsClass::GetMissingCameo(SHPStruct* pSHP)
{
	const auto pCameoRef = pSHP->AsReference();
	char pFilename[0x20];
	strcpy_s(pFilename, RulesExt::Global()->MissingCameo.data());
	_strlwr_s(pFilename);

	if (!_stricmp(pCameoRef->Filename, GameStrings::XXICON_SHP) && strstr(pFilename, ".pcx"))
	{
		PCX::Instance->LoadFile(pFilename);

		if (const auto MissingCameoPCX = PCX::Instance->GetSurface(pFilename))
			return MissingCameoPCX;
	}

	return nullptr;
}

void TacticalButtonsClass::HerosDraw()
{
	if (!this->HeroVisible || HouseClass::CurrentPlayer->IsObserver())
		return;

	auto& vec = ScenarioExt::Global()->OwnedHeros;
	const int size = vec.size();

	if (!size)
		return;

	Point2D position { DSurface::Composite->GetWidth() - 65, 35 };
	RectangleStruct drawRect { position.X, position.Y, 60, 48 };
	const int recordIndex = this->ButtonIndex - 1;
	int counts = Math::min(8, size);

	for (int i = 0; i < counts; ++i, position.Y += 50, drawRect.Y = position.Y)
	{
		const auto pExt = vec[i];
		const auto pTechno = pExt->OwnerObject();
		const auto pTypeExt = pExt->TypeExtData;
		const auto pType = pTypeExt->OwnerObject();

		if (const auto CameoPCX = pTypeExt->CameoPCX.GetSurface())
		{
			PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
		}
		else if (const auto pSHP = pType->GetCameo())
		{
			if (const auto MissingCameoPCX = this->GetMissingCameo(pSHP))
			{
				PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
			}
			else
			{
				RectangleStruct rect { 0, 0, position.X + 60, position.Y + 48 };
				DSurface::Composite->DrawSHP(pTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
					BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
			}
		}

		if (!pTechno->InLimbo)
		{
			const auto pRules = RulesClass::Instance();
			auto ratio = pTechno->GetHealthPercentage();

			if (pTechno->IsIronCurtained())
			{
				ColorStruct fillColor { 50, 50, 50 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20);
			}
			else
			{
				int time = Unsorted::CurrentFrame - pExt->LastHurtFrame;

				if (ratio < pRules->ConditionRed)
				{
					ColorStruct fillColor { 255, 0, 0 };
					int trans = 40 - time;

					if (trans < 0)
					{
						const int round = time % 60;
						trans = ((round <= 20) ? 0 : ((round <= 40) ? (round - 20) : (60 - round)));
					}

					if (trans > 0)
						DSurface::Composite->FillRectTrans(&drawRect, &fillColor, trans);
				}
				else if (ratio < pRules->ConditionYellow)
				{
					ColorStruct fillColor { 255, 0, 0 };
					int trans = 30 - time;

					if (trans < 0)
					{
						const int round = time % 160;
						trans = ((round <= 140) ? 0 : ((round <= 150) ? (round - 140) : (160 - round)));
					}

					if (trans > 0)
						DSurface::Composite->FillRectTrans(&drawRect, &fillColor, trans);
				}
				else if (time < 20)
				{
					ColorStruct fillColor { 255, 0, 0 };
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, (20 - time));
				}

				time = Unsorted::CurrentFrame - pTechno->LastFireBulletFrame;

				if (time < 20)
				{
					ColorStruct fillColor { 255, 255, 0 };
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20 - time);
				}

				if (pTechno->TemporalTargetingMe)
				{
					ColorStruct fillColor { 100, 100, 255 };
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
				}
				else if (pTechno->AirstrikeTintStage)
				{
					ColorStruct fillColor { 255, 50, 0 };
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
				}
				else if (pTechno->DrainingMe || pTechno->LocomotorSource)
				{
					ColorStruct fillColor { 200, 0, 255 };
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
				}
				else if (pTechno->IsUnderEMP())
				{
					ColorStruct fillColor { 128, 128, 128 };
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
				}
			}

			if (pTechno->BunkerLinkedItem && pTechno->WhatAmI() != AbstractType::Building)
			{
				RectangleStruct rect { (position.X + 3), (position.Y + 1), 54, 7 };
				DSurface::Composite->DrawRect(&rect, 0x781F);
			}

			RectangleStruct rect { (position.X + 4), (position.Y + 2), 52, 5 };
			DSurface::Composite->FillRect(&rect, 0);

			++rect.X;
			++rect.Y;
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			rect.Height = 3;

			const int color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
			DSurface::Composite->FillRect(&rect, color);

			const auto pShield = pExt->Shield.get();

			if (pShield && !pShield->IsBrokenAndNonRespawning())
			{
				ratio = (static_cast<double>(pShield->GetHP()) / pShield->GetType()->Strength.Get());
				rect.Width = static_cast<int>(50 * ratio + 0.5);
				ColorStruct fillColor { 153, 153, 255 };
				DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
			}

			if (pTechno->IsIronCurtained())
			{
				const auto timer = &pTechno->IronCurtainTimer;
				ratio = static_cast<double>(timer->GetTimeLeft()) / timer->TimeLeft;
				rect.Width = static_cast<int>(50 * ratio + 0.5);
				ColorStruct fillColor { 200, 50, 50 };
				DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
			}
		}
		else if (auto pSelect = pTechno->Transporter)
		{
			for (auto pTrans = pSelect; pTrans; pTrans = pTrans->Transporter)
				pSelect = pTrans;

			const auto pSelectExt = TechnoExt::ExtMap.Find(pSelect);
			const auto pRules = RulesClass::Instance();
			auto ratio = pTechno->GetHealthPercentage();

			if (pSelect->IsIronCurtained())
			{
				ColorStruct fillColor { 50, 50, 50 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20);
			}
			else
			{
				int time = Unsorted::CurrentFrame - pSelectExt->LastHurtFrame;

				if (ratio < pRules->ConditionRed)
				{
					ColorStruct fillColor { 255, 0, 0 };
					int trans = 40 - time;

					if (trans < 0)
					{
						const int round = time % 60;
						trans = ((round <= 20) ? 0 : ((round <= 40) ? (round - 20) : (60 - round)));
					}

					if (trans > 0)
						DSurface::Composite->FillRectTrans(&drawRect, &fillColor, trans);
				}
				else if (ratio < pRules->ConditionYellow)
				{
					ColorStruct fillColor { 255, 0, 0 };
					int trans = 30 - time;

					if (trans < 0)
					{
						const int round = time % 160;
						trans = ((round <= 140) ? 0 : ((round <= 150) ? (round - 140) : (160 - round)));
					}

					if (trans > 0)
						DSurface::Composite->FillRectTrans(&drawRect, &fillColor, trans);
				}
				else if (time < 20)
				{
					ColorStruct fillColor { 255, 0, 0 };
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, (20 - time));
				}

				time = Unsorted::CurrentFrame - pSelect->LastFireBulletFrame;

				if (time < 20)
				{
					ColorStruct fillColor { 255, 255, 0 };
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 20 - time);
				}

				if (pSelect->TemporalTargetingMe)
				{
					ColorStruct fillColor { 100, 100, 255 };
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
				}
				else if (pSelect->LocomotorSource)
				{
					ColorStruct fillColor { 200, 0, 255 };
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
				}
				else if (pSelect->IsUnderEMP())
				{
					ColorStruct fillColor { 128, 128, 128 };
					DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 25);
				}
			}

			RectangleStruct rect { (position.X + 3), (position.Y + 1), 0, 7 };

			if (pSelect->BunkerLinkedItem && pSelect->WhatAmI() != AbstractType::Building)
			{
				rect.Width = 54;
				DSurface::Composite->DrawRect(&rect, 0x781F);
			}
			else
			{
				rect.Width = static_cast<int>(54 * pSelect->GetHealthPercentage() + 0.5);
				DSurface::Composite->DrawRect(&rect, 0xFB20);
			}

			++rect.X;
			++rect.Y;
			rect.Width = 52;
			rect.Height = 5;

			DSurface::Composite->FillRect(&rect, 0);

			++rect.X;
			++rect.Y;
			rect.Width = static_cast<int>(50 * ratio + 0.5);
			rect.Height = 3;

			const int color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
			DSurface::Composite->FillRect(&rect, color);

			const auto pShield = pSelectExt->Shield.get();

			if (pShield && !pShield->IsBrokenAndNonRespawning())
			{
				ratio = (static_cast<double>(pShield->GetHP()) / pShield->GetType()->Strength.Get());
				rect.Width = static_cast<int>(50 * ratio + 0.5);
				ColorStruct fillColor { 153, 153, 255 };
				DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
			}

			if (pSelect->IsIronCurtained())
			{
				const auto timer = &pSelect->IronCurtainTimer;
				ratio = static_cast<double>(timer->GetTimeLeft()) / timer->TimeLeft;
				rect.Width = static_cast<int>(50 * ratio + 0.5);
				ColorStruct fillColor { 200, 50, 50 };
				DSurface::Composite->FillRectTrans(&rect, &fillColor, 80);
			}
		}
		else
		{
			const auto absType = pTechno->WhatAmI();
			const auto buildCat = (absType == AbstractType::Building) ? static_cast<BuildingClass*>(pTechno)->Type->BuildCat : BuildCat::DontCare;
			const auto pFactory = pTechno->Owner->GetPrimaryFactory(absType, pType->Naval, buildCat);

			if (pFactory && pFactory->Object == pTechno)
			{
				ColorStruct fillColor { 0, 0, 0 };
				DSurface::Composite->FillRectTrans(&drawRect, &fillColor, 30);

				RectangleStruct rect { (position.X + 4), (position.Y + 2), 52, 5 };
				DSurface::Composite->FillRect(&rect, 0);

				const auto ratio = static_cast<double>(pFactory->GetProgress()) / 54;
				rect = RectangleStruct { (position.X + 5), (position.Y + 3), static_cast<int>(50 * ratio), 3 };
				DSurface::Composite->FillRect(&rect, 0xFFFF);
			}
			else
			{
				RectangleStruct rect { (position.X + 3), (position.Y + 1), 54, 7 };
				DSurface::Composite->DrawRect(&rect, 0x781F);

				++rect.X;
				++rect.Y;
				rect.Width = 52;
				rect.Height = 5;

				DSurface::Composite->FillRect(&rect, 0);

				const auto ratio = pTechno->GetHealthPercentage();

				++rect.X;
				++rect.Y;
				rect.Width = static_cast<int>(50 * ratio + 0.5);
				rect.Height = 3;

				const auto pRules = RulesClass::Instance();
				const int color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
				DSurface::Composite->FillRect(&rect, color);
			}
		}

		if (i == recordIndex && !ScenarioClass::Instance->UserInputLocked)
		{
			RectangleStruct rect { 0, 0, position.X + 60, position.Y + 48 };
			DSurface::Composite->DrawRectEx(&rect, &drawRect, Drawing::RGB_To_Int(Drawing::TooltipColor));
		}
	}
}

void TacticalButtonsClass::HeroSelect(int buttonIndex)
{
	if (ScenarioClass::Instance->UserInputLocked || !this->HeroVisible)
		return;

	auto& vec = ScenarioExt::Global()->OwnedHeros;
	const int index = buttonIndex - 1;

	if (index >= static_cast<int>(vec.size()))
		return;

	const auto pTechno = vec[index]->OwnerObject();

	if (pTechno->IsAlive)
	{
		auto pSelect = pTechno;

		for (auto pTrans = pSelect->Transporter; pTrans; pTrans = pTrans->Transporter)
			pSelect = pTrans;

		if (ObjectClass::CurrentObjects->Count != 1 || !pSelect->IsSelected)
			MapClass::UnselectAll();

		if (!pSelect->InLimbo && !pSelect->Select())
			TacticalClass::Instance->SetTacticalPosition(&pSelect->Location);
	}
}

void TacticalButtonsClass::HeroSwitch()
{
	this->HeroVisible = !this->HeroVisible;

	MessageListClass::Instance->PrintMessage
	(
		(this->HeroVisible ?
			GeneralUtils::LoadStringUnlessMissing("TXT_HEROS_VISIBLE", L"Set heros info visible.") :
			GeneralUtils::LoadStringUnlessMissing("TXT_HEROS_INVISIBLE", L"Set heros info invisible.")),
		RulesClass::Instance->MessageDelay,
		HouseClass::CurrentPlayer->ColorSchemeIndex,
		true
	);
}

#pragma endregion

#pragma region SelectButtons

inline bool TacticalButtonsClass::IndexInSelectButtons()
{
	return this->ButtonIndex > 10 && this->ButtonIndex <= 100;
}

inline void TacticalButtonsClass::AddToCurrentSelect(TechnoTypeExt::ExtData* pTypeExt, int count, int checkIndex)
{
	const auto groupID = pTypeExt->GetSelectionGroupID();
	const int currentCounts = this->CurrentSelectCameo.size();

	for (int i = checkIndex; i < currentCounts; ++i)
	{
		if (this->CurrentSelectCameo[i].TypeExt->GetSelectionGroupID() == groupID)
		{
			this->CurrentSelectCameo[i].Count += count;
			return;
		}
	}

	SelectRecordStruct select { pTypeExt, count };
	this->CurrentSelectCameo.push_back(select);
}

BSurface* TacticalButtonsClass::SearchMissingCameo(AbstractType absType, SHPStruct* pSHP)
{
	const auto pRulesExt = RulesExt::Global();
	const auto pCameoRef = pSHP->AsReference();
	char pFilename[0x20];
	strcpy_s(pFilename, pRulesExt->MissingCameo.data());
	_strlwr_s(pFilename);

	if (!_stricmp(pCameoRef->Filename, GameStrings::XXICON_SHP))
	{
		if (absType == AbstractType::InfantryType)
		{
			if (const auto MissingCameoPCX = pRulesExt->SelectedInfantryMissingPCX.GetSurface())
				return MissingCameoPCX;
		}
		else if (absType == AbstractType::UnitType)
		{
			if (const auto MissingCameoPCX = pRulesExt->SelectedVehicleMissingPCX.GetSurface())
				return MissingCameoPCX;
		}
		else if (absType == AbstractType::AircraftType)
		{
			if (const auto MissingCameoPCX = pRulesExt->SelectedAircraftMissingPCX.GetSurface())
				return MissingCameoPCX;
		}
		else if (absType == AbstractType::BuildingType)
		{
			if (const auto MissingCameoPCX = pRulesExt->SelectedBuildingMissingPCX.GetSurface())
				return MissingCameoPCX;
		}

		if (strstr(pFilename, ".pcx"))
		{
			PCX::Instance->LoadFile(pFilename);

			if (const auto MissingCameoPCX = PCX::Instance->GetSurface(pFilename))
				return MissingCameoPCX;
		}
	}

	return nullptr;
}

void TacticalButtonsClass::SelectedTrigger(int buttonIndex, bool select)
{
	if (ScenarioClass::Instance->UserInputLocked || !Phobos::Config::SelectedDisplay_Enable)
		return;

	const int currentCounts = this->CurrentSelectCameo.size();

	if (buttonIndex > (currentCounts + 10))
		return;

	const auto pTypeExt = this->CurrentSelectCameo[buttonIndex - 11].TypeExt;
	const auto groupID = pTypeExt->GetSelectionGroupID();
	const auto shiftPressed = InputManagerClass::Instance->IsForceSelectKeyPressed();

	if (select)
	{
		if (shiftPressed)
		{
			std::vector<ObjectClass*> deselects;
			deselects.reserve(ObjectClass::CurrentObjects->Count);

			for (const auto& pCurrent : ObjectClass::CurrentObjects())
			{
				if (const auto pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() == groupID)
						continue;
				}

				deselects.push_back(pCurrent);
			}

			for (const auto& pDeselect : deselects)
				pDeselect->Deselect();
		}
		else
		{
			const int counts = ObjectClass::CurrentObjects->Count;
			std::vector<ObjectClass*> selects;
			selects.reserve(counts);
			std::vector<ObjectClass*> deselects;
			deselects.reserve(counts);

			for (const auto& pCurrent : ObjectClass::CurrentObjects())
			{
				if (const auto pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() == groupID)
					{
						selects.push_back(pCurrent);
						continue;
					}
				}

				deselects.push_back(pCurrent);
			}

			for (const auto& pDeselect : deselects)
				pDeselect->Deselect();

			const int size = selects.size();
			const int random = Unsorted::CurrentFrame % size;

			for (int i = 0; i < size; ++i)
			{
				if (i != random)
					selects[i]->Deselect();
			}
		}
	}
	else
	{
		if (shiftPressed)
		{
			std::vector<ObjectClass*> deselects;
			deselects.reserve(ObjectClass::CurrentObjects->Count);

			for (const auto& pCurrent : ObjectClass::CurrentObjects())
			{
				if (const auto pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() != groupID)
						continue;
				}

				deselects.push_back(pCurrent);
			}

			for (const auto& pDeselect : deselects)
				pDeselect->Deselect();
		}
		else
		{
			std::vector<ObjectClass*> selects;
			selects.reserve(ObjectClass::CurrentObjects->Count);

			for (const auto& pCurrent : ObjectClass::CurrentObjects())
			{
				if (const auto pCurrentTypeExt = TechnoTypeExt::ExtMap.Find(pCurrent->GetTechnoType()))
				{
					if (pCurrentTypeExt->GetSelectionGroupID() == groupID)
						selects.push_back(pCurrent);
				}
			}

			selects[Unsorted::CurrentFrame % selects.size()]->Deselect();
		}
	}
}

void TacticalButtonsClass::SelectedUpdate()
{
	this->UpdateSelect = false;

	if (this->CurrentSelectCameo.size())
		this->CurrentSelectCameo.clear();

	if (ObjectClass::CurrentObjects->Count <= 0)
	{
		this->RecordIndex = 11;
		return;
	}

	this->CurrentSelectCameo.reserve(10);

	std::map<int, int> CurrentSelectInfantry;
	std::map<int, int> CurrentSelectUnit;
	std::map<int, int> CurrentSelectAircraft;
	std::map<int, int> CurrentSelectBuilding;

	for (const auto& pCurrent : ObjectClass::CurrentObjects())
	{
		const auto absType = pCurrent->WhatAmI();

		if (absType == AbstractType::Infantry)
			++CurrentSelectInfantry[static_cast<InfantryClass*>(pCurrent)->Type->ArrayIndex];
		else if (absType == AbstractType::Unit)
			++CurrentSelectUnit[static_cast<UnitClass*>(pCurrent)->Type->ArrayIndex];
		else if (absType == AbstractType::Aircraft)
			++CurrentSelectAircraft[static_cast<AircraftClass*>(pCurrent)->Type->ArrayIndex];
		else if (absType == AbstractType::Building)
			++CurrentSelectBuilding[static_cast<BuildingClass*>(pCurrent)->Type->ArrayIndex];
	}

	if (CurrentSelectInfantry.size())
	{
		for (const auto& [index, count] : CurrentSelectInfantry)
		{
			if (const auto pTypeExt = TechnoTypeExt::ExtMap.Find(InfantryTypeClass::Array->Items[index]))
				this->AddToCurrentSelect(pTypeExt, count, 0);
		}
	}

	if (CurrentSelectUnit.size())
	{
		const int checkStart = this->CurrentSelectCameo.size();

		for (const auto& [index, count] : CurrentSelectUnit)
		{
			if (const auto pTypeExt = TechnoTypeExt::ExtMap.Find(UnitTypeClass::Array->Items[index]))
				this->AddToCurrentSelect(pTypeExt, count, checkStart);
		}
	}

	if (CurrentSelectAircraft.size())
	{
		const int checkStart = this->CurrentSelectCameo.size();

		for (const auto& [index, count] : CurrentSelectAircraft)
		{
			if (const auto pTypeExt = TechnoTypeExt::ExtMap.Find(AircraftTypeClass::Array->Items[index]))
				this->AddToCurrentSelect(pTypeExt, count, checkStart);
		}
	}

	if (CurrentSelectBuilding.size())
	{
		const int checkStart = this->CurrentSelectCameo.size();

		for (const auto& [index, count] : CurrentSelectBuilding)
		{
			if (const auto pTypeExt = TechnoTypeExt::ExtMap.Find(BuildingTypeClass::Array->Items[index]))
				this->AddToCurrentSelect(pTypeExt, count, checkStart);
		}
	}

	const int maxIndex = this->CurrentSelectCameo.size() + 10;

	if (this->RecordIndex > maxIndex)
		this->RecordIndex = maxIndex;
}

void TacticalButtonsClass::SelectedDraw()
{
	if (!Phobos::Config::SelectedDisplay_Enable)
		return;

	if (this->UpdateSelect)
		this->SelectedUpdate();

	const int currentCounts = Math::min(30, static_cast<int>(this->CurrentSelectCameo.size()));

	if (currentCounts > 1 || (currentCounts && this->CurrentSelectCameo[0].Count > 1)) // Mass
	{
		Point2D position { 0, DSurface::Composite->GetHeight() - 80 };
		Point2D textPosition { 1, position.Y + 1 };
		RectangleStruct drawRect { 0, position.Y, 60, 48 };

		const int gap = Math::min(60, ((DSurface::Composite->GetWidth() << 1) / (5 * currentCounts)));
		RectangleStruct surfaceRect { 0, 0, (gap * (currentCounts - 1) + 60), (position.Y + 48) };

		const int maxIndex = currentCounts + 10;
		const COLORREF color = Drawing::RGB_To_Int(Drawing::TooltipColor);
		TextPrintType printType = TextPrintType::Background | TextPrintType::FullShadow | TextPrintType::Point8;

		for (int i = 11; i < this->RecordIndex && i <= maxIndex; ++i, position.X += gap, drawRect.X = position.X, textPosition.X += gap)
		{
			const auto pSelect = this->CurrentSelectCameo[i - 11];
			const auto pTypeExt = pSelect.TypeExt;

			if (const auto CameoPCX = pTypeExt->CameoPCX.GetSurface())
			{
				PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
			}
			else if (const auto pSHP = pTypeExt->OwnerObject()->GetCameo())
			{
				if (const auto MissingCameoPCX = this->SearchMissingCameo(pTypeExt->OwnerObject()->WhatAmI(), pSHP))
				{
					PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
				}
				else
				{
					RectangleStruct rect { 0, 0, (position.X + 60), surfaceRect.Height };
					DSurface::Composite->DrawSHP(pTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
						BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
				}
			}

			const int count = pSelect.Count;

			if (count > 1)
			{
				wchar_t text[0x20];
				swprintf_s(text, L"%d", count);
				DSurface::Composite->DrawTextA(text, &surfaceRect, &textPosition, color, 0, printType);
			}
		}

		position.X = surfaceRect.Width - 60;
		drawRect.X = position.X;

		printType |= TextPrintType::Right;
		textPosition.X = surfaceRect.Width - 1;

		for (int i = maxIndex; i > this->RecordIndex; --i, position.X -= gap, drawRect.X = position.X, textPosition.X -= gap)
		{
			const auto pSelect = this->CurrentSelectCameo[i - 11];
			const auto pTypeExt = pSelect.TypeExt;

			if (const auto CameoPCX = pTypeExt->CameoPCX.GetSurface())
			{
				PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
			}
			else if (const auto pSHP = pTypeExt->OwnerObject()->GetCameo())
			{
				if (const auto MissingCameoPCX = this->SearchMissingCameo(pTypeExt->OwnerObject()->WhatAmI(), pSHP))
				{
					PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
				}
				else
				{
					RectangleStruct rect { 0, 0, (position.X + 60), surfaceRect.Height };
					DSurface::Composite->DrawSHP(pTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
						BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
				}
			}

			const int count = pSelect.Count;

			if (count > 1)
			{
				wchar_t text[0x20];
				swprintf_s(text, L"%d", count);
				DSurface::Composite->DrawTextA(text, &surfaceRect, &textPosition, color, 0, printType);
			}
		}

		const bool hovered = this->ButtonIndex == this->RecordIndex;

		if (hovered)
		{
			position.Y -= 10;
			drawRect.Y -= 10;
			textPosition.Y -= 10;

			ColorStruct fillColor { 0, 0, 0 };
			RectangleStruct fillRect { position.X, position.Y + 48, 60, 10 };
			DSurface::Composite->FillRectTrans(&fillRect, &fillColor, 60);
		}

		if (this->RecordIndex <= maxIndex)
		{
			const auto pSelect = this->CurrentSelectCameo[this->RecordIndex - 11];
			const auto pTypeExt = pSelect.TypeExt;

			if (const auto CameoPCX = pTypeExt->CameoPCX.GetSurface())
			{
				PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
			}
			else if (const auto pSHP = pTypeExt->OwnerObject()->GetCameo())
			{
				if (const auto MissingCameoPCX = this->SearchMissingCameo(pTypeExt->OwnerObject()->WhatAmI(), pSHP))
				{
					PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
				}
				else
				{
					RectangleStruct rect { 0, 0, (position.X + 60), (position.Y + 48) };
					DSurface::Composite->DrawSHP(pTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL), pSHP, 0, &position, &rect,
						BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
				}
			}

			const int count = pSelect.Count;

			if (count > 1)
			{
				wchar_t text[0x20];
				swprintf_s(text, L"%d", count);
				DSurface::Composite->DrawTextA(text, &surfaceRect, &textPosition, color, 0, printType);
			}
		}
	}
	else if (currentCounts) // Only 1
	{
		for (const auto& pCurrent : ObjectClass::CurrentObjects())
		{
			if (const auto pThis = abstract_cast<TechnoClass*>(pCurrent))
			{
				const auto pType = pThis->GetTechnoType();
				const auto pTypeExt = this->CurrentSelectCameo[0].TypeExt;

				if (pTypeExt->OwnerObject() == pType)
				{
					TextPrintType printType = TextPrintType::Center | TextPrintType::Point8;
					COLORREF color = Drawing::RGB_To_Int(Drawing::TooltipColor);
					RectangleStruct drawRect { 0, DSurface::Composite->GetHeight() - 80, 180, 48};
					{
						DSurface::Composite->FillRect(&drawRect, COLOR_BLACK);
						drawRect.X += 60;
						drawRect.Width = 120;
						DSurface::Composite->DrawRect(&drawRect, color);
						drawRect.X -= 60;
					}
					drawRect.Width = 60;
					Point2D position { 60, drawRect.Y };
					RectangleStruct surfaceRect { 0, 0, 180, position.Y + 48 };

					auto CameoPCX = pTypeExt->CameoPCX.GetSurface();
					auto pSHP = pType->GetCameo();
					auto pPal = pTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL);

					position += Point2D { 60, 3 };
					{
						const auto pHouse = HouseClass::CurrentPlayer();
						auto name = pType->UIName;
						const auto pOwner = pThis->Owner;

						if ((!pOwner || !pOwner->IsAlliedWith(pHouse)) && !pHouse->IsObserver())
						{
							if (pThis->IsDisguisedAs(pHouse))
							{
								const auto pDisguiseType = TechnoTypeExt::GetTechnoType(pThis->Disguise);

								if (const auto pDisguiseTypeExt = TechnoTypeExt::ExtMap.Find(pDisguiseType))
								{
									if (const auto pFakeType = pDisguiseTypeExt->FakeOf)
									{
										if (const auto pFakeTypeExt = TechnoTypeExt::ExtMap.Find(pFakeType))
										{
											if (const auto fakeName = pFakeTypeExt->EnemyUIName.Get().Text)
												name = fakeName;
											else
												name = pFakeType->UIName;

											if (const auto FakeCameoPCX = pFakeTypeExt->CameoPCX.GetSurface())
											{
												CameoPCX = FakeCameoPCX;
											}
											else if (const auto pFakeSHP = pFakeType->GetCameo())
											{
												pSHP = pFakeSHP;
												pPal = pFakeTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL);
											}
										}
										else
										{
											name = pFakeType->UIName;
											pSHP = pFakeType->GetCameo();
											pPal = FileSystem::CAMEO_PAL;
										}
									}
									else
									{
										if (const auto fakeName = pDisguiseTypeExt->EnemyUIName.Get().Text)
											name = fakeName;
										else
											name = pDisguiseType->UIName;

										if (const auto disguiseCameoPCX = pDisguiseTypeExt->CameoPCX.GetSurface())
										{
											CameoPCX = disguiseCameoPCX;
										}
										else if (const auto pDisguiseSHP = pDisguiseType->GetCameo())
										{
											pSHP = pDisguiseSHP;
											pPal = pDisguiseTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL);
										}
									}
								}
								else if (pDisguiseType) // May disguise as an object
								{
									name = pDisguiseType->UIName;
									pSHP = pDisguiseType->GetCameo();
									pPal = FileSystem::CAMEO_PAL;
								}
							}
							else if (const auto pFakeType = pTypeExt->FakeOf)
							{
								if (const auto pFakeTypeExt = TechnoTypeExt::ExtMap.Find(pFakeType))
								{
									if (const auto fakeName = pFakeTypeExt->EnemyUIName.Get().Text)
										name = fakeName;
									else
										name = pFakeType->UIName;

									if (const auto FakeCameoPCX = pFakeTypeExt->CameoPCX.GetSurface())
									{
										CameoPCX = FakeCameoPCX;
									}
									else if (const auto pFakeSHP = pFakeType->GetCameo())
									{
										pSHP = pFakeSHP;
										pPal = pFakeTypeExt->CameoPal.GetOrDefaultConvert(FileSystem::CAMEO_PAL);
									}
								}
								else
								{
									name = pFakeType->UIName;
									pSHP = pFakeType->GetCameo();
									pPal = FileSystem::CAMEO_PAL;
								}
							}
							else if (const auto enemyName = pTypeExt->EnemyUIName.Get().Text)
							{
								name = enemyName;
							}
						}

						if (name)
						{
							size_t length = Math::min(wcslen(name), static_cast<size_t>(31));

							for (auto check = name; *check != L'\0'; ++check)
							{
								if (*check == L'\n')
								{
									length = static_cast<size_t>(check - name);
									break;
								}
							}

							wchar_t text1[0x20] = {0};
							wcsncpy_s(text1, name, length);
							text1[length] = L'\0';
							DSurface::Composite->DrawTextA(text1, &surfaceRect, &position, color, 0, printType);
						}
					}

					position.Y += 15;
					{
						int value = -1, maxValue = 0;
						TechnoExt::GetValuesForDisplay(pThis, pTypeExt->SelectedInfo_UpperType, value, maxValue);

						if (value >= 0 && maxValue > 0)
						{
							if (pTypeExt->SelectedInfo_UpperColor.Get() == ColorStruct{ 0, 0, 0 })
							{
								const auto pRules = RulesClass::Instance();
								const auto ratio = static_cast<double>(value) / maxValue;
								color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
							}
							else
							{
								color = Drawing::RGB_To_Int(pTypeExt->SelectedInfo_UpperColor);
							}

							wchar_t text2[0x20] = {0};

							position.X += 10;
							printType &= ~TextPrintType::Center;
							swprintf_s(text2, L"%d", maxValue);
							DSurface::Composite->DrawTextA(text2, &surfaceRect, &position, color, 0, printType);

							position.X -= 20;
							printType |= TextPrintType::Right;
							swprintf_s(text2, L"%d", value);
							DSurface::Composite->DrawTextA(text2, &surfaceRect, &position, color, 0, printType);

							position.X += 10;
							printType &= ~TextPrintType::Right;
							printType |= TextPrintType::Center;
							DSurface::Composite->DrawTextA(L"/", &surfaceRect, &position, color, 0, printType);
						}
						else
						{
							DSurface::Composite->DrawTextA(L"-- / --", &surfaceRect, &position, color, 0, printType);
						}
					}

					position.Y += 13;
					{
						int value = -1, maxValue = 0;
						TechnoExt::GetValuesForDisplay(pThis, pTypeExt->SelectedInfo_BelowType, value, maxValue);

						if (value >= 0 && maxValue > 0)
						{
							if (pTypeExt->SelectedInfo_BelowColor.Get() == ColorStruct{ 0, 0, 0 })
							{
								const auto pRules = RulesClass::Instance();
								const auto ratio = static_cast<double>(value) / maxValue;
								color = (ratio > pRules->ConditionYellow) ? 0x67EC : (ratio > pRules->ConditionRed ? 0xFFEC : 0xF986);
							}
							else
							{
								color = Drawing::RGB_To_Int(pTypeExt->SelectedInfo_BelowColor);
							}

							wchar_t text3[0x20] = {0};

							position.X += 10;
							printType &= ~TextPrintType::Center;
							swprintf_s(text3, L"%d", maxValue);
							DSurface::Composite->DrawTextA(text3, &surfaceRect, &position, color, 0, printType);

							position.X -= 20;
							printType |= TextPrintType::Right;
							swprintf_s(text3, L"%d", value);
							DSurface::Composite->DrawTextA(text3, &surfaceRect, &position, color, 0, printType);

							position.X += 10;
							printType &= ~TextPrintType::Right;
							printType |= TextPrintType::Center;
							DSurface::Composite->DrawTextA(L"/", &surfaceRect, &position, color, 0, printType);
						}
						else
						{
							DSurface::Composite->DrawTextA(L"-- / --", &surfaceRect, &position, color, 0, printType);
						}
					}

					if (CameoPCX)
					{
						PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, CameoPCX);
					}
					else if (pSHP)
					{
						if (const auto MissingCameoPCX = this->SearchMissingCameo((pTypeExt->FakeOf ? pTypeExt->FakeOf->WhatAmI() : pType->WhatAmI()), pSHP))
						{
							PCX::Instance->BlitToSurface(&drawRect, DSurface::Composite, MissingCameoPCX);
							return;
						}

						position -= Point2D { 120, 31 };
						surfaceRect.Width = 60;
						DSurface::Composite->DrawSHP(pPal, pSHP, 0, &position, &surfaceRect, BlitterFlags::bf_400, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
					}

					return;
				}
			}
		}
	}
}

#pragma endregion

#pragma region SelectSwitch

void TacticalButtonsClass::SelectedSwitch()
{
	Phobos::Config::SelectedDisplay_Enable = !Phobos::Config::SelectedDisplay_Enable;

	if (Phobos::Config::SelectedDisplay_Enable)
	{
		this->UpdateSelect = true;
		MessageListClass::Instance->PrintMessage
		(
			GeneralUtils::LoadStringUnlessMissing("TXT_SELECT_VISIBLE", L"Set select info visible."),
			RulesClass::Instance->MessageDelay,
			HouseClass::CurrentPlayer->ColorSchemeIndex,
			true
		);
	}
	else
	{
		this->RecordIndex = 11;
		MessageListClass::Instance->PrintMessage
		(
			GeneralUtils::LoadStringUnlessMissing("TXT_SELECT_INVISIBLE", L"Set select info invisible."),
			RulesClass::Instance->MessageDelay,
			HouseClass::CurrentPlayer->ColorSchemeIndex,
			true
		);
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

DEFINE_HOOK(0x6D4941, TacticalClass_Render_DrawButtonCameo, 0x6)
{
	const auto pButtons = &TacticalButtonsClass::Instance;
	pButtons->FPSCounterDraw();

	// TODO New buttons (The later draw, the higher layer)

	pButtons->SelectedDraw();
	pButtons->HerosDraw();

	return 0;
}

DEFINE_HOOK(0x4F4583, GScreenClass_DrawCurrentSelectInfo, 0x6)
{
	TacticalButtonsClass::Instance.CurrentSelectInfoDraw();
	return 0;
}

#pragma endregion

#pragma region HerosHooks

DEFINE_HOOK(0x7015C9, TechnoClass_SetOwningHouse_ChangeHeroOwner, 0x6)
{
	GET(TechnoClass* const, pThis, ESI);
	GET(HouseClass* const, pNewOwner, EBP);

	const auto pExt = TechnoExt::ExtMap.Find(pThis);

	if (pExt->TypeExtData->UniqueTechno)
	{
		const auto pOldOwner = pThis->Owner;

		if (pOldOwner->IsControlledByCurrentPlayer())
		{
			if (!pNewOwner->IsControlledByCurrentPlayer())
			{
				auto& vec = ScenarioExt::Global()->OwnedHeros;
				vec.erase(std::remove(vec.begin(), vec.end(), pExt), vec.end());
			}
		}
		else if (pNewOwner->IsControlledByCurrentPlayer())
		{
			auto& vec = ScenarioExt::Global()->OwnedHeros;

			if (std::find(vec.begin(), vec.end(), pExt) == vec.end())
				vec.push_back(pExt);
		}
	}

	return 0;
}

#pragma endregion

#pragma region SelectHooks

DEFINE_HOOK_AGAIN(0x5F4718, ObjectClass_Select, 0x7)
DEFINE_HOOK(0x5F46AE, ObjectClass_Select, 0x7)
{
	GET(ObjectClass*, pThis, ESI);

	pThis->IsSelected = true;

	if (Phobos::Config::ShowFlashOnSelecting && RulesExt::Global()->SelectionFlashDuration > 0 && pThis->GetOwningHouse()->IsControlledByCurrentPlayer())
		pThis->Flash(RulesExt::Global()->SelectionFlashDuration);

	TacticalButtonsClass::Instance.UpdateSelect = true;

	return 0;
}

DEFINE_HOOK(0x5F44FC, ObjectClass_Deselect, 0x7)
{
	GET(ObjectClass*, pThis, ESI);

	pThis->IsSelected = false;

	TacticalButtonsClass::Instance.UpdateSelect = true;

	return 0;
}

#pragma endregion

#pragma region ToolTips

DEFINE_HOOK(0x4AE511, DisplayClass_GetToolTip_SkipTacticalTip, 0x5)
{
	enum { UseButtonTip = 0x4AE5F8, SkipGameCode = 0x4AE69B };

	const auto pButtons = &TacticalButtonsClass::Instance;
	const auto buttonIndex = pButtons->GetButtonIndex();

	if (buttonIndex < 0)
		return 0;

	if (!buttonIndex)
		return SkipGameCode;

	if (pButtons->IndexInHerosButtons())
		R->EAX(pButtons->HoveredHero);
	else if (pButtons->IndexInSelectButtons())
		R->EAX(pButtons->HoveredSelected);
	else
		R->EAX(0);

	return UseButtonTip;
}

#pragma endregion
