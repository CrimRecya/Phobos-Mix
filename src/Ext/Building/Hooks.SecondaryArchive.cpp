#include "Body.h"

#include <TacticalClass.h>

namespace SecondaryRallyPoint
{
	BuildingClass* pBuilding = nullptr;
	AbstractClass* ArchiveTarget = nullptr;
	AbstractClass* SecondaryArchiveTarget = nullptr;
}

DEFINE_HOOK(0x6DAAB2, TacticalClass_DrawRallyPointLines_SecondaryRallyPoint1, 0x6)
{
	enum { SkipDraw = 0x6DAD45, DoDraw = 0x6DAAC0 };

	GET(BuildingClass*, pBuilding, EDI);

	if (pBuilding->CurrentMission == Mission::Selling)
		return SkipDraw;

	SecondaryRallyPoint::pBuilding = pBuilding;
	const auto pRally = pBuilding->ArchiveTarget;
	const auto pSecondaryRally = BuildingExt::ExtMap.Find(pBuilding)->SecondaryArchiveTarget;

	if (pRally)
		SecondaryRallyPoint::ArchiveTarget = pRally;

	if (pSecondaryRally)
		SecondaryRallyPoint::SecondaryArchiveTarget = pSecondaryRally;

	return pRally || pSecondaryRally ? DoDraw : SkipDraw;
}

DEFINE_HOOK(0x6DAB20, TacticalClass_DrawRallyPointLines_SecondaryRallyPoint2, 0x6)
{
	enum { SetArchiveTarget = 0x6DAB26 };
	R->ECX(SecondaryRallyPoint::ArchiveTarget ? SecondaryRallyPoint::ArchiveTarget : SecondaryRallyPoint::SecondaryArchiveTarget);
	return SetArchiveTarget;
}

DEFINE_HOOK(0x6DAD45, TacticalClass_DrawRallyPointLines_SecondaryRallyPoint3, 0x5)
{
	enum { DrawAgain = 0x6DAAC0 };

	if (SecondaryRallyPoint::SecondaryArchiveTarget && SecondaryRallyPoint::pBuilding)
	{
		R->EDI(SecondaryRallyPoint::pBuilding);
		SecondaryRallyPoint::pBuilding = nullptr;
		return DrawAgain;
	}

	return 0;
}

DEFINE_HOOK(0x6DAC80, TacticalClass_DrawRallyPointLines_SecondaryRallyPoint4, 0x8)
{
	if (!SecondaryRallyPoint::ArchiveTarget)
	{
		R->CL(255);
		R->DL(255);
	}

	return 0;
}

DEFINE_HOOK(0x6DAC92, TacticalClass_DrawRallyPointLines_SecondaryRallyPoint5, 0x6)
{
	if (SecondaryRallyPoint::ArchiveTarget)
	{
		SecondaryRallyPoint::ArchiveTarget = nullptr;
	}
	else
	{
		R->AL(255);
		SecondaryRallyPoint::SecondaryArchiveTarget = nullptr;
	}

	return 0;
}

DEFINE_HOOK(0x455D50, BuildingClass_SetDestination_ResetSecondaryRallyPoint, 0xA)
{
	GET(BuildingClass*, pThis, ECX);
	GET_STACK(AbstractClass*, pTarget, STACK_OFFSET(0, 0x4));

	if (!pTarget)
		BuildingExt::ExtMap.Find(pThis)->SecondaryArchiveTarget = nullptr;

	return 0;
}
