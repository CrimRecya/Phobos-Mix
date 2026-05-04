#include "CeaseFireStance.h"

#include "Ext/Techno/Body.h"
#include <Ext/Event/Body.h>

const char* CeaseFireStanceClass::GetName() const
{
	return "CeaseFireStance";
}

const wchar_t* CeaseFireStanceClass::GetUIName() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_CEASEFIRE_STANCE", L"Cease Fire Stance");
}

const wchar_t* CeaseFireStanceClass::GetUICategory() const
{
	return CATEGORY_CONTROL;
}

const wchar_t* CeaseFireStanceClass::GetUIDescription() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_CEASEFIRE_STANCE_DESC", L"Cease Fire Stance");
}

void CeaseFireStanceClass::Execute(WWKey eInput) const
{
	CeaseFireStanceClass::CeaseFireExecute();
}

void CeaseFireStanceClass::CeaseFireExecute()
{
	std::vector<TechnoClass*> TechnoVectorCeaseFire;
	std::vector<TechnoClass*> TechnoVectorNonCeaseFire;

	// Get current selected units.
	// If all selected units are at CeaseFire stance, we should cancel their CeaseFire stance.
	// Otherwise, we should turn them into CeaseFire stance.
	bool isAnySelectedUnitTogglable = false;
	bool isAllSelectedUnitCeaseFireStance = true;

	auto processATechno = [&](TechnoClass* pTechno)
	{
		const auto pTechnoExt = TechnoExt::ExtMap.Find(pTechno);

		// If not togglable then exclude it from the iteration.
		if (!pTechnoExt->CanToggleCeaseFireStance())
			return;

		isAnySelectedUnitTogglable = true;

		if (pTechnoExt->GetCeaseFireStance())
		{
			TechnoVectorCeaseFire.push_back(pTechno);
		}
		else
		{
			isAllSelectedUnitCeaseFireStance = false;
			TechnoVectorNonCeaseFire.push_back(pTechno);
		}
	};

	for (const auto& pUnit : ObjectClass::CurrentObjects)
	{
		// try to cast to TechnoClass
		TechnoClass* pTechno = abstract_cast<TechnoClass*>(pUnit);

		// if not a techno or is in berserk or is not controlled by the local player then ignore it
		if (!pTechno || pTechno->Berzerk || !pTechno->Owner->IsControlledByCurrentPlayer())
			continue;

		processATechno(pTechno);

		if (auto pPassenger = pTechno->Passengers.GetFirstPassenger())
		{
			for (; pPassenger; pPassenger = abstract_cast<FootClass*>(pPassenger->NextObject))
				processATechno(pPassenger);
		}

		if (auto pBuilding = abstract_cast<BuildingClass*>(pTechno))
		{
			for (auto pOccupier : pBuilding->Occupants)
				processATechno(pOccupier);
		}
	}

	// If this boolean is false, then none of the selected units are togglable, meaning this hotket doesn't need to do anything.
	if (isAnySelectedUnitTogglable)
	{
		// If all selected units are CeaseFire stance, then cancel their CeaseFire stance;
		// otherwise, make all selected units CeaseFire stance.
		if (isAllSelectedUnitCeaseFireStance)
		{
			for (const auto& pTechno : TechnoVectorCeaseFire)
				EventExt::RaiseToggleCeaseFireStance(pTechno);

			wchar_t buffer[0x100];
			swprintf_s(buffer, GeneralUtils::LoadStringUnlessMissing("MSG:CEASEFIRE_STANCE_OFF", L"%i unit(s) ceased Cease Fire Stance."), TechnoVectorCeaseFire.size());
			MessageListClass::Instance.PrintMessage(buffer);
		}
		else
		{
			int ceasedAggressiveStanceCount = 0;
			for (const auto& pTechno : TechnoVectorNonCeaseFire)
			{
				if (TechnoExt::ExtMap.Find(pTechno)->GetAggressiveStance())
				{
					ceasedAggressiveStanceCount++;
					EventExt::RaiseToggleAggressiveStance(pTechno);
				}

				EventExt::RaiseToggleCeaseFireStance(pTechno);
			}

			wchar_t buffer[0x100];
			if (ceasedAggressiveStanceCount != 0)
				swprintf_s(buffer, GeneralUtils::LoadStringUnlessMissing("MSG:CEASEFIRE_STANCE_ON_V2", L"%i unit(s) entered Cease Fire Stance, %i unit(s) ceased Aggressive Stance."), TechnoVectorNonCeaseFire.size(), ceasedAggressiveStanceCount);
			else
				swprintf_s(buffer, GeneralUtils::LoadStringUnlessMissing("MSG:CEASEFIRE_STANCE_ON", L"%i unit(s) entered Cease Fire Stance."), TechnoVectorNonCeaseFire.size());
			MessageListClass::Instance.PrintMessage(buffer);
		}
	}
}
