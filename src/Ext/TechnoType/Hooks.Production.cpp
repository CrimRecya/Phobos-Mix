#include "Body.h"
#include <Ext/House/Body.h>

// Ares hooked all of the function away, so we can only hook the ending of the function.
DEFINE_HOOK(0x5F7A89, ObjectTypeClass_FindFactory_End, 0x5)
{
	GET(ObjectTypeClass* const, pObjectType, ECX);
	// GET_STACK(bool, allowOccupied, STACK_OFFSET(0, 0x4));
	GET_STACK(bool, requirePower, STACK_OFFSET(0, 0x8));
	GET_STACK(bool, requireCanBuild, STACK_OFFSET(0, 0xC));
	GET_STACK(HouseClass* const, pHouse, STACK_OFFSET(0, 0x10));

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find((TechnoTypeClass*)pObjectType);

	if (!pTypeExt || !pTypeExt->ThisIsAJumpjet)
		return 0;

	auto const pType = (TechnoTypeClass*)pObjectType;

	BuildingClass* pBuildingResult = nullptr;
	BuildingClass* pBuilding;
	unsigned int pOwnerHouse;

	pOwnerHouse = pType->GetOwners();
	int nBuildingCount = pHouse->Buildings.Count;

	if (nBuildingCount <= 0)
		return 0;

	for (int i = 0; i != nBuildingCount; ++i)
	{
		pBuilding = pHouse->Buildings.Items[i];

		if (!pBuilding->InLimbo
			&& pBuilding->Type->Factory == pType->WhatAmI()
			&& (!requirePower || pBuilding->HasPower)
			&& pBuilding->GetCurrentMission() != Mission::Selling
			&& pBuilding->QueuedMission != Mission::Selling
			&& (!requireCanBuild || (int)pBuilding->Owner->CanBuild(pType, true, true) > 0)
			&& (pBuilding->Type->GetOwners() & pOwnerHouse) != 0)
		{
			pBuildingResult = pBuilding;

			if (pBuilding->IsPrimaryFactory)
				break;
		}
	}

	R->EAX(pBuildingResult);
	return 0;
}

namespace KickOutJumpjetFromAirport
{
	bool Processing = false;
}

DEFINE_HOOK(0x443C71, BuildingClass_KickOutUnit_ThisIsAJumpjet, 0x6)
{
	GET(TechnoClass* const, pProduct, EDI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pProduct->GetTechnoType());

	if (pTypeExt && pTypeExt->ThisIsAJumpjet)
	{
		R->EDI(pTypeExt->ThisIsAJumpjet->CreateObject(pProduct->Owner));
		pProduct->UnInit();
		KickOutJumpjetFromAirport::Processing = true;
	}

	return 0;
}

DEFINE_HOOK(0x44409C, BuildingClass_KickOutUnit_ImAJumpjetFromAirport1, 0x6)
{
	GET(TechnoClass* const, pProduct, EDI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pProduct->GetTechnoType());

	if (pTypeExt && pTypeExt->ImAJumpjetFromAirport && KickOutJumpjetFromAirport::Processing)
	{
		KickOutJumpjetFromAirport::Processing = false;
		return 0x444159;
	}

	return 0;
}

DEFINE_HOOK(0x4445FB, BuildingClass_KickOutUnit_ImAJumpjetFromAirport2, 0x6)
{
	GET(TechnoClass* const, pProduct, EDI);

	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pProduct->GetTechnoType());

	return (pTypeExt && pTypeExt->ImAJumpjetFromAirport) ? 0x444638 : 0;
}
