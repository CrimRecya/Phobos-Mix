#include "AssignRallyPoint.h"

#include "DisplayClass.h"
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
	return CATEGORY_DEVELOPMENT;
}

const wchar_t* AssignRallyPointCommandClass::GetUIDescription() const
{
	return L"Assign the rally point of selected buildings to the mouse pointing coords.";
}

void AssignRallyPointCommandClass::Execute(WWKey eInput) const
{
	if (!ObjectClass::CurrentObjects().Count)
		return;

	// Get current buildings.
	DynamicVectorClass<BuildingClass*> buildings;

	for (auto pCurrent : ObjectClass::CurrentObjects())
	{
		if (auto pBuilding = abstract_cast<BuildingClass*>(pCurrent))
		{
			if (pBuilding->Owner->IsControlledByCurrentPlayer() && pBuilding->IsUnitFactory())
				buildings.AddItem(pBuilding);
		}
	}

	if (!buildings.Count)
		return;

	// Get pointed object.
	Point2D mouseCrd;
	WWMouseClass::Instance->GetCoords(&mouseCrd);
	auto screenCrd = mouseCrd - Point2D({ DSurface::ViewBounds->X ,DSurface::ViewBounds->Y });
	CellStruct cellBuffer;
	CoordStruct coordBuffer;
	ObjectClass* pObjectBuffer;
	bool foggedBuffer;
	bool shroudedBuffer;
	DisplayClass::Instance->ProcessClickCoords(&screenCrd, &cellBuffer, &coordBuffer, &pObjectBuffer, (BYTE*)(&foggedBuffer), (BYTE*)(&shroudedBuffer));

	AbstractClass* pPointed = nullptr;

	if (pObjectBuffer)
	{
		pPointed = (AbstractClass*)pObjectBuffer;
	}
	else if (MapClass::Instance->IsWithinUsableArea(cellBuffer, false))
	{
		pPointed = (AbstractClass*)MapClass::Instance->GetCellAt(cellBuffer);
	}

	if (!pPointed)
		return;

	// Raise event to assign rally point.
	for (auto pBuilding : buildings)
	{
		TargetClass target1 = TargetClass(pBuilding);
		TargetClass target2 = TargetClass(pPointed);
		EventClass event = EventClass(pBuilding->GetOwningHouseIndex(), EventType::Archive, target1, target2);
		EventClass::AddEvent(event);
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
	return CATEGORY_DEVELOPMENT;
}

const wchar_t* AssignSecondaryRallyPointCommandClass::GetUIDescription() const
{
	return L"Assign the secondary rally point of selected buildings to the mouse pointing coords.";
}

void AssignSecondaryRallyPointCommandClass::Execute(WWKey eInput) const
{
	if (!ObjectClass::CurrentObjects().Count)
		return;

	// Get current buildings.
	DynamicVectorClass<BuildingClass*> buildings;

	for (auto pCurrent : ObjectClass::CurrentObjects())
	{
		if (auto pBuilding = abstract_cast<BuildingClass*>(pCurrent))
		{
			if (pBuilding->Owner->IsControlledByCurrentPlayer() && BuildingTypeExt::ExtMap.Find(pBuilding->Type)->HasSecondaryRallyPoint)
				buildings.AddItem(pBuilding);
		}
	}

	if (!buildings.Count)
		return;

	// Get pointed object.
	Point2D mouseCrd;
	WWMouseClass::Instance->GetCoords(&mouseCrd);
	auto screenCrd = mouseCrd - Point2D({ DSurface::ViewBounds->X ,DSurface::ViewBounds->Y });
	CellStruct cellBuffer;
	CoordStruct coordBuffer;
	ObjectClass* pObjectBuffer;
	bool foggedBuffer;
	bool shroudedBuffer;
	DisplayClass::Instance->ProcessClickCoords(&screenCrd, &cellBuffer, &coordBuffer, &pObjectBuffer, (BYTE*)(&foggedBuffer), (BYTE*)(&shroudedBuffer));

	AbstractClass* pPointed = nullptr;

	if (pObjectBuffer)
	{
		pPointed = (AbstractClass*)pObjectBuffer;
	}
	else if (MapClass::Instance->IsWithinUsableArea(cellBuffer, false))
	{
		pPointed = (AbstractClass*)MapClass::Instance->GetCellAt(cellBuffer);
	}

	if (!pPointed)
		return;

	// Raise event to assign rally point.
	for (auto pBuilding : buildings)
	{
		TargetClass target1 = TargetClass(pBuilding);
		TargetClass target2 = TargetClass(pPointed);

		// For some Ares reason, setting the type in the CTOR call will crash the game.
		// Thus we do it here manually.
		EventClass event = EventClass(pBuilding->GetOwningHouseIndex(), EventType::Archive, target1, target2);
		event.Type = (EventType)(EventTypeExt::AssignSecondaryRallyPoint);

		EventClass::AddEvent(event);
	}
}
