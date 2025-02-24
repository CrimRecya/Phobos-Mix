#include "SelectedInfo.h"

#include <Ext/Sidebar/SelectedButton/SelectedInfoClass.h>

const char* SelectedInfoCommandClass::GetName() const
{
	return "Selected Technos Display";
}

const wchar_t* SelectedInfoCommandClass::GetUIName() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_SELECTED_INFO", L"Selected technos display");
}

const wchar_t* SelectedInfoCommandClass::GetUICategory() const
{
	return CATEGORY_CONTROL;
}

const wchar_t* SelectedInfoCommandClass::GetUIDescription() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_SELECTED_INFO_DESC", L"Switch between visible/invisible modes for selected technos info display");
}

void SelectedInfoCommandClass::Execute(WWKey eInput) const
{
	SelectedInfoClass::Instance.SwitchVisible();
}
