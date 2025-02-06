#include "FPSNewCounter.h"

#include <Utilities/GeneralUtils.h>

const char* FPSNewCounterCommandClass::GetName() const
{
	return "Phobos FPS Counter";
}

const wchar_t* FPSNewCounterCommandClass::GetUIName() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_FPSC_INFO", L"Phobos FPS Counter");
}

const wchar_t* FPSNewCounterCommandClass::GetUICategory() const
{
	return CATEGORY_CONTROL;
}

const wchar_t* FPSNewCounterCommandClass::GetUIDescription() const
{
	return GeneralUtils::LoadStringUnlessMissing("TXT_FPSC_INFO_DESC", L"Switch between visible/invisible modes for FPS counter");
}

void FPSNewCounterCommandClass::Execute(WWKey eInput) const
{
	Phobos::Config::FPSCounter_Enable = !Phobos::Config::FPSCounter_Enable;
}
