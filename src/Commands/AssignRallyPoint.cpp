#include "AssignRallyPoint.h"

#include <Utilities/Debug.h>

const char* AssignRallyPointCommandClass::GetName() const
{
	return "AssignRallyPoint";
}

const wchar_t* AssignRallyPointCommandClass::GetUIName() const
{
	return L"Assign rally point";
}

const wchar_t* AssignRallyPointCommandClass::GetUICategory() const
{
	return CATEGORY_DEVELOPMENT;
}

const wchar_t* AssignRallyPointCommandClass::GetUIDescription() const
{
	return L"Dummy";
}

void AssignRallyPointCommandClass::Execute(WWKey eInput) const
{
	Debug::Log("[Phobos] Dummy command runs.\n");
	MessageListClass::Instance->PrintMessage(L"[Phobos] Dummy command runs.");
}
