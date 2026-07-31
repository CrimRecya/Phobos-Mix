#include "ReversingStance.h"

#include "Ext/Unit/Body.h"
#include <Ext/Event/Body.h>
#include <Locomotion/AdvancedDriveLocomotionClass.h>

const char* ReversingStanceClass::GetName() const
{
	return "ReversingStance";
}

const wchar_t* ReversingStanceClass::GetUIName() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_REVERSING_STANCE", L"Reversing Stance");
}

const wchar_t* ReversingStanceClass::GetUICategory() const
{
	return CATEGORY_CONTROL;
}

const wchar_t* ReversingStanceClass::GetUIDescription() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_REVERSING_STANCE_DESC", L"Reversing Stance");
}

void ReversingStanceClass::Execute(WWKey eInput) const
{
	ReversingStanceClass::ReversingExecute();
}

void ReversingStanceClass::ReversingExecute()
{
	std::vector<UnitClass*> UnitVectorReversing;
	std::vector<UnitClass*> UnitVectorNonReversing;

	bool isAnySelectedUnitTogglable = false;
	bool isAllSelectedUnitNonReversingStance = true;

	for (const auto& pObject : ObjectClass::CurrentObjects)
	{
		const auto pUnit = abstract_cast<UnitClass*>(pObject);

		if (!pUnit || pUnit->Berzerk || !pUnit->Owner->IsControlledByCurrentPlayer() || !UnitTypeExt::Fetch(pUnit->Type)->AdvancedDrive_Reverse)
			continue;

		const auto pLoco = locomotion_cast<AdvancedDriveLocomotionClass*>(pUnit->Locomotor);

		if (!pLoco)
			return;

		isAnySelectedUnitTogglable = true;

		if (pLoco->IsForward)
		{
			UnitVectorNonReversing.push_back(pUnit);
		}
		else
		{
			isAllSelectedUnitNonReversingStance = false;
			UnitVectorReversing.push_back(pUnit);
		}
	}

	if (isAnySelectedUnitTogglable)
	{
		if (isAllSelectedUnitNonReversingStance)
		{
			for (const auto& pUnit : UnitVectorNonReversing)
				EventExt::RaiseToggleReversingStance(pUnit);

			wchar_t buffer[0x100];
			swprintf_s(buffer, GeneralUtils::LoadStringUnlessMissing("MSG:REVERSING_STANCE_ON", L"%i unit(s) entered Reversing Stance."), UnitVectorNonReversing.size());
			MessageListClass::Instance.PrintMessage(buffer);
		}
		else
		{
			for (const auto& pUnit : UnitVectorReversing)
				EventExt::RaiseToggleReversingStance(pUnit);

			wchar_t buffer[0x100];
			swprintf_s(buffer, GeneralUtils::LoadStringUnlessMissing("MSG:REVERSING_STANCE_OFF", L"%i unit(s) ceased Reversing Stance."), UnitVectorReversing.size());
			MessageListClass::Instance.PrintMessage(buffer);
		}
	}
}
