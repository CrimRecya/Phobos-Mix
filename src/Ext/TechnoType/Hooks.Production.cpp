#include "Body.h"

#include <AircraftClass.h>

#include <Ext/House/Body.h>
#include <Ext/Techno/Body.h>
#include <Utilities/AresHelper.h>

// Ares hooked all of the function away, so we can only hook the ending of the function.
DEFINE_HOOK(0x5F7A89, ObjectTypeClass_FindFactory_End, 0x5)
{
	// Different from Ares logic (stuck at 50%)
	if (!AresHelper::CanUseAres)
		return 0;

	GET(ObjectTypeClass* const, pObjectType, ECX);
	// GET_STACK(bool, allowOccupied, STACK_OFFSET(0, 0x4));
	GET_STACK(bool, requirePower, STACK_OFFSET(0, 0x8));
	GET_STACK(bool, requireCanBuild, STACK_OFFSET(0, 0xC));
	GET_STACK(HouseClass* const, pHouse, STACK_OFFSET(0, 0x10));

	if (const auto pAircraftType = abstract_cast<AircraftTypeClass*, true>(pObjectType))
	{
		if (TechnoTypeExt::ExtMap.Find(pAircraftType)->ThisIsAJumpjet)
		{
			BuildingClass* pBuildingResult = nullptr;
			const DWORD ownerHouse = pAircraftType->GetOwners();
			const int buildingCount = pHouse->Buildings.Count;

			for (int i = 0; i < buildingCount; ++i)
			{
				const auto pBuilding = pHouse->Buildings.Items[i];
				const auto pBuildingType = pBuilding->Type;

				if (pBuildingType->Factory == AbstractType::AircraftType
					&& !pBuildingType->WeaponsFactory
					&& (!requirePower || pBuilding->HasPower)
					&& pBuilding->CurrentMission != Mission::Selling
					&& pBuilding->QueuedMission != Mission::Selling
					&& !pBuilding->InLimbo
					&& (!requireCanBuild || pBuilding->Owner->CanBuild(pAircraftType, true, true) > CanBuildResult::Unbuildable)
					&& (pBuildingType->GetOwners() & ownerHouse) != 0)
				{
					pBuildingResult = pBuilding;

					if (pBuilding->IsPrimaryFactory)
						break;
				}
			}

			R->EAX<BuildingClass*>(pBuildingResult);
		}
	}

	return 0;
}

DEFINE_HOOK(0x443C71, BuildingClass_KickOutUnit_ThisIsAJumpjet, 0x6)
{
	GET(TechnoClass* const, pProduct, EDI);

	if (const auto pAircraft = abstract_cast<AircraftClass*>(pProduct))
	{
		if (const auto pJumpjetType = TechnoTypeExt::ExtMap.Find(pAircraft->Type)->ThisIsAJumpjet.Get())
		{
			if (pJumpjetType->Locomotor == LocomotionClass::CLSIDs::Jumpjet)
			{
				const auto pNewProduct = static_cast<UnitClass*>(pJumpjetType->CreateObject(pAircraft->Owner));
				TechnoExt::ExtMap.Find(pNewProduct)->JumpjetFromAirport = true;
				R->EDI<TechnoClass*>(pNewProduct);
				pAircraft->UnInit();
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x44409C, BuildingClass_KickOutUnit_ImAJumpjetFromAirport1, 0x6)
{
	GET(TechnoClass* const, pProduct, EDI);
	return TechnoExt::ExtMap.Find(pProduct)->JumpjetFromAirport ? 0x4445FB : 0;
}

DEFINE_HOOK(0x44498E, BuildingClass_KickOutUnit_ImAJumpjetFromAirport2, 0x6)
{
	GET(TechnoClass* const, pProduct, EDI);
	return TechnoExt::ExtMap.Find(pProduct)->JumpjetFromAirport ? 0x444638 : 0;
}
