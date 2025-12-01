#include <Ext/Techno/Body.h>
#include <Ext/BuildingType/Body.h>
#include <Ext/House/Body.h>
#include <Ext/Anim/Body.h>
#include <Utilities/EnumFunctions.h>

#pragma region EnterRefineryFix

DEFINE_HOOK(0x74312A, UnitClass_SetDestination_ReplaceWithHarvestMission, 0x5)
{
	enum { SkipGameCode = 0x742F48 };

	GET(UnitClass*, pThis, EBP);

	// Jumpjet will overlap when entering buildings,
	// which can cause errors in the connection between jumpjet harvester and refinery building,
	// leading to game crashes in drawing
	// Here change the Mission::Enter to Mission::Harvest
	pThis->QueueMission(Mission::Harvest, false);
	pThis->NextMission();
	pThis->MissionStatus = 2; // Status: returning to refinery
	pThis->IsHarvesting = false;
	// Note: jumpjet harvester should not be allowed to comply with this behavior alone, otherwise
	// it may still overlap with other types and crash

	return SkipGameCode;
}

DEFINE_HOOK(0x73E739, UnitClass_Mission_Harvest_SkipUselessArchiveTarget, 0x5)
{
	enum { SkipGameCode = 0x73E755 };

	GET(UnitClass*, pThis, EBP);
	GET(AbstractClass*, pFocus, EAX); // pThis->ArchiveTarget

	// Removing unnecessary set destination
	// This can effectively reduce the ineffective actions when Harvester automatically returning
	// to work after be manually operated to return to Refinery.
	if (pFocus->WhatAmI() != AbstractType::Building || pThis->GetCell()->GetBuilding() != pFocus)
		return 0;

	// Clear ArchiveTarget to avoid checking again next time
	pThis->ArchiveTarget = nullptr;

	return SkipGameCode;
}

#pragma endregion

#pragma region JumpjetHarvesters

DEFINE_HOOK(0x74613C, UnitClass_INoticeSink_CheckJumpjetHarvester, 0x6)
{
	GET(UnitClass*, pThis, ESI);

	const auto pType = pThis->Type;

	// Let jumpjet harvesters automatically go mining when leaving the factory
	if (pType->Harvester || pType->Weeder)
	{
		// Have checked pThis->HasAnyLink()
		if (const auto pBuilding = abstract_cast<BuildingClass*, true>(pThis->GetNthLink()))
		{
			// Only need to check WeaponsFactory
			if (pBuilding->Type->WeaponsFactory)
				pThis->QueueMission(Mission::Harvest, true);
		}
	}

	return 0;
}

#pragma endregion
/*
DEFINE_HOOK(0x740943, UnitClass_Mission_Guard_PlayerHarvester, 0x6)
{
	enum { SkipGameCode = 0x7408C7, ReturnFromFunction = 0x7409EF };

	GET(UnitClass*, pThis, ESI);

	if (pThis->Type->Teleporter || pThis->Type->MovementZone == MovementZone::Subterrannean)
	{
		auto const pCell = pThis->GetCell();
		int cellIndex = 0;

		while (true)
		{
			if (auto const pBuilding = pCell->GetNeighbourCell((FacingType)cellIndex)->GetBuilding())
			{
				if (pBuilding->Type->Refinery && pBuilding->Owner == pThis->Owner)
				{
					pThis->QueueMission(Mission::Harvest, false);
					return ReturnFromFunction;
				}
			}

			if (++cellIndex >= (int)FacingType::Count)
			{
				double percentage = pThis->GetStoragePercentage();

				if (pThis->ArchiveTarget && percentage > 0.0)
				{
					pThis->QueueMission(Mission::Harvest, false);
					return ReturnFromFunction;
				}
				else if (percentage != 1.0 && percentage >= 0.0)
				{
					return SkipGameCode;
				}

				if (!pThis->Locomotor->Is_Moving())
					return SkipGameCode;

				pThis->QueueMission(Mission::Harvest, false);
				return ReturnFromFunction;
			}
		}
	}

	return SkipGameCode;
}
*/
#pragma region HarvesterQuickUnloader

