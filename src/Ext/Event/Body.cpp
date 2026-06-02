#include "Body.h"

#include <Utilities/Debug.h>
#include <Ext/House/Body.h>
#include <Ext/Rules/Body.h>
#include "Ext/Techno/Body.h"
#include <Ext/Building/Body.h>
#include <Ext/WarheadType/Body.h>

#include <Helpers/Macro.h>
#include <BuildingClass.h>
#include <EventClass.h>
#include <HouseClass.h>
#include <FootClass.h>
#include <ShapeButtonClass.h>

bool EventExt::AddEvent()
{
	return EventClass::OutList.Add(*reinterpret_cast<EventClass*>(this));
}

void EventExt::RespondEvent()
{
	switch (this->Type)
	{
	case EventTypeExt::ApproachObject:
		this->RespondApproachObject();
		break;

	case EventTypeExt::TogglePlayerAutoRepair:
		this->RespondToTogglePlayerAutoRepair();
		break;

	case EventTypeExt::ManualReload:
		this->RespondToManualReloadEvent();
		break;

	case EventTypeExt::ToggleAggressiveStance:
		this->RespondToToggleAggressiveStance();
		break;

	case EventTypeExt::ToggleCeaseFireStance:
		this->RespondToToggleCeaseFireStance();
		break;

	case EventTypeExt::AssignSecondaryRallyPoint:
		this->RespondToAssignSecondaryRallyPoint();
		break;
	default:
		break;
	}
}

void EventExt::RaiseManualReloadEvent(TechnoClass* pTechno)
{
	EventExt eventExt {};
	eventExt.Type = EventTypeExt::ManualReload;
	eventExt.HouseIndex = static_cast<char>(pTechno->Owner->ArrayIndex);
	eventExt.Frame = Unsorted::CurrentFrame;
	eventExt.ManualReloadEvent.Who = TargetClass(pTechno);
	eventExt.AddEvent();
	Debug::LogGame("Adding event MANUAL_RELOAD\n");
}

void EventExt::RespondToManualReloadEvent()
{
	const auto pTechno = this->ManualReloadEvent.Who.As_Techno();

	if (TechnoExt::IsActive(pTechno) && pTechno->Ammo > 0 && !pTechno->Berzerk)
	{
		const auto pType = pTechno->GetTechnoType();
		const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

		if (pTypeExt->CanManualReload && (pTechno->Ammo != pType->Ammo || pTypeExt->CanManualReload_WhenFull))
		{
			if (pTypeExt->CanManualReload_DetonateWarhead && pTypeExt->CanManualReload_DetonateConsume <= pTechno->Ammo)
				WarheadTypeExt::DetonateAt(pTypeExt->CanManualReload_DetonateWarhead.Get(), pTechno->GetCoords(), pTechno, pTechno->Ammo, pTechno->Owner, pTechno->Target);

			if (pTypeExt->CanManualReload_ResetROF)
				pTechno->RearmTimer.Stop();

			pTechno->Ammo = 0;

			if (pTechno->WhatAmI() != AbstractType::Aircraft)
				pTechno->StartReloading();
		}
	}
}

void EventExt::RaiseToggleAggressiveStance(TechnoClass* pTechno)
{
	EventExt eventExt {};
	eventExt.Type = EventTypeExt::ToggleAggressiveStance;
	eventExt.HouseIndex = static_cast<char>(pTechno->Owner->ArrayIndex);
	eventExt.Frame = Unsorted::CurrentFrame;
	eventExt.ToggleAggressiveStance.Who = TargetClass(pTechno);
	eventExt.AddEvent();
	Debug::LogGame("Adding event TOGGLE_AGGRESSIVE\n");
}

void EventExt::RespondToToggleAggressiveStance()
{
	if (const auto pTechno = this->ToggleAggressiveStance.Who.As_Techno())
	{
		if (pTechno->IsAlive && !pTechno->Berzerk)
		{
			const auto pTechnoExt = TechnoExt::ExtMap.Find(pTechno);

			if (pTechnoExt->CanToggleAggressiveStance())
				pTechnoExt->ToggleAggressiveStance();
		}
	}
}

