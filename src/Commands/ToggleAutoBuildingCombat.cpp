#include "ToggleAutoBuildingCombat.h"

#include <Utilities/Debug.h>

const char* ToggleAutoBuildingCombatCommandClass::GetName() const
{
	return "ToggleAutoBuildingCombat";
}

const wchar_t* ToggleAutoBuildingCombatCommandClass::GetUIName() const
{
	return L"Toggle auto building of combat tab";
}

const wchar_t* ToggleAutoBuildingCombatCommandClass::GetUICategory() const
{
	return CATEGORY_DEVELOPMENT
}

const wchar_t* ToggleAutoBuildingCombatCommandClass::GetUIDescription() const
{
	return L"Toggle auto building of the building types in combat tab";
}

void ToggleAutoBuildingCombatCommandClass::Execute(WWKey eInput) const
{
	Debug::Log("[Phobos] Dummy command runs.\n");
	MessageListClass::Instance->PrintMessage(L"[Phobos] Dummy command runs.");
}
