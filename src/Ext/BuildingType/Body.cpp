#include "Body.h"

#include <EventClass.h>
#include <TacticalClass.h>

#include <Utilities/GeneralUtils.h>
#include <Ext/TechnoType/Body.h>
#include <Ext/House/Body.h>
#include <Ext/SWType/Body.h>
#include <Ext/Scenario/Body.h>

BuildingTypeExt::ExtContainer BuildingTypeExt::ExtMap;

// Assuming SuperWeapon & SuperWeapon2 are used (for the moment)
int BuildingTypeExt::ExtData::GetSuperWeaponCount() const
{
	// The user should only use SuperWeapon and SuperWeapon2 if the attached sw count isn't bigger than 2
	return 2 + this->SuperWeapons.size();
}

int BuildingTypeExt::ExtData::GetSuperWeaponIndex(const int index, HouseClass* pHouse) const
{
	auto idxSW = this->GetSuperWeaponIndex(index);

	if (auto pSuper = pHouse->Supers.GetItemOrDefault(idxSW))
	{
		auto pExt = SWTypeExt::ExtMap.Find(pSuper->Type);

		if (!pExt->IsAvailable(pHouse))
			return -1;
	}

	return idxSW;
}

int BuildingTypeExt::ExtData::GetSuperWeaponIndex(const int index) const
{
	const auto pThis = this->OwnerObject();

	// 2 = SuperWeapon & SuperWeapon2
	if (index < 2)
		return !index ? pThis->SuperWeapon : pThis->SuperWeapon2;
	else if (index - 2 < (int)this->SuperWeapons.size())
		return this->SuperWeapons[index - 2];

	return -1;
}

int BuildingTypeExt::GetEnhancedPower(BuildingClass* pBuilding, HouseClass* pHouse)
{
	int nAmount = 0;
	float fFactor = 1.0f;

	auto const pHouseExt = HouseExt::ExtMap.Find(pHouse);

	for (const auto& [bTypeIdx, nCount] : pHouseExt->PowerPlantEnhancers)
	{
		auto bTypeExt = BuildingTypeExt::ExtMap.Find(BuildingTypeClass::Array->Items[bTypeIdx]);
		if (bTypeExt->PowerPlantEnhancer_Buildings.Contains(pBuilding->Type))
		{
			fFactor *= std::powf(bTypeExt->PowerPlantEnhancer_Factor, static_cast<float>(nCount));
			nAmount += bTypeExt->PowerPlantEnhancer_Amount * nCount;
		}
	}

	return static_cast<int>(std::round(pBuilding->GetPowerOutput() * fFactor)) + nAmount;
}

int BuildingTypeExt::GetUpgradesAmount(BuildingTypeClass* pBuilding, HouseClass* pHouse) // not including producing upgrades
{
	int result = 0;
	bool isUpgrade = false;
	auto pPowersUp = pBuilding->PowersUpBuilding;

	auto checkUpgrade = [pHouse, pBuilding, &result, &isUpgrade](BuildingTypeClass* pTPowersUp)
	{
		isUpgrade = true;
		for (auto const& pBld : pHouse->Buildings)
		{
			if (pBld->Type == pTPowersUp)
			{
				for (auto const& pUpgrade : pBld->Upgrades)
				{
					if (pUpgrade == pBuilding)
						++result;
				}
			}
		}
	};

	if (pPowersUp[0])
	{
		if (auto const pTPowersUp = BuildingTypeClass::Find(pPowersUp))
			checkUpgrade(pTPowersUp);
	}

	if (auto pBuildingExt = BuildingTypeExt::ExtMap.Find(pBuilding))
	{
		for (auto pTPowersUp : pBuildingExt->PowersUp_Buildings)
			checkUpgrade(pTPowersUp);
	}

	return isUpgrade ? result : -1;
}

// Check whether can call the occupiers leave
bool BuildingTypeExt::CheckOccupierCanLeave(HouseClass* pBuildingHouse, HouseClass* pOccupierHouse)
{
	if (!pOccupierHouse || !pBuildingHouse)
		return false;
	else if (pBuildingHouse == pOccupierHouse)
		return true;
	else if (pOccupierHouse->IsAlliedWith(pBuildingHouse))
		return true;
	else if (SessionClass::IsCampaign() && pBuildingHouse->IsControlledByHuman() && pOccupierHouse->IsControlledByHuman())
		return true;

	return false;
}

