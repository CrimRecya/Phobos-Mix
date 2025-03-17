#include "ShowCurrentInfo.h"

#include <MessageListClass.h>
#include <Utilities/GeneralUtils.h>

const char* ShowCurrentInfoCommandClass::GetName() const
{
	return "Show Current Object Info";
}

const wchar_t* ShowCurrentInfoCommandClass::GetUIName() const
{
	return L"Show Current Object Info";
}

const wchar_t* ShowCurrentInfoCommandClass::GetUICategory() const
{
	return CATEGORY_DEVELOPMENT;
}

const wchar_t* ShowCurrentInfoCommandClass::GetUIDescription() const
{
	return L"Show Current Object Info.";
}

void ShowCurrentInfoCommandClass::Execute(WWKey eInput) const
{
	Phobos::ShowCurrentInfo = !Phobos::ShowCurrentInfo;
}