void EventExt::RaiseToggleCeaseFireStance(TechnoClass* pTechno)
{
	EventExt eventExt {};
	eventExt.Type = EventTypeExt::ToggleCeaseFireStance;
	eventExt.HouseIndex = static_cast<char>(pTechno->Owner->ArrayIndex);
	eventExt.Frame = Unsorted::CurrentFrame;
	eventExt.ToggleCeaseFireStance.Who = TargetClass(pTechno);
	eventExt.AddEvent();
	Debug::LogGame("Adding event TOGGLE_CEASEFIRE\n");
}

void EventExt::RespondToToggleCeaseFireStance()
{
	if (const auto pTechno = this->ToggleCeaseFireStance.Who.As_Techno())
	{
		if (pTechno->IsAlive && !pTechno->Berzerk)
		{
			const auto pTechnoExt = TechnoExt::ExtMap.Find(pTechno);

			if (pTechnoExt->CanToggleCeaseFireStance())
				pTechnoExt->ToggleCeaseFireStance();
		}
	}
}

void EventExt::RaiseAssignSecondaryRallyPoint(BuildingClass* pBuilding, AbstractClass* pTarget)
{
	EventExt eventExt {};
	eventExt.Type = EventTypeExt::AssignSecondaryRallyPoint;
	eventExt.HouseIndex = static_cast<char>(pBuilding->Owner->ArrayIndex);
	eventExt.Frame = Unsorted::CurrentFrame;
	eventExt.AssignSecondaryRallyPoint.Who = TargetClass(pBuilding);
	eventExt.AssignSecondaryRallyPoint.Whom = TargetClass(pTarget);
	eventExt.AddEvent();
	Debug::LogGame("Adding event ASSIGN_BLDRALLY\n");
}

void EventExt::RespondToAssignSecondaryRallyPoint()
{
	if (const auto pBuilding = this->AssignSecondaryRallyPoint.Who.As_Building())
	{
		if (pBuilding->IsAlive && BuildingTypeExt::ExtMap.Find(pBuilding->Type)->HasSecondaryRallyPoint)
			BuildingExt::ExtMap.Find(pBuilding)->SecondaryArchiveTarget = this->AssignSecondaryRallyPoint.Whom.As_Abstract();
	}
}

void EventExt::RaiseTogglePlayerAutoRepair()
{
	EventExt eventExt {};
	eventExt.Type = EventTypeExt::TogglePlayerAutoRepair;
	eventExt.HouseIndex = (char)HouseClass::CurrentPlayer->ArrayIndex;
	eventExt.Frame = Unsorted::CurrentFrame;
	eventExt.AddEvent();
	Debug::LogGame("Adding event TOGGLE_PLAYER_AUTOREPAIR\n");
}

size_t EventExt::GetDataSize(EventTypeExt type)
{
	switch (type)
	{
	case EventTypeExt::ApproachObject:
		return sizeof(EventExt::ApproachObject);
	case EventTypeExt::TogglePlayerAutoRepair:
		return sizeof(EventExt::TogglePlayerAutoRepair);
	case EventTypeExt::ManualReload:
		return sizeof(EventExt::ManualReloadEvent);
	case EventTypeExt::ToggleAggressiveStance:
		return sizeof(EventExt::ToggleAggressiveStance);
	case EventTypeExt::ToggleCeaseFireStance:
		return sizeof(EventExt::ToggleCeaseFireStance);
	case EventTypeExt::AssignSecondaryRallyPoint:
		return sizeof(EventExt::AssignSecondaryRallyPoint);
	default:
		break;
	}

	return 0;
}

bool EventExt::IsValidType(EventTypeExt type)
{
	return (type >= EventTypeExt::FIRST && type <= EventTypeExt::LAST);
}

