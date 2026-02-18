#pragma once

#include <cstddef>
#include <stdint.h>
#include <TechnoClass.h>
#include <TargetClass.h>

enum class EventTypeExt : uint8_t
{
	// Vanilla game used Events from 0x00 to 0x2F
	// CnCNet reserved Events from 0x30 to 0x3F
	// Ares used Events 0x60 and 0x61

	ApproachObject = 0x80,
	ManualReload = 0x81,
	ToggleAggressiveStance = 0x82,
	ToggleCeaseFireStance = 0x83,
	AssignSecondaryRallyPoint = 0x84,

	FIRST = ApproachObject,
	LAST = AssignSecondaryRallyPoint
};

#pragma pack(push, 1)
class EventExt
{
public:
	EventTypeExt Type;
	bool IsExecuted;
	char HouseIndex;
	uint32_t Frame;
	union
	{
		char DataBuffer[104];

		struct APPROACHOBJECT
		{
			TargetClass Whom;
			TargetClass Target;
		} ApproachObject;

		struct ManualReloadEvent
		{
			TargetClass Who;
		} ManualReloadEvent;

		struct ToggleAggressiveStance
		{
			TargetClass Who;
		} ToggleAggressiveStance;

		struct ToggleCeaseFireStance
		{
			TargetClass Who;
		} ToggleCeaseFireStance;

		struct AssignSecondaryRallyPoint
		{
			TargetClass Who;
			TargetClass Whom;
		} AssignSecondaryRallyPoint;
	};

	bool AddEvent();
	void RespondEvent();

	void RespondApproachObject();

	static void RaiseManualReloadEvent(TechnoClass* pTechno);
	void RespondToManualReloadEvent();

	static void RaiseToggleAggressiveStance(TechnoClass* pTechno);
	void RespondToToggleAggressiveStance();

	static void RaiseToggleCeaseFireStance(TechnoClass* pTechno);
	void RespondToToggleCeaseFireStance();

	static void RaiseAssignSecondaryRallyPoint(BuildingClass* pBuilding, AbstractClass* pTarget);
	void RespondToAssignSecondaryRallyPoint();

	static size_t GetDataSize(EventTypeExt type);
	static bool IsValidType(EventTypeExt type);
};

static_assert(sizeof(EventExt) == 111);
static_assert(offsetof(EventExt, DataBuffer) == 7);
#pragma pack(pop)