void __fastcall ArrivingRefineryNearBy(UnitClass* pThis, BuildingClass* pDock)
{
	pDock->UpdateRefinerySmokeSystems();
	reinterpret_cast<int(__thiscall*)(BuildingClass*, int, int, bool, int)>(0x451750)(pDock, 7, !pDock->IsGreenHP(), 0, 0); //BuildingClass::PlayAnimByIdx
	reinterpret_cast<int(__thiscall*)(BuildingClass*, int, int, bool, int)>(0x451750)(pDock, 8, !pDock->IsGreenHP(), 0, 0);

	const auto pOwner = pDock->GetOwningHouse();

	if (pThis->Type->Weeder)
	{
		bool playAnim = true;

		while (true)
		{
			const auto idx = reinterpret_cast<int(__fastcall*)(StorageClass*)>(0x6C9820)(&pThis->Tiberium);

			if (idx == -1)
				break;

			auto amount = pThis->Tiberium.GetAmount(idx);
			amount = pThis->Tiberium.RemoveAmount(amount, idx);

			if (amount <= 0.0)
				continue;

			playAnim = false;
			reinterpret_cast<int(__thiscall*)(HouseClass*, int, int)>(0x4F9700)(pOwner, static_cast<int>(amount), idx);
		}

		if (playAnim && pDock->Anims[10])
			pDock->DestroyNthAnim(BuildingAnimSlot::Special);
	}
	else
	{
		auto numPurifier = pOwner->NumOrePurifiers;

		if (!pOwner->IsHumanPlayer && !SessionClass::IsCampaign())
			numPurifier = RulesClass::Instance->AIVirtualPurifiers.Items[static_cast<int>(pOwner->AIDifficulty)] + numPurifier;

		const auto multiplier = numPurifier * RulesClass::Instance->PurifierBonus;
		int money = 0;

		while (true)
		{
			const auto idx = reinterpret_cast<int(__fastcall*)(StorageClass*)>(0x6C9820)(&pThis->Tiberium);

			if (idx == -1)
				break;

			auto amount = pThis->Tiberium.GetAmount(idx);
			amount = pThis->Tiberium.RemoveAmount(amount, idx);

			if (amount <= 0.0)
				continue;

			money += static_cast<int>(amount * TiberiumClass::Array.Items[idx]->Value);
			const auto amountFromPurifier = amount * multiplier;

			if (amountFromPurifier > 0.0)
				money += static_cast<int>(amountFromPurifier * TiberiumClass::Array.Items[idx]->Value);
		}

		if (money)
		{
			pOwner->GiveMoney(money);

			if (const auto pDockTypeExt = BuildingTypeExt::ExtMap.Find(pDock->Type))
			{
				const auto pRulesExt = RulesExt::Global();

				if ((pRulesExt->DisplayIncome_AllowAI || pDock->Owner->IsControlledByHuman()) && pDockTypeExt->DisplayIncome.Get(pRulesExt->DisplayIncome))
				{
					FlyingStrings::AddMoneyString(
						money,
						pDock,
						pDock->Owner,
						pDockTypeExt->DisplayIncome_Houses.Get(pRulesExt->DisplayIncome_Houses.Get()),
						pDock->GetRenderCoords(),
						pDockTypeExt->DisplayIncome_Offset
					);
				}
			}

			// pThis->Animation.Value = 0;
		}
		else if (pDock->Anims[10])
		{
			pDock->DestroyNthAnim(BuildingAnimSlot::Special);
		}
	}

	pThis->MissionStatus = 0;
	pThis->SetDestination(nullptr, false);
}

