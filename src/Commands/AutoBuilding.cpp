#include "AutoBuilding.h"

#include <HouseClass.h>
#include <MessageListClass.h>
#include <Utilities/GeneralUtils.h>

const char* AutoBuildingCommandClass::GetName() const
{
	return "Auto Building";
}

const wchar_t* AutoBuildingCommandClass::GetUIName() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_AUTO_BUILD", L"Toggle Auto Building");
}

const wchar_t* AutoBuildingCommandClass::GetUICategory() const
{
	return CATEGORY_CONTROL;
}

const wchar_t* AutoBuildingCommandClass::GetUIDescription() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_AUTO_BUILD_DESC", L"Toggle on/off automatically place building");
}

void AutoBuildingCommandClass::Execute(WWKey eInput) const
{
	Phobos::Config::AutomaticPlacingBuilding = !Phobos::Config::AutomaticPlacingBuilding;
	const int tabIndex = SidebarClass::Instance->ActiveTabIndex;

	if (!tabIndex || tabIndex == 1)
	{
		SidebarClass::Instance->SidebarBackgroundNeedsRedraw = true;
		SidebarClass::Instance->RepaintSidebar(tabIndex);
	}
}

const char* AutoBuildingCombatCommandClass::GetName() const
{
	return "Auto Combat Building";
}

const wchar_t* AutoBuildingCombatCommandClass::GetUIName() const
{
	return L"Toggle Auto Building of Combat Tab";
}

const wchar_t* AutoBuildingCombatCommandClass::GetUICategory() const
{
	return CATEGORY_CONTROL;
}

const wchar_t* AutoBuildingCombatCommandClass::GetUIDescription() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_AUTO_COMBAT_BUILD_DESC", L"Toggle on/off automatically place combat building");
}

void AutoBuildingCombatCommandClass::Execute(WWKey eInput) const
{
	Phobos::Config::AutomaticPlacingCombatBuilding = !Phobos::Config::AutomaticPlacingCombatBuilding;
	const int tabIndex = SidebarClass::Instance->ActiveTabIndex;

	if (!tabIndex || tabIndex == 1)
	{
		SidebarClass::Instance->SidebarBackgroundNeedsRedraw = true;
		SidebarClass::Instance->RepaintSidebar(tabIndex);
	}
}
