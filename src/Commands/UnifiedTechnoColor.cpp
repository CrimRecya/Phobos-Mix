#include "UnifiedTechnoColor.h"

#include <Utilities/GeneralUtils.h>

const char* UnifiedTechnoColorCommandClass::GetName() const
{
	return "Unify techno color";
}

const wchar_t* UnifiedTechnoColorCommandClass::GetUIName() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_UNIFY_COLOR", L"Unify techno color");
}

const wchar_t* UnifiedTechnoColorCommandClass::GetUICategory() const
{
	return CATEGORY_INTERFACE;
}

const wchar_t* UnifiedTechnoColorCommandClass::GetUIDescription() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_UNIFY_COLOR_DESC", L"Switch on/off unified techno color display mode");
}

void UnifiedTechnoColorCommandClass::Execute(WWKey eInput) const
{
	Phobos::Config::UnifiedTechnoColor = !Phobos::Config::UnifiedTechnoColor;
}
