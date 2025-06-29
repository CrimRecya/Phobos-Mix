#include "ShowCurrentInfo.h"

const char* ShowCurrentInfoCommandClass::GetName() const
{
	return "CrimViewerEx";
}

const wchar_t* ShowCurrentInfoCommandClass::GetUIName() const
{
	return L"CrimViewerEx";
}

const wchar_t* ShowCurrentInfoCommandClass::GetUICategory() const
{
	return CATEGORY_DEVELOPMENT;
}

const wchar_t* ShowCurrentInfoCommandClass::GetUIDescription() const
{
	return L"Show Object Extra Info - CrimRecya";
}

void ShowCurrentInfoCommandClass::Execute(WWKey eInput) const
{
	Phobos::ShowCurrentInfo = !Phobos::ShowCurrentInfo;
}