// MissionStatus == 2 means the unit is returning.
DEFINE_HOOK(0x73EB2C, UnitClass_MissionHarvest_Status2, 0x6)
{
	enum { SkipGameCode = 0x73EE85 };

	GET(UnitClass* const, pThis, EBP);

	const auto pType = pThis->Type;
	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	if (!pTypeExt->HarvesterQuickUnloader)
		return 0;

	std::vector<BuildingTypeClass*> docks;

	if (const auto dockCount = pType->Dock.Count)
	{
		docks.reserve(dockCount);

		for (const auto& pBuildingType : pType->Dock)
		{
			if (pBuildingType)
				docks.push_back(pBuildingType);
		}
	}

	if (!docks.size())
		return SkipGameCode;

	// Check arrived
	const auto pThisCell = pThis->GetCell();
	const auto pHouse = pThis->Owner;

	for (int i = 0; i < 8; ++i)
	{
		if (const auto pBuilding = pThisCell->GetNeighbourCell(static_cast<FacingType>(i))->GetBuilding())
		{
			const auto pCellBuildingType = pBuilding->Type;

			for (const auto& pBuildingType : docks)
			{
				if (pCellBuildingType == pBuildingType && pBuilding->Owner == pHouse)
				{
					ArrivingRefineryNearBy(pThis, pBuilding);
					return SkipGameCode;
				}
			}
		}
	}

	// Check destination
	if (const auto pDestination = pThis->Destination)
	{
		if (Unsorted::CurrentFrame - HouseExt::ExtMap.Find(pHouse)->LastRefineryBuildFrame >= pThis->UpdateTimer.TimeLeft)
		{
			const auto pDestinationCell = (pDestination->WhatAmI() == AbstractType::Cell)
				? static_cast<CellClass*>(pDestination)
				: MapClass::Instance.GetCellAt(pDestination->GetCoords());

			for (int i = 0; i < 8; ++i)
			{
				if (const auto pBuilding = pDestinationCell->GetNeighbourCell(static_cast<FacingType>(i))->GetBuilding())
				{
					const auto pCellBuildingType = pBuilding->Type;

					for (const auto& pBuildingType : docks)
					{
						if (pCellBuildingType == pBuildingType && pBuilding->Owner == pHouse)
							return SkipGameCode;
					}
				}
			}
		}
	}

	// Find nearest dock
	const auto thisLocation = pThis->GetCoords();
	const auto thisPosition = Point2D { (thisLocation.X >> 4), (thisLocation.Y >> 4) };

	auto move = pType->MovementZone;

	if (pType->Teleporter && (move == MovementZone::AmphibiousCrusher || move == MovementZone::AmphibiousDestroyer))
		move = MovementZone::Amphibious;
	else if (move == MovementZone::Subterrannean)
		move = MovementZone::Fly;

	const auto destLocation = pThis->GetDestination();
	auto destCell = CellStruct { static_cast<short>(destLocation.X >> 8), static_cast<short>(destLocation.Y >> 8) };

	int distanceSquared = INT_MAX;
	BuildingClass* pDock = nullptr;

	for (const auto& pBuildingType : docks)
	{
		for (const auto& pBuilding : pHouse->Buildings)
		{
			if (pBuilding && pBuilding->Type == pBuildingType && !pBuilding->InLimbo) // Prevent check radio links
			{
				const auto dockLocation = pBuilding->GetCoords();
				auto dockCell = CellStruct { static_cast<short>(dockLocation.X >> 8), static_cast<short>(dockLocation.Y >> 8) };

				if (reinterpret_cast<bool(__thiscall*)(DisplayClass*, CellStruct*, CellStruct*, MovementZone, bool, bool, bool)>(0x56D100)
					(&DisplayClass::Instance, &destCell, &dockCell, move, pThis->IsOnBridge(), false ,false)) // Prevent send command
				{
					const auto difference = Point2D { (thisPosition.X - (dockLocation.X >> 4)), (thisPosition.Y - (dockLocation.Y >> 4)) };
					const auto newDistanceSquared = (difference.X * difference.X) + (difference.Y * difference.Y);

					if (newDistanceSquared < distanceSquared) // No check for primary building
					{
						distanceSquared = newDistanceSquared;
						pDock = pBuilding;
					}
				}
			}
		}
	}

	if (!pDock)
	{
		pThis->SetDestination(nullptr, true);
		return SkipGameCode;
	}

	// Find a final destination
	const auto dockLocation = pDock->GetCoords();
	destCell = CellClass::Coord2Cell(dockLocation);

	if (thisLocation.X < dockLocation.X && !(pDock->Type->GetFoundationWidth() & 1))
		destCell.X--;

	if (thisLocation.Y < dockLocation.Y && !(pDock->Type->GetFoundationHeight(false) & 1))
		destCell.Y--;

	auto closeTo = CellStruct::Empty;

	if (distanceSquared > 6400)
		closeTo = CellClass::Coord2Cell(thisLocation);

	destCell = MapClass::Instance.NearByLocation(destCell, pType->SpeedType, -1, move, false, 1, 1, false, false, false, true, closeTo, false, false);

	if (destCell == CellStruct::Empty)
	{
		pThis->SetDestination(nullptr, true);
		return SkipGameCode;
	}

	const auto pDestCell = MapClass::Instance.TryGetCellAt(destCell);

	if (!pDestCell || !pType->Teleporter)
	{
		pThis->SetDestination(pDestCell, true);
		return SkipGameCode;
	}

	// Teleporters
	if (const auto ParasiteEatingMe = pThis->ParasiteEatingMe)
		ParasiteEatingMe->ParasiteImUsing->ExitUnit();

	pThis->Mark(MarkType::Up);
	auto sound = pType->ChronoOutSound;

	if (sound != -1 || (sound = RulesClass::Instance->ChronoOutSound, sound != -1))
		VocClass::PlayAt(sound, thisLocation, 0);

	if (pTypeExt->WarpIn.size() > 0)
		AnimExt::CreateRandomAnim(pTypeExt->WarpIn, pThis->Location, nullptr, pHouse);
	else if (const auto pWarpIn = RulesClass::Instance->WarpIn)
		GameCreate<AnimClass>(pWarpIn, pThis->Location)->Owner = pHouse;

	pThis->SetLocation(pDestCell->GetCoords());
	pThis->OnBridge = pDestCell->ContainsBridge();
	pThis->SetHeight(0);
	pThis->Mark(MarkType::Down);
	pThis->UpdatePosition(PCPType::End);
	pThis->Locomotor->Stop_Moving();
	// pThis->SetDestination(nullptr, true);
	pDestCell->CollectCrate(pThis);
	pThis->unknown_280 = 0;

	if ((sound = pType->ChronoInSound, sound != -1) || (sound = RulesClass::Instance->ChronoInSound, sound != -1))
		VocClass::PlayAt(sound, pThis->Location, 0);

	if (pTypeExt->WarpOut.size() > 0)
		AnimExt::CreateRandomAnim(pTypeExt->WarpOut, pThis->Location, nullptr, pHouse);
	else if (const auto pWarpOut = RulesClass::Instance->WarpOut)
		GameCreate<AnimClass>(pWarpOut, pThis->Location)->Owner = pHouse;

	return SkipGameCode;
}