// Force occupiers leave, return: whether it should stop right now
bool BuildingTypeExt::CleanUpBuildingSpace(BuildingTypeClass* pBuildingType, CellStruct topLeftCell, HouseClass* pHouse, TechnoClass* pExceptTechno)
{
	// Step 1: Find the technos inside of the building place grid.
	auto infantryCount = CellStruct::Empty;
	std::vector<TechnoClass*> checkedTechnos;
	checkedTechnos.reserve(24);
	std::vector<CellClass*> checkedCells;
	checkedCells.reserve(24);

	for (auto pFoundation = pBuildingType->GetFoundationData(false); *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
	{
		auto currentCell = topLeftCell + *pFoundation;

		if (const auto pCell = MapClass::Instance->TryGetCellAt(currentCell))
		{
			auto pObject = pCell->FirstObject;

			while (pObject)
			{
				const auto absType = pObject->WhatAmI();

				if (absType == AbstractType::Infantry || absType == AbstractType::Unit)
				{
					const auto pCellTechno = static_cast<TechnoClass*>(pObject);
					const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pCellTechno->GetTechnoType());

					if ((!pTypeExt || !pTypeExt->CanBeBuiltOn) && pCellTechno != pExceptTechno) // No need to check house
					{
						const auto pFoot = static_cast<FootClass*>(pCellTechno);

						if (pFoot->GetCurrentSpeed() <= 0 || !pFoot->Locomotor->Is_Moving())
						{
							if (absType == AbstractType::Infantry)
								++infantryCount.X;

							checkedTechnos.push_back(pCellTechno);
						}
					}
				}

				pObject = pObject->NextObject;
			}

			checkedCells.push_back(pCell);
		}
	}

	if (checkedTechnos.size() <= 0) // All in moving
		return false;

	// Step 2: Find the cells around the building.
	std::vector<CellClass*> optionalCells;
	optionalCells.reserve(24);

	for (auto pFoundation = pBuildingType->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
	{
		auto searchCell = topLeftCell + *pFoundation;

		if (const auto pSearchCell = MapClass::Instance->TryGetCellAt(searchCell))
		{
			if (std::find(checkedCells.begin(), checkedCells.end(), pSearchCell) == checkedCells.end() // TODO If there is a cellflag (or CellExt) that can be used …
				&& !pSearchCell->GetBuilding()
				&& pSearchCell->IsClearToMove(SpeedType::Amphibious, true, true, -1, MovementZone::Amphibious, -1, false))
			{
				optionalCells.push_back(pSearchCell);
			}
		}
	}

	if (optionalCells.size() <= 0) // There is no place for scattering
		return true;

	// Step 3: Sort the technos by the distance out of the foundation.
	std::sort(&checkedTechnos[0], &checkedTechnos[checkedTechnos.size()],[optionalCells](TechnoClass* pTechnoA, TechnoClass* pTechnoB)
	{
		int minA = INT_MAX;
		int minB = INT_MAX;

		for (const auto& pOptionalCell : optionalCells) // If there are many valid cells at start, it means most of occupiers will near to the edge
		{
			if (minA > 65536) // If distance squared is lower or equal to 256^2, then no need to calculate any more because it is on the edge
			{
				auto curA = static_cast<int>(pTechnoA->GetMapCoords().DistanceFromSquared(pOptionalCell->MapCoords));

				if (curA < minA)
					minA = curA;
			}

			if (minB > 65536)
			{
				auto curB = static_cast<int>(pTechnoB->GetMapCoords().DistanceFromSquared(pOptionalCell->MapCoords));

				if (curB < minB)
					minB = curB;
			}
		}

		return minA > minB;
	});

	// Step 4: Core, successively find the farthest techno and its closest valid destination.
	std::vector<TechnoClass*> reCheckedTechnos;
	reCheckedTechnos.reserve(12);

	struct InfantryCountInCell // Temporary struct
	{
		CellClass* position;
		int count;
	};
	std::vector<InfantryCountInCell> infantryCells;
	infantryCells.reserve(4);

	struct TechnoWithDestination // Also temporary struct
	{
		TechnoClass* techno;
		CellClass* destination;
	};
	std::vector<TechnoWithDestination> finalOrder;
	finalOrder.reserve(24);

	do
	{
		// Step 4.1: Push the technos discovered just now back to the vector.
		for (const auto& pRecheckedTechno : reCheckedTechnos)
		{
			if (pRecheckedTechno->WhatAmI() == AbstractType::Infantry)
				++infantryCount.X;

			checkedTechnos.push_back(pRecheckedTechno);
		}

		reCheckedTechnos.clear();

		// Step 4.2: Check the techno vector.
		for (const auto& pCheckedTechno : checkedTechnos)
		{
			CellClass* pDestinationCell = nullptr;

			// Step 4.2.1: Search the closest valid cell to be the destination.
			do
			{
				const auto location = pCheckedTechno->GetMapCoords();
				const bool isInfantry = pCheckedTechno->WhatAmI() == AbstractType::Infantry;
				const auto pCheckedType = pCheckedTechno->GetTechnoType();

				if (isInfantry) // Try to maximizing cells utilization
				{
					if (infantryCells.size() && infantryCount.Y >= (infantryCount.X / 3 + (infantryCount.X % 3 ? 1 : 0)))
					{
						std::sort(&infantryCells[0], &infantryCells[infantryCells.size()],[location](InfantryCountInCell cellA, InfantryCountInCell cellB){
							return cellA.position->MapCoords.DistanceFromSquared(location) < cellB.position->MapCoords.DistanceFromSquared(location);
						});

						for (auto& infantryCell : infantryCells)
						{
							if (static_cast<InfantryClass*>(pCheckedTechno)->Destination == infantryCell.position)
							{
								infantryCell.count = 3;
							}
							else if (infantryCell.count < 3 && infantryCell.position->IsClearToMove(pCheckedType->SpeedType, true, true, -1, pCheckedType->MovementZone, -1, false))
							{
								pDestinationCell = infantryCell.position;
								++infantryCell.count;

								break;
							}
						}

						if (pDestinationCell)
							break; // Complete
					}
				}

				std::sort(&optionalCells[0], &optionalCells[optionalCells.size()],[location](CellClass* pCellA, CellClass* pCellB){
					return pCellA->MapCoords.DistanceFromSquared(location) < pCellB->MapCoords.DistanceFromSquared(location);
				});
				const auto minDistanceSquared = optionalCells[0]->MapCoords.DistanceFromSquared(location);

				for (const auto& pOptionalCell : optionalCells) // Prioritize selecting empty cells
				{
					if (!pOptionalCell->FirstObject && pOptionalCell->IsClearToMove(pCheckedType->SpeedType, true, true, -1, pCheckedType->MovementZone, -1, false))
					{
						if (isInfantry) // Not need to remove it now
						{
							infantryCells.push_back(InfantryCountInCell{ pOptionalCell, 1 });
							++infantryCount.Y;
						}

						if (pOptionalCell->MapCoords.DistanceFromSquared(location) < (minDistanceSquared * 4)) // Empty cell is not too far
							pDestinationCell = pOptionalCell;

						break;
					}
				}

				if (!pDestinationCell)
				{
					std::vector<CellClass*> deleteCells;
					deleteCells.reserve(8);

					for (const auto& pOptionalCell : optionalCells)
					{
						auto pCurObject = pOptionalCell->FirstObject;
						std::vector<TechnoClass*> optionalTechnos;
						optionalTechnos.reserve(4);
						bool valid = true;

						while (pCurObject)
						{
							const auto absType = pCurObject->WhatAmI();

							if (absType == AbstractType::Infantry || absType == AbstractType::Unit)
							{
								const auto pCurTechno = static_cast<TechnoClass*>(pCurObject);

								if (!BuildingTypeExt::CheckOccupierCanLeave(pHouse, pCurTechno->Owner)) // Means invalid for all
								{
									deleteCells.push_back(pOptionalCell);
									valid = false;
									break;
								}

								optionalTechnos.push_back(pCurTechno);
							}

							pCurObject = pCurObject->NextObject;
						}

						if (valid && pOptionalCell->IsClearToMove(pCheckedType->SpeedType, true, true, -1, pCheckedType->MovementZone, -1, false))
						{
							for (const auto& pOptionalTechno : optionalTechnos)
							{
								reCheckedTechnos.push_back(pOptionalTechno);
							}

							if (isInfantry) // Not need to remove it now
							{
								infantryCells.push_back(InfantryCountInCell{ pOptionalCell, 1 });
								++infantryCount.Y;
							}

							pDestinationCell = pOptionalCell;
							break;
						}
					}

					for (const auto& pDeleteCell : deleteCells) // Mark the invalid cells
					{
						checkedCells.push_back(pDeleteCell);
						optionalCells.erase(std::remove(optionalCells.begin(), optionalCells.end(), pDeleteCell), optionalCells.end());
					}
				}
			}
			while (false);

			// Step 4.2.2: Mark the cell and push back its surrounded cells, then prepare for the command.
			if (pDestinationCell)
			{
				if (std::find(checkedCells.begin(), checkedCells.end(), pDestinationCell) == checkedCells.end())
					checkedCells.push_back(pDestinationCell);

				if (std::find(optionalCells.begin(), optionalCells.end(), pDestinationCell) != optionalCells.end())
				{
					optionalCells.erase(std::remove(optionalCells.begin(), optionalCells.end(), pDestinationCell), optionalCells.end());
					auto searchCell = pDestinationCell->MapCoords - CellStruct { 1, 1 };

					for (int i = 0; i < 4; ++i)
					{
						for (int j = 0; j < 2; ++j)
						{
							if (const auto pSearchCell = MapClass::Instance->TryGetCellAt(searchCell))
							{
								if (std::find(checkedCells.begin(), checkedCells.end(), pSearchCell) == checkedCells.end()
									&& std::find(optionalCells.begin(), optionalCells.end(), pSearchCell) == optionalCells.end()
									&& !pSearchCell->GetBuilding()
									&& pSearchCell->IsClearToMove(SpeedType::Amphibious, true, true, -1, MovementZone::Amphibious, -1, false))
								{
									optionalCells.push_back(pSearchCell);
								}
							}

							if (i % 2)
								searchCell.Y += static_cast<short>((i / 2) ? -1 : 1);
							else
								searchCell.X += static_cast<short>((i / 2) ? -1 : 1);
						}
					}
				}

				const auto thisOrder = TechnoWithDestination { pCheckedTechno, pDestinationCell };
				finalOrder.push_back(thisOrder);
			}
			else // Can not build
			{
				return true;
			}
		}

		checkedTechnos.clear();
	}
	while (reCheckedTechnos.size());

	// Step 5: Confirm command execution.
	for (const auto& pThisOrder : finalOrder)
	{
		const auto pCheckedTechno = pThisOrder.techno;
		const auto pDestinationCell = pThisOrder.destination;
		const auto absType = pCheckedTechno->WhatAmI();

		if (absType == AbstractType::Infantry)
		{
			const auto pInfantry = static_cast<InfantryClass*>(pCheckedTechno);

			if (pInfantry->IsDeployed())
				pInfantry->PlayAnim(Sequence::Undeploy, true);

			pInfantry->SetDestination(pDestinationCell, true);
		}
		else if (absType == AbstractType::Unit)
		{
			const auto pUnit = static_cast<UnitClass*>(pCheckedTechno);

			if (pUnit->Deployed && !(pUnit->Deploying || pUnit->Undeploying))
				pUnit->QueueMission(Mission::Unload, false);

			pUnit->SetDestination(pDestinationCell, true);
		}
	}

	return false;
}

void BuildingTypeExt::DrawAdjacentLines()
{
	const auto pType = abstract_cast<BuildingTypeClass*>(DisplayClass::Instance->CurrentBuildingType);

	if (!pType)
		return;

	const auto adjacent = static_cast<short>(pType->Adjacent + 1);

	if (adjacent <= 0)
		return;

	const auto foundation = CellStruct { pType->GetFoundationWidth(), pType->GetFoundationHeight(false) };

	if (foundation == CellStruct::Empty)
		return;

	const auto topLeft = DisplayClass::Instance->CurrentFoundation_CenterCell + DisplayClass::Instance->CurrentFoundation_TopLeftOffset;
	const auto min = CellStruct { static_cast<short>(topLeft.X - adjacent), static_cast<short>(topLeft.Y - adjacent) };
	const auto max = CellStruct { static_cast<short>(topLeft.X + foundation.X + adjacent - 1), static_cast<short>(topLeft.Y + foundation.Y + adjacent - 1) };

	auto rect = DSurface::Temp->GetRect();
	rect.Height -= 32;

	if (const auto pCell = MapClass::Instance->TryGetCellAt(min))
	{
		auto point = TacticalClass::Instance->CoordsToClient(CellClass::Cell2Coord(pCell->MapCoords, (1 + pCell->GetFloorHeight(Point2D::Empty)))).first;
		point.Y -= 1;
		auto nextPoint = point;

		point.Y -= 14;
		nextPoint.X += 29;
		DSurface::Temp->DrawLineEx(&rect, &point, &nextPoint, COLOR_WHITE);

		point.X -= 1;
		nextPoint.X -= 59;
		DSurface::Temp->DrawLineEx(&rect, &point, &nextPoint, COLOR_WHITE);
	}

	if (const auto pCell = MapClass::Instance->TryGetCellAt(CellStruct{ min.X, max.Y }))
	{
		auto point = TacticalClass::Instance->CoordsToClient(CellClass::Cell2Coord(pCell->MapCoords, (1 + pCell->GetFloorHeight(Point2D::Empty)))).first;
		point.X -= 1;
		auto nextPoint = point;

		point.X -= 29;
		nextPoint.Y += 14;
		DSurface::Temp->DrawLineEx(&rect, &point, &nextPoint, COLOR_WHITE);

		point.Y -= 1;
		nextPoint.Y -= 29;
		DSurface::Temp->DrawLineEx(&rect, &point, &nextPoint, COLOR_WHITE);
	}

	if (const auto pCell = MapClass::Instance->TryGetCellAt(max))
	{
		auto point = TacticalClass::Instance->CoordsToClient(CellClass::Cell2Coord(pCell->MapCoords, (1 + pCell->GetFloorHeight(Point2D::Empty)))).first;
		auto nextPoint = point;

		point.Y += 14;
		nextPoint.X += 29;
		DSurface::Temp->DrawLineEx(&rect, &point, &nextPoint, COLOR_WHITE);

		point.X -= 1;
		nextPoint.X -= 59;
		DSurface::Temp->DrawLineEx(&rect, &point, &nextPoint, COLOR_WHITE);
	}

	if (const auto pCell = MapClass::Instance->TryGetCellAt(CellStruct{ max.X, min.Y }))
	{
		auto point = TacticalClass::Instance->CoordsToClient(CellClass::Cell2Coord(pCell->MapCoords, (1 + pCell->GetFloorHeight(Point2D::Empty)))).first;
		auto nextPoint = point;

		point.X += 29;
		nextPoint.Y += 14;
		DSurface::Temp->DrawLineEx(&rect, &point, &nextPoint, COLOR_WHITE);

		point.Y -= 1;
		nextPoint.Y -= 29;
		DSurface::Temp->DrawLineEx(&rect, &point, &nextPoint, COLOR_WHITE);
	}
}

bool BuildingTypeExt::IsSameBuildingType(BuildingTypeClass* pType1, BuildingTypeClass* pType2)
{
	return ((pType1->BuildCat != BuildCat::Combat) == (pType2->BuildCat != BuildCat::Combat));
}

CellStruct BuildingTypeExt::SimulatePlacingAction(BuildingTypeClass* pType, CellStruct rallyCell, HouseClass* pHouse)
{
	auto startCell = CellStruct::Empty;
	auto extraOffset = 0;
	{
		auto distanceSquared = INT_MAX;
		{
			const auto& vecBlds = pHouse->Buildings;

			if (vecBlds.Count > 0)
			{
				for (const auto& pBuilding : vecBlds)
				{
					const auto pBaseType = pBuilding->Type;

					if (pBaseType->BaseNormal)
					{
						const auto mapCell = pBuilding->GetMapCoords();
						const auto newDistanceSquared = static_cast<int>(mapCell.DistanceFromSquared(rallyCell));

						if (newDistanceSquared < distanceSquared)
						{
							startCell = mapCell;
							extraOffset = Math::max(pBaseType->GetFoundationWidth() / 2, pBaseType->GetFoundationHeight(true) / 2);
							distanceSquared = newDistanceSquared;
						}
					}
				}
			}
		}

		if (RulesExt::Global()->CheckExtraBaseNormal)
		{
			const auto& vecUnits = ScenarioExt::Global()->BaseNormalTechnos;

			if (!vecUnits.empty())
			{
				for (const auto& pUnitExt : vecUnits)
				{
					const auto pBase = pUnitExt->OwnerObject();

					if (pHouse == pBase->Owner)
					{
						const auto mapCell = pBase->GetMapCoords();
						const auto newDistanceSquared = static_cast<int>(mapCell.DistanceFromSquared(rallyCell));

						if (newDistanceSquared < distanceSquared)
						{
							startCell = mapCell;
							extraOffset = 0;
							distanceSquared = newDistanceSquared;
						}
					}
				}
			}
		}
	}

	if (startCell == CellStruct::Empty)
		return CellStruct::Empty;

	const auto width = static_cast<short>(pType->GetFoundationWidth() / 2);
	const auto height = static_cast<short>(pType->GetFoundationHeight(true) / 2);
	const auto topLeftOffset = CellStruct { width, height };
	const auto difference = rallyCell - startCell;
	auto cell = startCell - topLeftOffset;

	if (difference != CellStruct::Empty)
		cell += difference * Math::min((pType->Adjacent + Math::max(width, height) + extraOffset + 1) / Math::max(std::abs(difference.X), std::abs(difference.Y)), 1.0);

	auto buildGap = static_cast<short>(BuildingTypeExt::ExtMap.Find(pType)->AutoBuilding_Gap.Get());

	if (pType->ProtectWithWall)
		++buildGap;

	if (pType->Adjacent < buildGap)
		return CellStruct::Empty;

	return BuildingTypeExt::NearbyPlacingLocation(pType, cell, pHouse, buildGap, true, true);
}

// The function that requires synchronization prohibits checking *shroud*
CellStruct BuildingTypeExt::NearbyPlacingLocation(BuildingTypeClass* pType, CellStruct cell, HouseClass* pHouse, int buildGap, bool checkAdjacent, bool checkShroud)
{
	std::unordered_map<int, int> checkedCells;
	checkedCells.reserve(53);

	// Basic
	auto canExistHere = [&](CellStruct currentCell)
	{
		if (pType->PlaceAnywhere)
			return true;

		for (auto pFoundation = pType->GetFoundationData(true); *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
		{
			const auto checkCell = currentCell + *pFoundation;
			const auto cellIndex = MapClass::GetCellIndex(checkCell);
			const auto flag = checkedCells[cellIndex];

			if (flag & 0xB0)
				return false;
			else if (flag & 0x1)
				continue;

			if (const auto pCell = MapClass::Instance->TryGetCellAt(checkCell))
			{
				if (pCell->CanThisExistHere(pType->SpeedType, pType, pHouse))
				{
					checkedCells[cellIndex] |= 0x1;
					continue;
				}
			}

			checkedCells[cellIndex] |= 0x10;
			return false;
		}

		return true;
	};

	// Adjacent 0x4A8EB0
	const auto width = pType->GetFoundationWidth();
	const auto height = pType->GetFoundationHeight(true);
	const auto pTypeExt = BuildingTypeExt::ExtMap.Find(pType);

	if (RulesExt::Global()->CheckExtraBaseNormal)
	{
		const auto& baseNormalTechnos = ScenarioExt::Global()->BaseNormalTechnos;

		if (baseNormalTechnos.size())
		{
			for (const auto& pTechnoExt : baseNormalTechnos)
			{
				const auto pTechno = pTechnoExt->OwnerObject();

				if (!TechnoExt::IsActive(pTechno))
					continue;

				const auto pTechnoTypeExt = pTechnoExt->TypeExtData;
				auto canBeBaseNormal = [&]()
				{
					const auto pOwner = pTechno->Owner;

					if (pOwner == pHouse)
						return pTechnoTypeExt->ExtraBaseNormal.Get();
					else if (RulesClass::Instance->BuildOffAlly && pOwner->IsAlliedWith(pHouse))
						return pTechnoTypeExt->ExtraBaseForAllyBuilding.Get();

					return false;
				};

				if (!canBeBaseNormal())
					continue;

				const auto& pExtraAllowed = pTypeExt->Adjacent_AllowedExtra;

				if (pExtraAllowed.size() > 0 && !pExtraAllowed.Contains(pTechnoTypeExt->OwnerObject()))
					continue;

				const auto& pExtraDisallowed = pTypeExt->Adjacent_DisallowedExtra;

				if (pExtraDisallowed.size() > 0 && pExtraDisallowed.Contains(pTechnoTypeExt->OwnerObject()))
					continue;

				checkedCells[MapClass::GetCellIndex(pTechno->GetMapCoords())] |= 0x4;
			}
		}
	}

	const auto range = pType->Adjacent + 1;

	auto canBuildHere = [&](CellStruct currentCell)
	{
		const auto maxX = currentCell.X + range + width;
		const auto maxY = currentCell.Y + range + height;
		const auto minX = currentCell.X - range;
		const auto minY = currentCell.Y - range;

		for (int x = minX; x < maxX; ++x)
		{
			for (int y = minY; y < maxY; ++y)
			{
				const auto checkCell = CellStruct { static_cast<short>(x), static_cast<short>(y) };
				const auto cellIndex = MapClass::GetCellIndex(checkCell);
				const auto flag = checkedCells[cellIndex];

				if (flag & 0x4)
					return true;
				else if (flag & 0x40)
					continue;

				if (const auto pCell = MapClass::Instance->TryGetCellAt(checkCell))
				{
					if (const auto pCellBuilding = pCell->GetBuilding())
					{
						auto canBeBaseNormal = [&]()
						{
							const auto pOwner = pCellBuilding->Owner;

							if (pOwner == pHouse)
								return pCellBuilding->Type->BaseNormal;
							else if (RulesClass::Instance->BuildOffAlly && pOwner->IsAlliedWith(pHouse))
								return pCellBuilding->Type->EligibileForAllyBuilding;

							return false;
						};

						if (canBeBaseNormal() && (!BuildingTypeExt::ExtMap.Find(pCellBuilding->Type)->NoBuildAreaOnBuildup || pCellBuilding->CurrentMission != Mission::Construction))
						{
							auto const& pBuildingsAllowed = pTypeExt->Adjacent_Allowed;

							if (pBuildingsAllowed.empty() || pBuildingsAllowed.Contains(pCellBuilding->Type))
							{
								auto const& pBuildingsDisallowed = pTypeExt->Adjacent_Disallowed;

								if (pBuildingsDisallowed.empty() || !pBuildingsDisallowed.Contains(pCellBuilding->Type))
								{
									checkedCells[cellIndex] |= 0x6;
									return true;
								}
							}
						}

						checkedCells[cellIndex] |= 0x20;
					}
				}

				checkedCells[cellIndex] |= 0x40;
			}
		}

		return false;
	};

	// Gap
	auto canSplitHere = [&](CellStruct currentCell)
	{
		const auto maxX = currentCell.X + buildGap + width;
		const auto maxY = currentCell.Y + buildGap + height;
		const auto minX = currentCell.X - buildGap;
		const auto minY = currentCell.Y - buildGap;

		for (int x = minX; x < maxX; ++x)
		{
			for (int y = minY; y < maxY; ++y)
			{
				const auto checkCell = CellStruct { static_cast<short>(x), static_cast<short>(y) };
				const auto cellIndex = MapClass::GetCellIndex(checkCell);
				const auto flag = checkedCells[cellIndex];

				if (flag & 0xB0)
					return false;
				else if (flag & 0x2)
					continue;

				if (const auto pCell = MapClass::Instance->TryGetCellAt(checkCell))
				{
					if (pCell->GetBuilding())
					{
						checkedCells[cellIndex] |= 0x20;
						return false;
					}
				}

				checkedCells[cellIndex] |= 0x2;
			}
		}

		return true;
	};

	// Shroud 0x4A9070
	auto canPlaceHere = [&](CellStruct currentCell)
	{
		for (auto pFoundation = pType->GetFoundationData(true); *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
		{
			const auto checkCell = currentCell + *pFoundation;
			const auto cellIndex = MapClass::GetCellIndex(checkCell);
			const auto flag = checkedCells[cellIndex];

			if (flag & 0xB0)
				return false;
			else if (flag & 0x8)
				continue;

			auto coords = CellClass::Cell2Coord(checkCell);
			coords.Z = MapClass::Instance->GetCellFloorHeight(coords);

			if (MapClass::Instance->IsLocationShrouded(coords))
			{
				checkedCells[cellIndex] |= 0x80;
				return false;
			}

			checkedCells[cellIndex] |= 0x8;
		}

		return true;
	};

	auto isValidCellToPlace = [&](CellStruct currentCell)
	{
		return canExistHere(currentCell) && (!checkAdjacent || canBuildHere(currentCell))
			&& (buildGap <= 0 || canSplitHere(currentCell)) && (!checkShroud || canPlaceHere(currentCell));
	};

    for (int n = 1; n <= 16; ++n) // r = 16
	{
        int x, y;

        // Right side -> downward
        x = cell.X + n;

        for (y = cell.Y - n; y <= cell.Y + n - 1; ++y)
		{
			const CellStruct currentCell { static_cast<short>(x), static_cast<short>(y) };

			if (isValidCellToPlace(currentCell))
				return currentCell;
		}

        // Down side -> leftward
        y = cell.Y + n;

        for (x = cell.X + n; x >= cell.X - n + 1; --x)
		{
			const CellStruct currentCell { static_cast<short>(x), static_cast<short>(y) };

			if (isValidCellToPlace(currentCell))
				return currentCell;
		}

        // Left side -> upward
        x = cell.X - n;

        for (y = cell.Y + n; y >= cell.Y - n + 1; --y)
		{
			const CellStruct currentCell { static_cast<short>(x), static_cast<short>(y) };

			if (isValidCellToPlace(currentCell))
				return currentCell;
		}

        // Up side -> rightward
        y = cell.Y - n;

        for (x = cell.X - n; x <= cell.X + n - 1; ++x)
		{
			const CellStruct currentCell { static_cast<short>(x), static_cast<short>(y) };

			if (isValidCellToPlace(currentCell))
				return currentCell;
		}
    }

	return CellStruct::Empty;
}

bool BuildingTypeExt::AutoPlaceBuilding(BuildingClass* pBuilding)
{
	const auto pType = pBuilding->Type;
	const auto isDefense = pType->BuildCat == BuildCat::Combat;

	if (isDefense ? !Phobos::Config::AutomaticPlacingCombatBuilding : !Phobos::Config::AutomaticPlacingBuilding)
		return false;

	const auto pTypeExt = BuildingTypeExt::ExtMap.Find(pType);

	if (!pTypeExt->AutoBuilding.Get(RulesExt::Global()->AutoBuilding) || pType->LaserFence || pType->Gate || pType->ToTile)
		return false;

	const auto pHouse = pBuilding->Owner;

	if (pHouse->Buildings.Count <= 0)
		return false;

	const auto pHouseExt = HouseExt::ExtMap.Find(pHouse);

	auto getMapCell = [&pHouseExt](BuildingClass* pBuilding)
	{
		if (!pBuilding->IsAlive || pBuilding->Health <= 0 || !pBuilding->IsOnMap || pBuilding->InLimbo || pHouseExt->OwnsLimboDeliveredBuilding(pBuilding))
			return CellStruct::Empty;

		return pBuilding->GetMapCoords();
	};

	auto addPlaceEvent = [&pType, &pHouse](CellStruct cell)
	{
		const EventClass event (pHouse->ArrayIndex, EventType::Place, AbstractType::Building, pType->GetArrayIndex(), pType->Naval, cell);
		EventClass::AddEvent(event);
	};

	if (pType->LaserFencePost || pType->Wall)
	{
		for (const auto& pOwned : pHouse->Buildings)
		{
			const auto pOwnedType = pOwned->Type;

			if (!pOwnedType->ProtectWithWall)
				continue;

			const auto baseCell = getMapCell(pOwned);

			if (baseCell == CellStruct::Empty)
				continue;

			const auto width = pOwnedType->GetFoundationWidth();
			const auto height = pOwnedType->GetFoundationHeight(true);
			auto cell = CellStruct::Empty;
			int index = 0, check = width + 1, count = 0;

			for (auto pFoundation = pOwnedType->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
			{
				if (++index != check)
					continue;

				check += (++count & 1) ? 1 : (height * 2 + width + 1);
				const auto outsideCell = baseCell + *pFoundation;
				const auto pCell = MapClass::Instance->TryGetCellAt(outsideCell);

				if (pCell && pCell->CanThisExistHere(pOwnedType->SpeedType, pOwnedType, pHouse))
				{
					addPlaceEvent(outsideCell);
					return true;
				}
			}

			for (auto pFoundation = pOwnedType->FoundationOutside; *pFoundation != CellStruct { 0x7FFF, 0x7FFF }; ++pFoundation)
			{
				const auto outsideCell = baseCell + *pFoundation;
				const auto pCell = MapClass::Instance->TryGetCellAt(outsideCell);

				if (pCell && pCell->CanThisExistHere(pOwnedType->SpeedType, pOwnedType, pHouse))
					cell = outsideCell;
			}

			if (cell == CellStruct::Empty)
				continue;

			addPlaceEvent(cell);
			return true;
		}

		return false;
	}
	else if (pType->PowersUpBuilding[0])
	{
		for (const auto& pOwned : pHouse->Buildings)
		{
			if (!reinterpret_cast<bool(__thiscall*)(BuildingClass*, BuildingTypeClass*, HouseClass*)>(0x452670)(pOwned, pType, pHouse)) // CanUpgradeBuilding
				continue;

			const auto cell = getMapCell(pOwned);

			if (cell == CellStruct::Empty || pOwned->CurrentMission == Mission::Selling)
				continue;

			addPlaceEvent(cell);
			return true;
		}

		return false;
	}

	if (pHouse->ConYards.Count > 0)
	{
		auto tryBuildAt = [&pType, &pHouse, &addPlaceEvent](CellStruct baseCell)
		{
			if (baseCell == CellStruct::Empty)
				return false;

			const auto placeCell = BuildingTypeExt::SimulatePlacingAction(pType, baseCell, pHouse);

			if (placeCell == CellStruct::Empty)
				return false;

			addPlaceEvent(placeCell);
			return true;
		};

		std::vector<CellStruct> rallyCells;
		rallyCells.reserve(pHouse->ConYards.Count);
		CellStruct primaryCell = CellStruct::Empty;

		for (auto pConYard : pHouse->ConYards)
		{
			auto pArchiveTarget = isDefense && BuildingTypeExt::ExtMap.Find(pConYard->Type)->HasSecondaryRallyPoint
				? BuildingExt::ExtMap.Find(pConYard)->SecondaryArchiveTarget : pConYard->ArchiveTarget;

			if (!pArchiveTarget)
				pArchiveTarget = pConYard;

			auto rallyCell = CellClass::Coord2Cell(pArchiveTarget->GetCoords());

			if (rallyCell == CellStruct::Empty)
				continue;

			rallyCells.push_back(rallyCell);

			if (pConYard->IsPrimaryFactory)
				primaryCell = rallyCell;
		}

		if (tryBuildAt(primaryCell))
			return true;

		for (auto rallyCell : rallyCells)
		{
			if (tryBuildAt(rallyCell))
				return true;
		}
	}

	return false;
}

bool BuildingTypeExt::BuildLimboBuilding(BuildingClass* pBuilding)
{
	const auto pBuildingType = pBuilding->Type;

	if (BuildingTypeExt::ExtMap.Find(pBuildingType)->LimboBuild)
	{
		const EventClass event
		(
			pBuilding->Owner->ArrayIndex,
			EventType::Place,
			AbstractType::Building,
			pBuildingType->GetArrayIndex(),
			pBuildingType->Naval,
			CellStruct { 1, 1 }
		);
		EventClass::AddEvent(event);

		return true;
	}

	return false;
}

void BuildingTypeExt::CreateLimboBuilding(BuildingClass* pBuilding, BuildingTypeClass* pType, HouseClass* pOwner, int ID)
{
	if (pBuilding || (pBuilding = static_cast<BuildingClass*>(pType->CreateObject(pOwner)), pBuilding))
	{
		// All of these are mandatory
		pBuilding->InLimbo = false;
		pBuilding->IsAlive = true;
		pBuilding->IsOnMap = true;

		// For reasons beyond my comprehension, the discovery logic is checked for certain logics like power drain/output in campaign only.
		// Normally on unlimbo the buildings are revealed to current player if unshrouded or if game is a campaign and to non-player houses always.
		// Because of the unique nature of LimboDelivered buildings, this has been adjusted to always reveal to the current player in singleplayer
		// and to the owner of the building regardless, removing the shroud check from the equation since they don't physically exist - Starkku
		if (SessionClass::IsCampaign())
			pBuilding->DiscoveredBy(HouseClass::CurrentPlayer);

		pBuilding->DiscoveredBy(pOwner);

		pOwner->RegisterGain(pBuilding, false);
		pOwner->UpdatePower();
		pOwner->RecheckTechTree = true;
		pOwner->RecheckPower = true;
		pOwner->RecheckRadar = true;
		pOwner->Buildings.AddItem(pBuilding);

		// Different types of building logics
		if (pType->ConstructionYard)
			pOwner->ConYards.AddItem(pBuilding); // why would you do that????

		if (pType->SecretLab)
			pOwner->SecretLabs.AddItem(pBuilding);

		auto const pBuildingExt = BuildingExt::ExtMap.Find(pBuilding);
		auto const pOwnerExt = HouseExt::ExtMap.Find(pOwner);

		if (pType->FactoryPlant)
		{
			if (pBuildingExt->TypeExtData->FactoryPlant_AllowTypes.size() > 0 || pBuildingExt->TypeExtData->FactoryPlant_DisallowTypes.size() > 0)
			{
				pOwnerExt->RestrictedFactoryPlants.push_back(pBuilding);
			}
			else
			{
				pOwner->FactoryPlants.AddItem(pBuilding);
				pOwner->CalculateCostMultipliers();
			}
		}

		// BuildingClass::Place is already called in DiscoveredBy
		// it added OrePurifier and xxxGainSelfHeal to House counter already

		// LimboKill ID
		pBuildingExt->LimboID = ID;

		// Add building to list of owned limbo buildings
		pOwnerExt->OwnedLimboDeliveredBuildings.push_back(pBuilding);

		if (!pBuilding->Type->Insignificant && !pBuilding->Type->DontScore)
			pOwnerExt->AddToLimboTracking(pBuilding->Type);

		auto const pTechnoExt = TechnoExt::ExtMap.Find(pBuilding);
		auto const pTechnoTypeExt = pTechnoExt->TypeExtData;

		if (pTechnoTypeExt->AutoDeath_Behavior.isset())
		{
			ScenarioExt::Global()->AutoDeathObjects.push_back(pTechnoExt);

			if (pTechnoTypeExt->AutoDeath_AfterDelay > 0)
				pTechnoExt->AutoDeathTimer.Start(pTechnoTypeExt->AutoDeath_AfterDelay);
		}
	}
}

bool BuildingTypeExt::DeleteLimboBuilding(BuildingClass* pBuilding, int ID)
{
	const auto pBuildingExt = BuildingExt::ExtMap.Find(pBuilding);

	if (pBuildingExt->LimboID != ID)
		return false;

	if (pBuildingExt->TypeExtData->LimboBuildID == ID)
	{
		const auto pHouse = pBuilding->Owner;
		const auto index = pBuilding->Type->ArrayIndex;

		for (auto& pBaseNode : pHouse->Base.BaseNodes)
		{
			if (pBaseNode.BuildingTypeIndex == index)
				pBaseNode.Placed = false;
		}
	}

	return true;
}

void BuildingTypeExt::ExtData::Initialize()
{ }

// =============================
// load / save

void BuildingTypeExt::ExtData::LoadFromINIFile(CCINIClass* const pINI)
{
	auto pThis = this->OwnerObject();
	const char* pSection = pThis->ID;
	const char* pArtSection = pThis->ImageFile;
	auto pArtINI = &CCINIClass::INI_Art();

	if (!pINI->GetSection(pSection))
		return;

	INI_EX exINI(pINI);
	INI_EX exArtINI(pArtINI);

	this->PowersUp_Owner.Read(exINI, pSection, "PowersUp.Owner");
	this->PowersUp_Buildings.Read(exINI, pSection, "PowersUp.Buildings");
	this->PowerPlantEnhancer_Buildings.Read(exINI, pSection, "PowerPlantEnhancer.PowerPlants");
	this->PowerPlantEnhancer_Amount.Read(exINI, pSection, "PowerPlantEnhancer.Amount");
	this->PowerPlantEnhancer_Factor.Read(exINI, pSection, "PowerPlantEnhancer.Factor");
	this->Powered_KillSpawns.Read(exINI, pSection, "Powered.KillSpawns");

	if (pThis->PowersUpBuilding[0] == NULL && this->PowersUp_Buildings.size() > 0)
		strcpy_s(pThis->PowersUpBuilding, this->PowersUp_Buildings[0]->ID);

	this->AllowAirstrike.Read(exINI, pSection, "AllowAirstrike");
	this->CanC4_AllowZeroDamage.Read(exINI, pSection, "CanC4.AllowZeroDamage");

	this->InitialStrength_Cloning.Read(exINI, pSection, "InitialStrength.Cloning");
	this->ExcludeFromMultipleFactoryBonus.Read(exINI, pSection, "ExcludeFromMultipleFactoryBonus");

	this->Grinding_AllowAllies.Read(exINI, pSection, "Grinding.AllowAllies");
	this->Grinding_AllowOwner.Read(exINI, pSection, "Grinding.AllowOwner");
	this->Grinding_AllowTypes.Read(exINI, pSection, "Grinding.AllowTypes");
	this->Grinding_DisallowTypes.Read(exINI, pSection, "Grinding.DisallowTypes");
	this->Grinding_Sound.Read(exINI, pSection, "Grinding.Sound");
	this->Grinding_PlayDieSound.Read(exINI, pSection, "Grinding.PlayDieSound");
	this->Grinding_Weapon.Read<true>(exINI, pSection, "Grinding.Weapon");
	this->Grinding_Weapon_RequiredCredits.Read(exINI, pSection, "Grinding.Weapon.RequiredCredits");

	this->DisplayIncome.Read(exINI, pSection, "DisplayIncome");
	this->DisplayIncome_Houses.Read(exINI, pSection, "DisplayIncome.Houses");
	this->DisplayIncome_Offset.Read(exINI, pSection, "DisplayIncome.Offset");

	this->ConsideredVehicle.Read(exINI, pSection, "ConsideredVehicle");
	this->SellBuildupLength.Read(exINI, pSection, "SellBuildupLength");
	this->IsDestroyableObstacle.Read(exINI, pSection, "IsDestroyableObstacle");

	this->JustHasRallyPoint.Read(exINI, pSection, "JustHasRallyPoint");
	this->JumpjetExitCoord.Read(exINI, pSection, "JumpjetExitCoord");
	this->RallySpeedType.Read(exINI, pSection, "RallySpeedType");
	this->RallyMovementZone.Read(exINI,pSection,"RallyMovementZone");

	this->Cameo_ShouldCount.Read(exINI, pSection, "Cameo.ShouldCount");
	this->AutoBuilding.Read(exINI, pSection, "AutoBuilding");
	this->AutoBuilding_Gap.Read(exINI, pSection, "AutoBuilding.Gap");
	this->LimboBuild.Read(exINI, pSection, "LimboBuild");
	this->LimboBuildID.Read(exINI, pSection, "LimboBuildID");
	this->LaserFencePost_Fence.Read(exINI, pSection, "LaserFencePost.Fence");
	this->PlaceBuilding_OnLand.Read(exINI, pSection, "PlaceBuilding.OnLand");
	this->PlaceBuilding_OnWater.Read(exINI, pSection, "PlaceBuilding.OnWater");

	this->FactoryPlant_AllowTypes.Read(exINI, pSection, "FactoryPlant.AllowTypes");
	this->FactoryPlant_DisallowTypes.Read(exINI, pSection, "FactoryPlant.DisallowTypes");

	this->IsAnimDelayedBurst.Read(exArtINI, pSection, "IsAnimDelayedBurst");
	this->Units_RepairRate.Read(exINI, pSection, "Units.RepairRate");
	this->Units_RepairStep.Read(exINI, pSection, "Units.RepairStep");
	this->Units_RepairPercent.Read(exINI, pSection, "Units.RepairPercent");
	this->Units_UseRepairCost.Read(exINI, pSection, "Units.UseRepairCost");

	this->NoBuildAreaOnBuildup.Read(exINI, pSection, "NoBuildAreaOnBuildup");
	this->Adjacent_Allowed.Read(exINI, pSection, "Adjacent.Allowed");
	this->Adjacent_Disallowed.Read(exINI, pSection, "Adjacent.Disallowed");
	this->Adjacent_AllowedExtra.Read(exINI, pSection, "Adjacent.AllowedExtra");
	this->Adjacent_DisallowedExtra.Read(exINI, pSection, "Adjacent.DisallowedExtra");

	this->BarracksExitCell.Read(exINI, pSection, "BarracksExitCell");

	this->HasSecondaryRallyPoint.Read(exINI, pSection, "HasSecondaryRallyPoint");

	if (pThis->NumberOfDocks > 0)
	{
		this->AircraftDockingDirs.clear();
		this->AircraftDockingDirs.resize(pThis->NumberOfDocks);

		Nullable<DirType> nLandingDir;
		nLandingDir.Read(exINI, pSection, "AircraftDockingDir");

		if (nLandingDir.isset())
			this->AircraftDockingDirs[0] = nLandingDir.Get();

		for (int i = 0; i < pThis->NumberOfDocks; ++i)
		{
			char tempBuffer[32];
			_snprintf_s(tempBuffer, sizeof(tempBuffer), "AircraftDockingDir%d", i);
			nLandingDir.Read(exINI, pSection, tempBuffer);

			if (nLandingDir.isset())
				this->AircraftDockingDirs[i] = nLandingDir.Get();
		}
	}

	// Ares tag
	this->SpyEffect_Custom.Read(exINI, pSection, "SpyEffect.Custom");
	if (SuperWeaponTypeClass::Array->Count > 0)
	{
		this->SuperWeapons.Read(exINI, pSection, "SuperWeapons");

		this->SpyEffect_VictimSuperWeapon.Read(exINI, pSection, "SpyEffect.VictimSuperWeapon");
		this->SpyEffect_InfiltratorSuperWeapon.Read(exINI, pSection, "SpyEffect.InfiltratorSuperWeapon");
	}
	this->SpyEffect_RadarJamDuration.Read(exINI, pSection, "SpyEffect.RadarJamDuration");

	if (pThis->MaxNumberOccupants > 10)
	{
		char tempBuffer[32];
		this->OccupierMuzzleFlashes.clear();
		this->OccupierMuzzleFlashes.resize(pThis->MaxNumberOccupants);

		for (int i = 0; i < pThis->MaxNumberOccupants; ++i)
		{
			Nullable<Point2D> nMuzzleLocation;
			_snprintf_s(tempBuffer, sizeof(tempBuffer), "MuzzleFlash%d", i);
			nMuzzleLocation.Read(exArtINI, pArtSection, tempBuffer);
			this->OccupierMuzzleFlashes[i] = nMuzzleLocation.Get(Point2D::Empty);
		}
	}

	this->Refinery_UseStorage.Read(exINI, pSection, "Refinery.UseStorage");

	// PlacementPreview
	{
		this->PlacementPreview.Read(exINI, pSection, "PlacementPreview");
		this->PlacementPreview_Shape.Read(exINI, pSection, "PlacementPreview.Shape");
		this->PlacementPreview_ShapeFrame.Read(exINI, pSection, "PlacementPreview.ShapeFrame");
		this->PlacementPreview_Offset.Read(exINI, pSection, "PlacementPreview.Offset");
		this->PlacementPreview_Remap.Read(exINI, pSection, "PlacementPreview.Remap");
		this->PlacementPreview_Palette.LoadFromINI(pINI, pSection, "PlacementPreview.Palette");
		this->PlacementPreview_Translucency.Read(exINI, pSection, "PlacementPreview.Translucency");
	}

	// Art
	this->ZShapePointMove_OnBuildup.Read(exArtINI, pSection, "ZShapePointMove.OnBuildup");
}

void BuildingTypeExt::ExtData::CompleteInitialization()
{
	auto const pThis = this->OwnerObject();
	UNREFERENCED_PARAMETER(pThis);
}

template <typename T>
void BuildingTypeExt::ExtData::Serialize(T& Stm)
{
	Stm
		.Process(this->PowersUp_Owner)
		.Process(this->PowersUp_Buildings)
		.Process(this->PowerPlantEnhancer_Buildings)
		.Process(this->PowerPlantEnhancer_Amount)
		.Process(this->PowerPlantEnhancer_Factor)
		.Process(this->SuperWeapons)
		.Process(this->OccupierMuzzleFlashes)
		.Process(this->Powered_KillSpawns)
		.Process(this->AllowAirstrike)
		.Process(this->CanC4_AllowZeroDamage)
		.Process(this->InitialStrength_Cloning)
		.Process(this->ExcludeFromMultipleFactoryBonus)
		.Process(this->Refinery_UseStorage)
		.Process(this->Grinding_AllowAllies)
		.Process(this->Grinding_AllowOwner)
		.Process(this->Grinding_AllowTypes)
		.Process(this->Grinding_DisallowTypes)
		.Process(this->Grinding_Sound)
		.Process(this->Grinding_PlayDieSound)
		.Process(this->Grinding_Weapon)
		.Process(this->Grinding_Weapon_RequiredCredits)
		.Process(this->DisplayIncome)
		.Process(this->DisplayIncome_Houses)
		.Process(this->DisplayIncome_Offset)
		.Process(this->PlacementPreview)
		.Process(this->PlacementPreview_Shape)
		.Process(this->PlacementPreview_ShapeFrame)
		.Process(this->PlacementPreview_Offset)
		.Process(this->PlacementPreview_Remap)
		.Process(this->PlacementPreview_Palette)
		.Process(this->PlacementPreview_Translucency)
		.Process(this->SpyEffect_Custom)
		.Process(this->SpyEffect_VictimSuperWeapon)
		.Process(this->SpyEffect_InfiltratorSuperWeapon)
		.Process(this->SpyEffect_RadarJamDuration)
		.Process(this->ConsideredVehicle)
		.Process(this->ZShapePointMove_OnBuildup)
		.Process(this->SellBuildupLength)
		.Process(this->JustHasRallyPoint)
		.Process(this->JumpjetExitCoord)
		.Process(this->RallySpeedType)
		.Process(this->RallyMovementZone)
		.Process(this->Cameo_ShouldCount)
		.Process(this->AutoBuilding)
		.Process(this->AutoBuilding_Gap)
		.Process(this->LimboBuild)
		.Process(this->LimboBuildID)
		.Process(this->LaserFencePost_Fence)
		.Process(this->PlaceBuilding_OnLand)
		.Process(this->PlaceBuilding_OnWater)
		.Process(this->AircraftDockingDirs)
		.Process(this->FactoryPlant_AllowTypes)
		.Process(this->FactoryPlant_DisallowTypes)
		.Process(this->IsAnimDelayedBurst)
		.Process(this->IsDestroyableObstacle)
		.Process(this->Units_RepairRate)
		.Process(this->Units_RepairStep)
		.Process(this->Units_RepairPercent)
		.Process(this->Units_UseRepairCost)
		.Process(this->NoBuildAreaOnBuildup)
		.Process(this->Adjacent_Allowed)
		.Process(this->Adjacent_Disallowed)
		.Process(this->Adjacent_AllowedExtra)
		.Process(this->Adjacent_DisallowedExtra)
		.Process(this->BarracksExitCell)
		.Process(this->HasSecondaryRallyPoint)
		;
}

void BuildingTypeExt::ExtData::LoadFromStream(PhobosStreamReader& Stm)
{
	Extension<BuildingTypeClass>::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void BuildingTypeExt::ExtData::SaveToStream(PhobosStreamWriter& Stm)
{
	Extension<BuildingTypeClass>::SaveToStream(Stm);
	this->Serialize(Stm);
}

bool BuildingTypeExt::ExtContainer::Load(BuildingTypeClass* pThis, IStream* pStm)
{
	BuildingTypeExt::ExtData* pData = this->LoadKey(pThis, pStm);

	return pData != nullptr;
};

bool BuildingTypeExt::LoadGlobals(PhobosStreamReader& Stm)
{

	return Stm.Success();
}

bool BuildingTypeExt::SaveGlobals(PhobosStreamWriter& Stm)
{


	return Stm.Success();
}
// =============================
// container

BuildingTypeExt::ExtContainer::ExtContainer() : Container("BuildingTypeClass") { }

BuildingTypeExt::ExtContainer::~ExtContainer() = default;

// =============================
// container hooks

DEFINE_HOOK(0x45E50C, BuildingTypeClass_CTOR, 0x6)
{
	GET(BuildingTypeClass*, pItem, EAX);

	BuildingTypeExt::ExtMap.TryAllocate(pItem);

	return 0;
}

DEFINE_HOOK(0x45E707, BuildingTypeClass_DTOR, 0x6)
{
	GET(BuildingTypeClass*, pItem, ESI);

	BuildingTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK_AGAIN(0x465300, BuildingTypeClass_SaveLoad_Prefix, 0x5)
DEFINE_HOOK(0x465010, BuildingTypeClass_SaveLoad_Prefix, 0x5)
{
	GET_STACK(BuildingTypeClass*, pItem, 0x4);
	GET_STACK(IStream*, pStm, 0x8);

	BuildingTypeExt::ExtMap.PrepareStream(pItem, pStm);

	return 0;
}

DEFINE_HOOK(0x4652ED, BuildingTypeClass_Load_Suffix, 0x7)
{
	BuildingTypeExt::ExtMap.LoadStatic();
	return 0;
}

DEFINE_HOOK(0x46536A, BuildingTypeClass_Save_Suffix, 0x7)
{
	BuildingTypeExt::ExtMap.SaveStatic();
	return 0;
}

DEFINE_HOOK_AGAIN(0x464A56, BuildingTypeClass_LoadFromINI, 0xA)
DEFINE_HOOK(0x464A49, BuildingTypeClass_LoadFromINI, 0xA)
{
	GET(BuildingTypeClass*, pItem, EBP);
	GET_STACK(CCINIClass*, pINI, 0x364);

	BuildingTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}
