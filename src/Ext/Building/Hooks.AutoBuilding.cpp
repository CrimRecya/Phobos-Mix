#include "Body.h"

#include <BulletClass.h>
#include <UnitClass.h>
#include <SuperClass.h>
#include <GameOptionsClass.h>
#include <Ext/Anim/Body.h>
#include <Ext/House/Body.h>
#include <Ext/SWType/Body.h>
#include <Ext/WarheadType/Body.h>
#include <TacticalClass.h>
#include <PlanningTokenClass.h>

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

	int ret = SkipDraw;
	SecondaryRallyPoint::pBuilding = pBuilding;

	if (auto pRally = pBuilding->ArchiveTarget)
	{
		SecondaryRallyPoint::ArchiveTarget = pRally;
		ret = DoDraw;
	}

	if (auto pSecondaryRally = BuildingExt::ExtMap.Find(pBuilding)->SecondaryArchiveTarget)
	{
		SecondaryRallyPoint::SecondaryArchiveTarget = pSecondaryRally;
		ret = DoDraw;
	}

	return ret;
}

DEFINE_HOOK(0x6DAB20, TacticalClass_DrawRallyPointLines_SecondaryRallyPoint2, 0x6)
{
	enum { ret = 0x6DAB26 };

	if (SecondaryRallyPoint::ArchiveTarget)
	{
		R->ECX(SecondaryRallyPoint::ArchiveTarget);
		SecondaryRallyPoint::ArchiveTarget = nullptr;
		return ret;
	}
	else
	{
		R->ECX(SecondaryRallyPoint::SecondaryArchiveTarget);
		SecondaryRallyPoint::SecondaryArchiveTarget = nullptr;
		return ret;
	}
}

DEFINE_HOOK(0x6DAD45, TacticalClass_DrawRallyPointLines_SecondaryRallyPoint3, 0x5)
{
	if (SecondaryRallyPoint::SecondaryArchiveTarget)
	{
		R->EDI(SecondaryRallyPoint::pBuilding);
		return 0x6DAAC0;
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