DEFINE_HOOK(0x441226, BuildingClass_Unlimbo_RecheckRefinery, 0x6)
{
	GET(BuildingClass* const, pThis, ESI);

	if (pThis->Type->Refinery && pThis->Owner)
		HouseExt::ExtMap.Find(pThis->Owner)->LastRefineryBuildFrame = Unsorted::CurrentFrame;

	return 0;
}

#pragma endregion

DEFINE_HOOK(0x73E411, UnitClass_Mission_Unload_DumpAmount, 0x7)
{
	enum { SkipGameCode = 0x73E41D };

	GET(UnitClass*, pThis, ESI);
	GET(const int, tiberiumIdx, EBP);
	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pThis->Type);
	const float totalAmount = pThis->Tiberium.GetAmount(tiberiumIdx);
	float dumpAmount = pTypeExt->HarvesterDumpAmount.Get(RulesExt::Global()->HarvesterDumpAmount);

	if (dumpAmount <= 0.0f || totalAmount < dumpAmount)
		dumpAmount = totalAmount;

	__asm fld dumpAmount;

	return SkipGameCode;
}

DEFINE_HOOK(0x4D6D34, FootClass_MissionAreaGuard_Miner, 0x5)
{
	enum { GoGuardArea = 0x4D6D69 };

	GET(FootClass*, pThis, ESI);

	auto const pTypeExt = TechnoExt::ExtMap.Find(pThis)->TypeExtData;

	return pTypeExt->Harvester_CanGuardArea && pThis->Owner->IsControlledByHuman() ? GoGuardArea : 0;
}

#pragma region HarvesterScanAfterUnload

