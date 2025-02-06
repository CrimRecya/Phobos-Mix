#include "AggressiveStance.h"

#include "Ext/Techno/Body.h"
#include <Ext/Event/Body.h>

const char* AggressiveStanceClass::GetName() const
{
	return "AggressiveStance";
}

const wchar_t* AggressiveStanceClass::GetUIName() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_AGGRESSIVE_STANCE", L"Aggressive Stance");
}

const wchar_t* AggressiveStanceClass::GetUICategory() const
{
	return CATEGORY_CONTROL;
}

const wchar_t* AggressiveStanceClass::GetUIDescription() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_AGGRESSIVE_STANCE_DESC", L"Aggressive Stance");
}

void AggressiveStanceClass::Execute(WWKey eInput) const
{
	std::vector<TechnoClass*> TechnoVectorAggressive;
	std::vector<TechnoClass*> TechnoVectorNonAggressive;

	// Get current selected units.
	// If all selected units are at aggressive stance, we should cancel their aggressive stance.
	// Otherwise, we should turn them into aggressive stance.
	bool isAnySelectedUnitTogglable = false;
	bool isAllSelectedUnitAggressiveStance = true;

	for (const auto& pUnit : ObjectClass::CurrentObjects())
	{
		// try to cast to TechnoClass
		TechnoClass* pTechno = abstract_cast<TechnoClass*>(pUnit);

		// if not a techno or is in berserk or is not controlled by the local player then ignore it
		if (!pTechno || pTechno->Berzerk || !pTechno->Owner->IsControlledByCurrentPlayer())
			continue;

		const auto pTechnoExt = TechnoExt::ExtMap.Find(pTechno);

		// If not togglable then exclude it from the iteration.
		if (!pTechnoExt->CanToggleAggressiveStance())
			continue;

		isAnySelectedUnitTogglable = true;

		if (pTechnoExt->GetAggressiveStance())
		{
			TechnoVectorAggressive.push_back(pTechno);
		}
		else
		{
			isAllSelectedUnitAggressiveStance = false;
			TechnoVectorNonAggressive.push_back(pTechno);
		}
	}

	// If this boolean is false, then none of the selected units are togglable, meaning this hotket doesn't need to do anything.
	if (isAnySelectedUnitTogglable)
	{
		// If all selected units are aggressive stance, then cancel their aggressive stance;
		// otherwise, make all selected units aggressive stance.
		if (isAllSelectedUnitAggressiveStance)
		{
			for (const auto& pTechno : TechnoVectorAggressive)
			{
				EventExt::RaiseToggleAggressiveStance(pTechno);
				pTechno->QueueVoice(TechnoTypeExt::ExtMap.Find(pTechno->GetTechnoType())->VoiceExitAggressiveStance.Get());
			}

			wchar_t buffer[0x100];
			swprintf_s(buffer, GeneralUtils::LoadStringUnlessMissing("MSG:AGGRESSIVE_STANCE_OFF", L"%i unit(s) ceased Aggressive Stance."), TechnoVectorAggressive.size());
			MessageListClass::Instance->PrintMessage(buffer);
		}
		else
		{
			for (const auto& pTechno : TechnoVectorNonAggressive)
			{
				EventExt::RaiseToggleAggressiveStance(pTechno);
				const auto pTechnoType = pTechno->GetTechnoType();
				int voiceIndex = TechnoTypeExt::ExtMap.Find(pTechnoType)->VoiceEnterAggressiveStance.Get();

				if (voiceIndex < 0)
				{
					const auto& voiceList = pTechnoType->VoiceAttack.Count ? pTechnoType->VoiceAttack : pTechnoType->VoiceMove;

					if (const auto count = voiceList.Count)
						voiceIndex = voiceList.GetItem(Randomizer::Global().Random() % count);
				}

				pTechno->QueueVoice(voiceIndex);
			}

			wchar_t buffer[0x100];
			swprintf_s(buffer, GeneralUtils::LoadStringUnlessMissing("MSG:AGGRESSIVE_STANCE_ON", L"%i unit(s) entered Aggressive Stance."), TechnoVectorNonAggressive.size());
			MessageListClass::Instance->PrintMessage(buffer);
		}
	}
}