void EventExt::RespondApproachObject()
{
	const auto pSource = this->ApproachObject.Whom.As_Foot();

	if (!pSource || static_cast<char>(pSource->Owner->ArrayIndex) != this->HouseIndex)
		return;

	pSource->ClearPlanningTokens(nullptr);

	if (!pSource->IsAlive || pSource->Health <= 0 || pSource->InLimbo)
		return;

	if (pSource->IsTether)
	{
		const auto pLink = abstract_cast<BuildingClass*>(pSource->GetNthLink());

		if (pLink && pLink->IsAlive && pLink->Type->DockUnload)
		{
			pSource->SendToFirstLink(RadioCommand::NotifyUnlink);
			pSource->IsTether = false;
		}
	}
	else
	{
		pSource->SendToFirstLink(RadioCommand::NotifyUnlink);
	}

	pSource->QueueUpToEnter = nullptr;
	pSource->LastDestination = nullptr;

	if (const auto pManager = pSource->SlaveManager)
		pManager->AllGuard();

	pSource->ClearNavigationList();
	pSource->SetDestination(nullptr, true);
	// According to the report at https://github.com/Phobos-developers/Phobos/pull/2134#issuecomment-4062110663:
	// If the target is not cleared here, it may cause desync. The specific reason has not been fully investigated.
	// Anyone is welcome to provide a more detailed explanation.
	pSource->SetTarget(nullptr);
	pSource->SetArchiveTarget(nullptr);

	const auto pObject = this->ApproachObject.Target.As_Object();

	if (!pObject)
		return;

	pSource->Target = pObject;
	pSource->ApproachTarget(0);
	pSource->Target = nullptr;
}

void EventExt::RespondToTogglePlayerAutoRepair()
{
	if (this->HouseIndex >= HouseClass::Array.Count)
		return;

	if (!RulesExt::Global()->ExtendedPlayerRepair)
		return;

	auto pHouse = HouseClass::Array.GetItem(this->HouseIndex);
	auto pHouseExt = HouseExt::ExtMap.Find(pHouse);
	pHouseExt->PlayerAutoRepair = !pHouseExt->PlayerAutoRepair;

	if (HouseClass::CurrentPlayer == pHouse)
	{
		SidebarClass::Instance.SidebarNeedsRedraw = true;

		if (pHouseExt->PlayerAutoRepair)
			SidebarClass::ToggleRepairButton.TurnOn();
		else
			SidebarClass::ToggleRepairButton.TurnOff();
	}
}

// hooks

DEFINE_HOOK(0x4C6CC8, Networking_RespondToEvent, 0x5)
{
	GET(EventExt*, pEvent, ESI);

	if (EventExt::IsValidType(pEvent->Type))
		pEvent->RespondEvent();

	return 0;
}

DEFINE_HOOK(0x64B6FE, sub_64B660_GetEventSize, 0x6)
{
	const auto eventType = static_cast<EventTypeExt>(R->EDI() & 0xFF);

	if (EventExt::IsValidType(eventType))
	{
		const size_t eventSize = EventExt::GetDataSize(eventType);

		R->EDX(eventSize);
		R->EBP(eventSize);
		return 0x64B71D;
	}

	return 0;
}

DEFINE_HOOK(0x64BE7D, sub_64BDD0_GetEventSize1, 0x6)
{
	const auto eventType = static_cast<EventTypeExt>(R->EDI() & 0xFF);

	if (EventExt::IsValidType(eventType))
	{
		const size_t eventSize = EventExt::GetDataSize(eventType);

		REF_STACK(size_t, eventSizeInStack, STACK_OFFSET(0xAC, -0x8C));
		eventSizeInStack = eventSize;
		R->ECX(eventSize);
		R->EBP(eventSize);
		return 0x64BE97;
	}

	return 0;
}

DEFINE_HOOK(0x64C30E, sub_64BDD0_GetEventSize2, 0x6)
{
	const auto eventType = static_cast<EventTypeExt>(R->ESI() & 0xFF);

	if (EventExt::IsValidType(eventType))
	{
		const size_t eventSize = EventExt::GetDataSize(eventType);

		R->ECX(eventSize);
		R->EBP(eventSize);
		return 0x64C321;
	}

	return 0;
}