DEFINE_HOOK(0x73E730, UnitClass_MissionHarvest_HarvesterScanAfterUnload, 0x5)
{
	GET(UnitClass* const, pThis, EBP);
	GET(AbstractClass* const, pFocus, EAX);

	const auto pType = pThis->Type;
	// Focus is set when the harvester is fully loaded and go home.
	if (pFocus && !pType->Weeder && TechnoTypeExt::ExtMap.Find(pType)->HarvesterScanAfterUnload.Get(RulesExt::Global()->HarvesterScanAfterUnload))
	{
		auto cellBuffer = CellStruct::Empty;
		const auto pCellStru = pThis->ScanForTiberium(&cellBuffer, RulesClass::Instance->TiberiumLongScan / Unsorted::LeptonsPerCell, 0);

		if (*pCellStru != CellStruct::Empty)
		{
			const auto pCell = MapClass::Instance.TryGetCellAt(*pCellStru);
			const auto distFromTiberium = pCell ? pThis->DistanceFrom(pCell) : -1;

			// Check if pCell is better than focus.
			if (distFromTiberium >= 0 && distFromTiberium < pThis->DistanceFrom(pFocus))
				R->EAX(pCell);
		}
	}

	return 0;
}

#pragma endregion

// Hooks that allow harvesters / weeders to work correctly with MovementZone=Subterannean (sic) - Starkku
#pragma region SubterraneanHarvesters

// Allow scanning for docks in all map zones.
DEFINE_HOOK(0x4DEFC6, FootClass_FindDock_SubterraneanHarvester, 0x5)
{
	GET(TechnoTypeClass*, pTechnoType, EAX);

	if (auto const pUnitType = abstract_cast<UnitTypeClass*>(pTechnoType))
	{
		if ((pUnitType->Harvester || pUnitType->Weeder) && pUnitType->MovementZone == MovementZone::Subterrannean)
			R->ECX(MovementZone::Fly);
	}

	return 0;
}

// Allow scanning for ore in all map zones.
DEFINE_HOOK(0x4DCF86, FootClass_FindTiberium_SubterraneanHarvester, 0x5)
{
	enum { SkipGameCode = 0x4DCF9B };

	GET(MovementZone, mZone, ECX);

	if (mZone == MovementZone::Subterrannean)
		R->ECX(MovementZone::Fly);

	return 0;
}

// Allow scanning for weeds in all map zones.
DEFINE_HOOK(0x4DDB23, FootClass_FindWeeds_SubterraneanHarvester, 0x5)
{
	enum { SkipGameCode = 0x4DCF9B };

	GET(MovementZone, mZone, EAX);

	if (mZone == MovementZone::Subterrannean)
		R->EAX(MovementZone::Fly);

	return 0;
}

// Set flag on factory exit to mark the harvester as having just exited factory.
DEFINE_HOOK(0x44459A, BuildingClass_ExitObject_SubterraneanHarvester, 0x5)
{
	GET(TechnoClass*, pThis, EDI);

	if (auto const pUnit = abstract_cast<UnitClass*>(pThis))
	{
		auto const pType = pUnit->Type;

		if ((pType->Harvester || pType->Weeder) && pType->MovementZone == MovementZone::Subterrannean)
		{
			auto const pExt = TechnoExt::ExtMap.Find(pUnit);
			pExt->SubterraneanHarvStatus = 1;
			pExt->SubterraneanHarvRallyPoint = pThis->ArchiveTarget;
			pThis->ArchiveTarget = nullptr;
		}
	}

	return 0;
}

// Apply same special rules on idle to player-owned subterranean harvesters as to Teleporter=yes ones.
DEFINE_HOOK(0x740949, UnitClass_Mission_Guard_SubterraneanHarvester, 0x6)
{
	enum { Continue = 0x740957 };

	GET(UnitTypeClass*, pType, ECX);

	if (pType->MovementZone == MovementZone::Subterrannean)
		return Continue;

	return 0;
}

// Fix an edge case issue stemming from subterranean unit nav queue handling leading
// to harvesters becoming idle randomly while harvesting.
DEFINE_HOOK(0x738A3E, UnitClass_EnterIdleMode_SubterraneanHarvester, 0x5)
{
	enum { ReturnFromFunction = 0x738D21 };

	GET(UnitClass*, pThis, ESI);

	if (auto const pUnit = abstract_cast<UnitClass*>(pThis))
	{
		auto const pType = pUnit->Type;

		if ((pType->Harvester || pType->Weeder) && pType->MovementZone == MovementZone::Subterrannean)
		{
			auto const mission = pUnit->CurrentMission;

			if (mission == Mission::Unload || mission == Mission::Harvest)
				return ReturnFromFunction;
		}
	}

	return 0;
}

#pragma endregion
