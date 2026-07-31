#pragma once
#include <EventClass.h>
#include <TargetClass.h>
#include <HouseClass.h>
#include <TechnoClass.h>

#include <cstddef>
#include <stdint.h>

enum class EventTypeExt : uint8_t
{
	// Vanilla game used Events from 0x00 to 0x2F
	// CnCNet reserved Events from 0x30 to 0x3F
	// Ares used Events 0x60 and 0x61

	ApproachObject = 0x80,
	TogglePlayerAutoRepair = 0x81,
	ManualReload = 0x82,
	ToggleAggressiveStance = 0x83,
	ToggleCeaseFireStance = 0x84,
	ToggleReversingStance = 0x85,
	AssignSecondaryRallyPoint = 0x86,

	FIRST = ApproachObject,
	LAST = AssignSecondaryRallyPoint
};

#pragma pack(push, 1)
class EventExt
{
	struct EventStruct_Obj0
	{
	};
	struct EventStruct_Obj1
	{
		TargetClass Whom;
	};
	struct EventStruct_Obj2
	{
		TargetClass Whom;
		TargetClass Target;
	};

public:
	EventTypeExt Type;
	bool IsExecuted;
	char HouseIndex;
	uint32_t Frame;
	union
	{
		char DataBuffer[104];
		EventStruct_Obj2 ApproachObject;
		EventStruct_Obj0 TogglePlayerAutoRepair;
		EventStruct_Obj1 ManualReloadEvent;
		EventStruct_Obj1 ToggleAggressiveStance;
		EventStruct_Obj1 ToggleCeaseFireStance;
		EventStruct_Obj1 ToggleReversingStance;
		EventStruct_Obj2 AssignSecondaryRallyPoint;
	};

	bool AddEvent();
	void RespondEvent();

	void RespondApproachObject();
	static void RaiseTogglePlayerAutoRepair();
	void RespondToTogglePlayerAutoRepair();

	static void RaiseManualReloadEvent(TechnoClass* pTechno);
	void RespondToManualReloadEvent();

	static void RaiseToggleAggressiveStance(TechnoClass* pTechno);
	void RespondToToggleAggressiveStance();

	static void RaiseToggleCeaseFireStance(TechnoClass* pTechno);
	void RespondToToggleCeaseFireStance();

	static void RaiseToggleReversingStance(TechnoClass* pTechno);
	void RespondToToggleReversingStance();

	static void RaiseAssignSecondaryRallyPoint(BuildingClass* pBuilding, AbstractClass* pTarget);
	void RespondToAssignSecondaryRallyPoint();

	static size_t GetDataSize(EventTypeExt type);
	static bool IsValidType(EventTypeExt type);
};

static_assert(sizeof(EventExt) == 111);
static_assert(offsetof(EventExt, DataBuffer) == 7);
#pragma pack(pop)

