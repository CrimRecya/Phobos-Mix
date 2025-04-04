#include "UnifiedTechnoColor.h"

#include <TacticalClass.h>
#include <VocClass.h>

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

	// Play sound
	VocClass::PlayGlobal(RulesClass::Instance->GUIMainButtonSound, 0x2000, 1.0);

	// Redraw tactical
	TacticalClass::Instance->RegisterDirtyArea(DSurface::ViewBounds, false);

	// Redraw radar
	const auto pRadar = &RadarClass::Instance;

	if (pRadar->IsAvailableNow)
	{
		const auto pSurface = reinterpret_cast<DSurface*>(pRadar->unknown_121C);
		const auto width = pSurface->GetWidth();
		const auto height = pSurface->GetHeight();

		for (int x = 0; x < width; ++x)
		{
			for (int y = 0; y < height; ++y)
				reinterpret_cast<void(__thiscall*)(RadarClass*, const Point2D&)>(0x6562D0)(pRadar, Point2D{x,y});
		}
	}
}
