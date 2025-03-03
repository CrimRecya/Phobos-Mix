#include "AssignRallyPoint.h"

#include "MouseClass.h"
#include "WWMouseClass.h"
#include "MapClass.h"
#include "Surface.h"
#include "HouseClass.h"
#include "BuildingClass.h"
#include "EventClass.h"

#include "Ext/BuildingType/Body.h"
#include "Ext/Building/Body.h"
#include "Ext/Event/Body.h"

#include <Utilities/Debug.h>

const char* AssignRallyPointCommandClass::GetName() const
{
	return "AssignRallyPoint";
}

const wchar_t* AssignRallyPointCommandClass::GetUIName() const
{
	return L"Assign rally point";
}

const wchar_t* AssignRallyPointCommandClass::GetUICategory() const
{
	return CATEGORY_CONTROL;
}

const wchar_t* AssignRallyPointCommandClass::GetUIDescription() const
{
	return L"Assign the rally point of selected buildings to the mouse pointing coords.";
}

void AssignRallyPointCommandClass::Execute(WWKey eInput) const
{
	if (!ObjectClass::CurrentObjects->Count)
		return;

	// Get current buildings.
	std::vector<BuildingClass*> buildings;

	for (const auto& pCurrent : ObjectClass::CurrentObjects())
	{
		if (const auto& pBuilding = abstract_cast<BuildingClass*>(pCurrent))
		{
			if (pBuilding->Owner->IsControlledByCurrentPlayer())
				buildings.push_back(pBuilding);
		}
	}

	if (buildings.empty())
		return;

	// Get pointed object.
	auto point = WWMouseClass::Instance->XY1 - Point2D { DSurface::ViewBounds->X, DSurface::ViewBounds->Y };
	auto cell = CellStruct::Empty;
	auto coords = CoordStruct::Empty;
	ObjectClass* pObj = nullptr;
	BYTE fogged = 0;
	BYTE shrouded = 0;

	DisplayClass::Instance->ProcessClickCoords(&point, &cell, &coords, &pObj, &fogged, &shrouded);

	if (const auto pPointed = pObj ? static_cast<AbstractClass*>(pObj) : (MapClass::Instance->IsWithinUsableArea(cell, false) ? MapClass::Instance->TryGetCellAt(cell) : nullptr))
	{
		for (const auto& pBuilding : buildings)
			EventClass::AddEvent(EventClass(pBuilding->GetOwningHouseIndex(), EventType::Archive, TargetClass(pBuilding), TargetClass(pPointed)));
	}
}

const char* AssignSecondaryRallyPointCommandClass::GetName() const
{
	return "AssignSecondaryRallyPoint";
}

const wchar_t* AssignSecondaryRallyPointCommandClass::GetUIName() const
{
	return L"Assign secondary rally point";
}

const wchar_t* AssignSecondaryRallyPointCommandClass::GetUICategory() const
{
	return CATEGORY_CONTROL;
}

const wchar_t* AssignSecondaryRallyPointCommandClass::GetUIDescription() const
{
	return L"Assign the secondary rally point of selected buildings to the mouse pointing coords.";
}

void AssignSecondaryRallyPointCommandClass::Execute(WWKey eInput) const
{
	if (!ObjectClass::CurrentObjects->Count)
		return;

	// Get current buildings.
	std::vector<BuildingClass*> buildings;

	for (const auto& pCurrent : ObjectClass::CurrentObjects())
	{
		if (const auto& pBuilding = abstract_cast<BuildingClass*>(pCurrent))
		{
			if (pBuilding->Owner->IsControlledByCurrentPlayer() && BuildingTypeExt::ExtMap.Find(pBuilding->Type)->HasSecondaryRallyPoint)
				buildings.push_back(pBuilding);
		}
	}

	if (buildings.empty())
		return;

	// Get pointed object.
	auto point = WWMouseClass::Instance->XY1 - Point2D { DSurface::ViewBounds->X, DSurface::ViewBounds->Y };
	auto cell = CellStruct::Empty;
	auto coords = CoordStruct::Empty;
	ObjectClass* pObj = nullptr;
	BYTE fogged = 0;
	BYTE shrouded = 0;

	DisplayClass::Instance->ProcessClickCoords(&point, &cell, &coords, &pObj, &fogged, &shrouded);

	if (const auto pPointed = pObj ? static_cast<AbstractClass*>(pObj) : (MapClass::Instance->IsWithinUsableArea(cell, false) ? MapClass::Instance->TryGetCellAt(cell) : nullptr))
	{
		for (const auto& pBuilding : buildings)
			EventExt::RaiseAssignSecondaryRallyPoint(pBuilding, pPointed);
	}
}
