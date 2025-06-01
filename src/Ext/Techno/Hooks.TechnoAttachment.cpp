#include "Body.h"

#include <EventClass.h>
#include <WarheadTypeClass.h>
#include <TacticalClass.h>

#include <Commands/DistributionMode.h>
#include <Ext/TechnoType/Body.h>
#include <Ext/Cell/Body.h>

#include <Utilities/Helpers.Alex.h>
#include <Utilities/Macro.h>

#include <Commands/FrameByFrame.h>

DEFINE_HOOK(0x707CB3, TechnoClass_KillCargo_HandleAttachments, 0x6)
{
	GET(TechnoClass*, pThis, EBX);
	GET_STACK(TechnoClass*, pSource, STACK_OFFSET(0x4, 0x4));

	TechnoExt::DestroyAttachments(pThis, pSource);

	return 0;
}

DEFINE_HOOK(0x5F6609, ObjectClass_RemoveThis_TechnoClass_NotifyParent, 0x9)
{
	GET(TechnoClass*, pThis, ESI);

	pThis->KillPassengers(nullptr);  // restored code
	TechnoExt::HandleDestructionAsChild(pThis);

	return 0x5F6612;
}

DEFINE_HOOK(0x4DEBB4, FootClass_OnDestroyed_NotifyParent, 0x8)
{
	GET(FootClass*, pThis, ESI);

	TechnoExt::HandleDestructionAsChild(pThis);

	return 0;
}


DEFINE_HOOK(0x6F6F20, TechnoClass_Unlimbo_UnlimboAttachments, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	TechnoExt::UnlimboAttachments(pThis);

	return 0;
}

DEFINE_HOOK(0x6F6B1C, TechnoClass_Limbo_LimboAttachments, 0x6)
{
	GET(TechnoClass*, pThis, ESI);

	TechnoExt::LimboAttachments(pThis);

	return 0;
}

#pragma region Cell occupation handling

// see hooks for CellExt

namespace TechnoAttachmentTemp
{
	// no idea what Ares or w/e else is doing with occupation flags,
	// so just to be safe assume it can be nothing and store it
	byte storedVehicleFlag;
}

// Game assumes cell is occupied by a vehicle by default and if this vehicle
// turns out to be self, then it un-assumes the occupancy. Because with techno
// attachment logic it's possible to have multiple vehicles on the same cell,
// we flip the logic from "passable if special case is found" to "impassable if
// non-special case is found" - Kerbiter

void AssumeNoVehicleByDefault(byte& occupyFlags, bool& isVehicleFlagSet)
{
	TechnoAttachmentTemp::storedVehicleFlag = occupyFlags & 0x20;

	occupyFlags &= ~0x20;
	isVehicleFlagSet = false;
}

DEFINE_HOOK(0x73F520, UnitClass_CanEnterCell_AssumeNoVehicleByDefault, 0x0)
{
	enum { Check = 0x73F528, Skip = 0x73FA92 };

	REF_STACK(byte, occupyFlags, STACK_OFFSET(0x90, -0x7C));
	REF_STACK(bool, isVehicleFlagSet, STACK_OFFSET(0x90, -0x7B));

	GET(TechnoClass*, pOccupier, ESI);

	if (!pOccupier)  // stolen code
		return Skip;

	AssumeNoVehicleByDefault(occupyFlags, isVehicleFlagSet);

	return Check;
}

bool IsOccupierIgnorable(TechnoClass* pThis, ObjectClass* pOccupier, byte& occupyFlags, bool& isVehicleFlagSet)
{
	if (pThis == pOccupier)
		return true;

	if (pOccupier)
	{
		if (auto const pTechno = abstract_cast<TechnoClass* ,true>(pOccupier))
		{
			if (TechnoExt::DoesntOccupyCellAsChild(pTechno) || TechnoExt::IsChildOf(pTechno, pThis))
				return true;
		}

		if (abstract_cast<UnitClass* ,true>(pOccupier))
		{
			occupyFlags |= TechnoAttachmentTemp::storedVehicleFlag;
			isVehicleFlagSet = (occupyFlags & 0x20) != 0;
		}
	}

	return false;
}

DEFINE_HOOK(0x73F528, UnitClass_CanEnterCell_SkipChildren, 0x0)
{
	enum { SkipToNextOccupier = 0x73FA87, ContinueCheck = 0x73F530 };

	GET(UnitClass*, pThis, EBX);
	GET(ObjectClass*, pOccupier, ESI);

	REF_STACK(byte, occupyFlags, STACK_OFFSET(0x90, -0x7C));
	REF_STACK(bool, isVehicleFlagSet, STACK_OFFSET(0x90, -0x7B));

	return IsOccupierIgnorable(pThis, pOccupier, occupyFlags, isVehicleFlagSet) ? SkipToNextOccupier : ContinueCheck;
}

void AccountForMovingInto(CellClass* into, bool isAlt, TechnoClass* pThis, byte& occupyFlags, bool& isVehicleFlagSet)
{
	auto const pCellExt = CellExt::ExtMap.Find(into);
	auto& pIncoming = isAlt ? pCellExt->IncomingUnitAlt : pCellExt->IncomingUnit;

	// Non-occupiers shouldn't be inserted as incoming units anyways so don't check that
	if (pIncoming)
	{
		const unsigned int addr = VTable::Get(pIncoming);

		if (addr != 0x7F5C70) // UnitClass::AbsVTable
		{
			const auto& pIncomingType = isAlt ? pCellExt->IncomingUnitAltType : pCellExt->IncomingUnitType;
			const char* pName = pIncomingType ? pIncomingType->get_ID() : "N/A";
			Debug::LogAndMessage("FootClass::IsCellOccupied: Found InvalidUnit [%s] at(%d,%d) with dirty vtable in moving check!\n",
				pName, into->MapCoords.X, into->MapCoords.Y);

			pIncoming = nullptr;
			auto& occupationFlags = isAlt ? into->AltOccupationFlags : into->OccupationFlags;
			occupationFlags &= ~0x20;

			if (SessionClass::IsSingleplayer() && Phobos::Config::DevelopmentCommands)
			{
				Debug::LogAndMessage("Skip processing. Entering Stepping Mode...\n");
				FrameByFrameCommandClass::FrameStep = true;
				auto coords = into->GetCoords();
				TacticalClass::Instance->SetTacticalPosition(&coords);
			}

			return;
		}

		if (pIncoming != pThis && !TechnoExt::IsChildOf(pIncoming, pThis))
		{
			occupyFlags |= TechnoAttachmentTemp::storedVehicleFlag;
			isVehicleFlagSet = (occupyFlags & 0x20) != 0;
		}
	}
}

DEFINE_HOOK(0x73FA92, UnitClass_CanEnterCell_CheckMovingInto, 0x0)
{
	enum { ContinueCheck = 0x73FC24, NoMove = 0x73FACD };

	GET_STACK(CellClass*, into, STACK_OFFSET(0x90, 0x4));
	GET_STACK(bool const, isAlt, STACK_OFFSET(0x90, -0x7D));
	GET(UnitClass*, pThis, EBX);

	REF_STACK(byte, occupyFlags, STACK_OFFSET(0x90, -0x7C));
	REF_STACK(bool, isVehicleFlagSet, STACK_OFFSET(0x90, -0x7B));

	AccountForMovingInto(into, isAlt, pThis, occupyFlags, isVehicleFlagSet);

	// stolen code ahead
	return GroundType::Array[static_cast<int>(isAlt ? LandType::Road : into->LandType)].Cost[static_cast<int>(pThis->Type->SpeedType)] == 0.0f ? NoMove : ContinueCheck;
}

DEFINE_HOOK(0x51C249, InfantryClass_CanEnterCell_AssumeNoVehicleByDefault, 0x0)
{
	enum { Check = 0x51C251, Skip = 0x51C78F };

	REF_STACK(byte, occupyFlags, STACK_OFFSET(0x34, -0x21));
	REF_STACK(bool, isVehicleFlagSet, STACK_OFFSET(0x34, -0x22));

	GET(TechnoClass*, pOccupier, ESI);

	if (!pOccupier)  // stolen code
		return Skip;

	AssumeNoVehicleByDefault(occupyFlags, isVehicleFlagSet);

	return Check;
}

DEFINE_HOOK(0x51C251, InfantryClass_CanEnterCell_SkipChildren, 0x0)
{
	enum { IgnoreOccupier = 0x51C70F, Continue = 0x51C259 };

	GET(InfantryClass*, pThis, EBP);
	GET(ObjectClass*, pOccupier, ESI);

	REF_STACK(byte, occupyFlags, STACK_OFFSET(0x34, -0x21));
	REF_STACK(bool, isVehicleFlagSet, STACK_OFFSET(0x34, -0x22));

	return IsOccupierIgnorable(pThis, pOccupier, occupyFlags, isVehicleFlagSet) ? IgnoreOccupier : Continue;
}

DEFINE_HOOK(0x51C78F, InfantryClass_CanEnterCell_CheckMovingInto, 0x6)
{
	GET_STACK(CellClass*, into, STACK_OFFSET(0x34, 0x4));
	GET_STACK(bool const, isAlt, STACK_OFFSET(0x34, -0x23));
	GET(InfantryClass*, pThis, EBP);

	REF_STACK(byte, occupyFlags, STACK_OFFSET(0x34, -0x21));
	REF_STACK(bool, isVehicleFlagSet, STACK_OFFSET(0x34, -0x22));

	AccountForMovingInto(into, isAlt, pThis, occupyFlags, isVehicleFlagSet);

	return 0;
}

// Temporary no use
/*
enum class CellTechnoMode
{
	NoAttachments,
	NoVirtualOrRelatives,
	NoVirtual,
	NoRelatives, // misleading name but I think doesn't matter for the use case for now
	All,

	DefaultBehavior = All,
};

namespace TechnoAttachmentTemp
{
	CellTechnoMode currentMode = CellTechnoMode::DefaultBehavior;
}

#define DEFINE_CELLTECHNO_WRAPPER(mode) \
TechnoClass* __fastcall CellTechno_##mode(CellClass* pThis, discard_t, Point2D *a2, bool check_alt, TechnoClass* techno) \
{ \
	TechnoAttachmentTemp::currentMode = CellTechnoMode::mode; \
	auto const retval = pThis->FindTechnoNearestTo(*a2, check_alt, techno); \
	TechnoAttachmentTemp::currentMode = CellTechnoMode::DefaultBehavior; \
	return retval; \
}

DEFINE_CELLTECHNO_WRAPPER(NoAttachments);
DEFINE_CELLTECHNO_WRAPPER(NoVirtualOrRelatives);
DEFINE_CELLTECHNO_WRAPPER(NoVirtual);
DEFINE_CELLTECHNO_WRAPPER(NoRelatives);
DEFINE_CELLTECHNO_WRAPPER(All);

#undef DEFINE_CELLTECHNO_WRAPPER

DEFINE_HOOK(0x47C432, CellClass_CellTechno_HandleAttachments, 0x0)
{
	enum { Continue = 0x47C437, IgnoreOccupier = 0x47C4A7 };

	GET(TechnoClass*, pOccupier, ESI);
	GET_BASE(TechnoClass*, pSelf, 0x10);

	using namespace TechnoAttachmentTemp;
	const bool noAttachments =
		currentMode == CellTechnoMode::NoAttachments;
	const bool noVirtual =
		currentMode == CellTechnoMode::NoVirtual
		|| currentMode == CellTechnoMode::NoVirtualOrRelatives;
	const bool noRelatives =
		currentMode == CellTechnoMode::NoRelatives
		|| currentMode == CellTechnoMode::NoVirtualOrRelatives;

	if (pOccupier == pSelf  // restored code
		|| noAttachments && TechnoExt::IsAttached(pOccupier)
		|| noVirtual && TechnoExt::DoesntOccupyCellAsChild(pOccupier)
		|| noRelatives && TechnoExt::IsChildOf(pOccupier, (TechnoClass*)pSelf))
	{
		return IgnoreOccupier;
	}

	return Continue;
}
*/

// ExtendedPlacing
/*
// skip building placement occupation checks for virtuals
DEFINE_FUNCTION_JUMP(CALL, 0x47C805, CellTechno_NoVirtual);
DEFINE_FUNCTION_JUMP(CALL, 0x47C738, CellTechno_NoVirtual);
*/

// EnhancedScatter
/*
// skip building attachments in bib check
DEFINE_FUNCTION_JUMP(CALL, 0x4495F2, CellTechno_NoVirtualOrRelatives);
DEFINE_FUNCTION_JUMP(CALL, 0x44964E, CellTechno_NoVirtualOrRelatives);

DEFINE_HOOK(0x4495F7, BuildingClass_ClearFactoryBib_SkipCreatedUnitAttachments, 0x0)
{
	enum { BibClear = 0x44969B, NotClear = 0x4495FF };

	GET(TechnoClass*, pBibTechno, EAX);

	if (!pBibTechno)
		return BibClear;

	GET(BuildingClass*, pThis, ESI);

	TechnoClass* pBuiltTechno = pThis->GetNthLink(0);
	if (TechnoExt::IsChildOf(pBibTechno, pBuiltTechno))
		return BibClear;

	return NotClear;
}
*/

// original code doesn't account for multiple possible technos on the cell
DEFINE_HOOK(0x73A5EA, UnitClass_PerCellProcess_EntryLoopTechnos, 0x0)
{
	enum { SkipEntry = 0x73A7D2, TryEnterTarget = 0x73A6D1 };

	GET(UnitClass*, pThis, EBP);

	if (pThis->GetCurrentMission() != Mission::Enter)
		return SkipEntry;

	CellClass* pCell = pThis->GetCell();

	for (ObjectClass* pObject = (pThis->OnBridge ? pCell->AltObject : pCell->FirstObject); pObject; pObject = pObject->NextObject)
	{
		auto pEntryTarget = abstract_cast<TechnoClass*, true>(pObject);

		if (pEntryTarget
			&& pEntryTarget != pThis
			&& pEntryTarget->GetMapCoords() == pThis->GetMapCoords()
			&& pThis->ContainsLink(pEntryTarget)
			&& pEntryTarget->GetTechnoType()->Passengers > 0)
		{
			R->ESI<TechnoClass*>(pEntryTarget);
			return TryEnterTarget;
		}
	}

	return SkipEntry;
}

DEFINE_HOOK(0x51A0DA, InfantryClass_PerCellProcess_EntryLoopTechnos, 0x0)
{
	enum { SkipEntry = 0x51A4BF, TryEnterTarget = 0x51A258 };

	GET(InfantryClass*, pThis, ESI);

	if (pThis->GetCurrentMission() != Mission::Enter)
		return SkipEntry;

	CellClass* pCell = pThis->GetCell();

	for (ObjectClass* pObject = (pThis->OnBridge ? pCell->AltObject : pCell->FirstObject); pObject; pObject = pObject->NextObject)
	{
		auto pEntryTarget = abstract_cast<TechnoClass*, true>(pObject);

		// TODO additional priority checks (original code gets technos in certain order) because may backfire

		if (pEntryTarget
			&& pEntryTarget != pThis
			&& (pThis->Target == pEntryTarget
				|| pThis->Destination == pEntryTarget
				|| (pThis->OnBridge
					&& pCell == pEntryTarget->GetCell())))
		{
			R->EDI<TechnoClass*>(pEntryTarget);
			R->EBP<size_t>(0);
			return TryEnterTarget;
		}
	}

	return SkipEntry;
}

#pragma endregion

#pragma region InAir/OnGround

bool __fastcall TechnoClass_OnGround(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	return pExt->ParentAttachment && pExt->ParentAttachment->GetType()->InheritHeightStatus
		? pExt->ParentAttachment->Parent->IsOnFloor()
		: pThis->ObjectClass::IsOnFloor();
}

bool __fastcall TechnoClass_InAir(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	return pExt->ParentAttachment && pExt->ParentAttachment->GetType()->InheritHeightStatus
		? pExt->ParentAttachment->Parent->IsInAir()
		: pThis->ObjectClass::IsInAir();
}

bool __fastcall TechnoClass_IsSurfaced(TechnoClass* pThis)
{
	auto const pExt = TechnoExt::ExtMap.Find(pThis);

	return pExt->ParentAttachment && pExt->ParentAttachment->GetType()->InheritHeightStatus
		? pExt->ParentAttachment->Parent->IsSurfaced()
		: pThis->ObjectClass::IsSurfaced();
}

// TechnoClass
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F49B0, TechnoClass_OnGround);
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F49B4, TechnoClass_InAir);
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F49DC, TechnoClass_IsSurfaced);

// BuildingClass
DEFINE_FUNCTION_JUMP(VTABLE, 0x7E3F0C, TechnoClass_OnGround);
DEFINE_FUNCTION_JUMP(VTABLE, 0x7E3F10, TechnoClass_InAir);
DEFINE_FUNCTION_JUMP(VTABLE, 0x7E3F38, TechnoClass_IsSurfaced);

// FootClass
DEFINE_FUNCTION_JUMP(VTABLE, 0x7E8CE4, TechnoClass_OnGround);
DEFINE_FUNCTION_JUMP(VTABLE, 0x7E8CE8, TechnoClass_InAir);
DEFINE_FUNCTION_JUMP(VTABLE, 0x7E8D10, TechnoClass_IsSurfaced);

// UnitClass
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F5CC0, TechnoClass_OnGround);
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F5CC4, TechnoClass_InAir);
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F5CEC, TechnoClass_IsSurfaced);

// InfantryClass
DEFINE_FUNCTION_JUMP(VTABLE, 0x7EB0A8, TechnoClass_OnGround);
DEFINE_FUNCTION_JUMP(VTABLE, 0x7EB0AC, TechnoClass_InAir);
DEFINE_FUNCTION_JUMP(VTABLE, 0x7EB0D4, TechnoClass_IsSurfaced);

// AircraftClass has it's own logic, who would want to attach aircrafts anyways

#pragma endregion

DEFINE_HOOK(0x6CC763, SuperClass_Place_ChronoWarp_SkipChildren, 0x6)
{
	enum { Skip = 0x6CCCCA, Continue = 0x0 };

	GET(FootClass* const, pFoot, ESI);

	return TechnoExt::IsAttached(pFoot) ? Skip : Continue;
}

#pragma region Command inheritance

void ParentClickedWaypoint(TechnoClass* pThis, int idxPath, signed char idxWP)
{
	// Rewrite of the original code
	pThis->AssignPlanningPath(idxPath, idxWP);

	if ((pThis->AbstractFlags & AbstractFlags::Foot) == AbstractFlags::Foot)
		pThis->unknown_bool_430 = false;

	// Children handling
	if (const auto& pExt = TechnoExt::ExtMap.Find(pThis))
	{
		for (const auto& pAttachment : pExt->ChildAttachments)
		{
			if (pAttachment->Child && pAttachment->GetType()->InheritCommands)
				ParentClickedWaypoint(pAttachment->Child, idxPath, idxWP);
		}
	}
}

void ParentClickedTargetAction(TechnoClass* pThis, Action action, ObjectClass* pTarget)
{
	pThis->ObjectClickedAction(action, pTarget, false);
	Unsorted::MoveFeedback = false;

	// Children handling
	if (const auto& pExt = TechnoExt::ExtMap.Find(pThis))
	{
		for (const auto& pAttachment : pExt->ChildAttachments)
		{
			if (pAttachment->Child && pAttachment->GetType()->InheritCommands)
				ParentClickedTargetAction(pAttachment->Child, action, pTarget);
		}
	}
}

void ParentClickedCellAction(TechnoClass* pThis, Action action, CellStruct* pCell, CellStruct* pSecondCell)
{
	pThis->CellClickedAction(action, pCell, pSecondCell, false);
	Unsorted::MoveFeedback = false;

	// Children handling
	if (const auto& pExt = TechnoExt::ExtMap.Find(pThis))
	{
		for (const auto& pAttachment : pExt->ChildAttachments)
		{
			if (pAttachment->Child && pAttachment->GetType()->InheritCommands)
				ParentClickedCellAction(pAttachment->Child, action, pCell, pSecondCell);
		}
	}
}

void ParentAreaGuardAction(TechnoClass* pThis)
{
	pThis->ClickedMission(Mission::Area_Guard, reinterpret_cast<ObjectClass*>(pThis->GetCellAgain()), nullptr, nullptr);
	Unsorted::MoveFeedback = false;

	// Children handling
	if (const auto& pExt = TechnoExt::ExtMap.Find(pThis))
	{
		for (const auto& pAttachment : pExt->ChildAttachments)
		{
			if (pAttachment->Child && pAttachment->GetType()->InheritCommands)
				ParentAreaGuardAction(pAttachment->Child);
		}
	}
}

DEFINE_HOOK(0x4AE7B3, DisplayClass_ActiveClickWith_Iterate, 0x0)
{
	REF_STACK(int, idxPath, STACK_OFFSET(0x18, -0x8));
	REF_STACK(unsigned char, idxWP, STACK_OFFSET(0x18, -0xC));

	for (const auto& pObject : ObjectClass::CurrentObjects)
	{
		if (const auto pTechno = abstract_cast<TechnoClass*>(pObject))
			ParentClickedWaypoint(pTechno, idxPath, idxWP);
	}

	GET_STACK(ObjectClass* const, pTarget, STACK_OFFSET(0x18, +0x4));
	LEA_STACK(CellStruct* const, pCell, STACK_OFFSET(0x18, +0x8));
	GET_STACK(Action const, action, STACK_OFFSET(0x18, +0xC));

	if (pTarget)
	{
		const auto count = ObjectClass::CurrentObjects.Count;

		if (count > 0)
		{
			const auto mode1 = Phobos::Config::DistributionSpreadMode;
			const auto mode2 = Phobos::Config::DistributionFilterMode;

			// Distribution mode main
			if (DistributionModeHoldDownCommandClass::Enabled
				&& mode1
				&& count > 1
				&& action != Action::NoMove
				&& !PlanningNodeClass::PlanningModeActive
				&& (pTarget->AbstractFlags & AbstractFlags::Techno) != AbstractFlags::None
				&& !pTarget->IsInAir())
			{
				VocClass::PlayGlobal(RulesExt::Global()->AddDistributionModeCommandSound, 0x2000, 1.0);

				const auto pTargetHouse = static_cast<TechnoClass*>(pTarget)->Owner;
				const bool targetIsNeutral = pTargetHouse->IsNeutral();

				const auto range = (2 << mode1);
				const auto center = pTarget->GetCoords();
				const auto pItems = Helpers::Alex::getCellSpreadItems(center, range);

				std::vector<std::pair<TechnoClass*, int>> record;
				const auto maxSize = pItems.size();
				record.reserve(maxSize);

				int current = 1;

				for (const auto& pItem : pItems)
				{
					if (pItem->IsDisguisedAs(HouseClass::CurrentPlayer))
						continue;

					if (pItem->CloakState == CloakState::Cloaked && !pItem->GetCell()->Sensors_InclHouse(HouseClass::CurrentPlayer->ArrayIndex))
						continue;

					auto coords = pItem->GetCoords();

					if (!MapClass::Instance.IsWithinUsableArea(coords))
						continue;

					coords.Z = MapClass::Instance.GetCellFloorHeight(coords);

					if (MapClass::Instance.GetCellAt(coords)->ContainsBridge())
						coords.Z += CellClass::BridgeHeight;

					if (!MapClass::Instance.IsLocationShrouded(coords))
						record.emplace_back(pItem, 0);
				}

				const auto recordSize = record.size();
				std::sort(&record[0], &record[recordSize],[&center](const auto& pairA, const auto& pairB)
				{
					const auto coordsA = pairA.first->GetCoords();
					const auto distanceA = Point2D{coordsA.X, coordsA.Y}.DistanceFromSquared(Point2D{center.X, center.Y});

					const auto coordsB = pairB.first->GetCoords();
					const auto distanceB = Point2D{coordsB.X, coordsB.Y}.DistanceFromSquared(Point2D{center.X, center.Y});

					return distanceA < distanceB;
				});

				for (const auto& pSelect : ObjectClass::CurrentObjects)
				{
					const auto pTechno = abstract_cast<TechnoClass*>(pSelect);

					if (!pTechno)
						continue;

					size_t canTargetIndex = maxSize;
					size_t newTargetIndex = maxSize;

					for (size_t i = 0; i < recordSize; ++i)
					{
						const auto& [pItem, num] = record[i];

						if (pTechno->MouseOverObject(pItem) != action)
							continue;

						if (!targetIsNeutral && pItem->Owner->IsNeutral())
							continue;

						if (mode2 < 2 || (pItem->WhatAmI() == pTarget->WhatAmI()
							&& (mode2 < 3 || TechnoTypeExt::GetSelectionGroupID(pItem->GetTechnoType())
								== TechnoTypeExt::GetSelectionGroupID(pTarget->GetTechnoType()))))
						{
							canTargetIndex = i;

							if (num < current)
							{
								newTargetIndex = i;
								break;
							}
						}
					}

					if (newTargetIndex == maxSize && canTargetIndex != maxSize)
					{
						++current;
						newTargetIndex = canTargetIndex;
					}

					if (newTargetIndex != maxSize)
					{
						auto& [pNewTarget, recordCount] = record[newTargetIndex];

						++recordCount;
						ParentClickedTargetAction(pTechno, action, pNewTarget);
					}
					else
					{
						const auto currentAction = pSelect->MouseOverObject(pTarget);

						if (mode2 && currentAction == Action::NoMove)
							ParentAreaGuardAction(pTechno);
						else
							ParentClickedTargetAction(pTechno, currentAction, pTarget);
					}
				}
			}
			else // Vanilla
			{
				for (const auto& pSelect : ObjectClass::CurrentObjects)
				{
					const auto pTechno = abstract_cast<TechnoClass*>(pSelect);

					if (!pTechno)
						continue;

					const auto currentAction = pTechno->MouseOverObject(pTarget);

					if (mode2 && action != Action::NoMove && currentAction == Action::NoMove)
						ParentAreaGuardAction(pTechno);
					else
						ParentClickedTargetAction(pTechno, currentAction, pTarget);
				}
			}
		}
	}
	else
	{
		auto invalidCell = CellStruct { -1, -1 };
		auto pSecondCell = &invalidCell;

		if (action == Action::Move || action == Action::PatrolWaypoint || action == Action::NoMove)
			pSecondCell = pCell;

		for (const auto& pObject : ObjectClass::CurrentObjects)
		{
			if (const auto pTechno = abstract_cast<TechnoClass*>(pObject))
				ParentClickedCellAction(pTechno, pTechno->MouseOverCell(pCell, false, false), pCell, pSecondCell);
		}
	}

	Unsorted::MoveFeedback = true;

	return 0x4AE99B;
}

namespace TechnoAttachmentTemp
{
	bool stopPressed = false;
	bool deployPressed = false;
}

DEFINE_HOOK(0x730EA0, StopCommand_Context_Set, 0x5)
{
	TechnoAttachmentTemp::stopPressed = true;
	return 0;
}

DEFINE_HOOK(0x730AF0, DeployCommand_Context_Set, 0x8)
{
	TechnoAttachmentTemp::deployPressed = true;
	return 0;
}

namespace TechnoAttachmentTemp
{
	TechnoClass* pParent = nullptr;
}

DEFINE_HOOK(0x6FFE00, TechnoClass_ClickedEvent_Context_Set, 0x5)
{
	TechnoAttachmentTemp::pParent = R->ECX<TechnoClass*>();
	return 0;
}

DEFINE_HOOK_AGAIN(0x6FFEB1, TechnoClass_ClickedEvent_HandleChildren, 0x6)
DEFINE_HOOK(0x6FFE4F, TechnoClass_ClickedEvent_HandleChildren, 0x6)
{
	if ((TechnoAttachmentTemp::stopPressed || TechnoAttachmentTemp::deployPressed) && TechnoAttachmentTemp::pParent)
	{
		if (auto const& pExt = TechnoExt::ExtMap.Find(TechnoAttachmentTemp::pParent))
		{
			for (auto const& pAttachment : pExt->ChildAttachments)
			{
				if (!pAttachment->Child)
					continue;

				if (TechnoAttachmentTemp::stopPressed && pAttachment->GetType()->InheritCommands_StopCommand)
					pAttachment->Child->ClickedEvent(EventType::Idle);

				if (TechnoAttachmentTemp::deployPressed && pAttachment->GetType()->InheritCommands_DeployCommand)
					pAttachment->Child->ClickedEvent(EventType::Deploy);
			}
		}
	}

	return 0;
}

DEFINE_HOOK(0x730F1C, StopCommand_Context_Unset, 0x5)
{
	TechnoAttachmentTemp::stopPressed = false;
	return 0;
}

DEFINE_HOOK(0x730D55, DeployCommand_Context_Unset, 0x7)
{
	TechnoAttachmentTemp::deployPressed = false;
	return 0;
}


#pragma endregion

DEFINE_HOOK(0x469672, BulletClass_Logics_Locomotor_CheckIfAttached, 0x6)
{
	enum { SkipInfliction = 0x469AA4, ContinueCheck = 0x0 };

	GET(FootClass*, pThis, EDI);

	return TechnoExt::ExtMap.Find(pThis)->ParentAttachment ? SkipInfliction : ContinueCheck;
}

DEFINE_HOOK(0x6FC3F4, TechnoClass_CanFire_HandleAttachmentLogics, 0x6)
{
	enum { ReturnFireErrorIllegal = 0x6FC86A, ContinueCheck = 0x0 };

	GET(TechnoClass*, pThis, ESI);
	GET(TechnoClass*, pTarget, EBP);
	GET(WeaponTypeClass*, pWeapon, EDI);

	return (pWeapon->Warhead && pWeapon->Warhead->IsLocomotor && TechnoExt::IsChildOf(pThis, pTarget)) ? ReturnFireErrorIllegal : ContinueCheck;
}

// TODO WhatWeaponShouldIUse

DEFINE_HOOK(0x6F3283, TechnoClass_CanScatter_CheckIfAttached, 0x8)
{
	enum { ReturnFalse = 0x6F32C5, ContinueCheck = 0x0 };

	GET(TechnoClass*, pThis, ECX);

	auto const& pExt = TechnoExt::ExtMap.Find(pThis);

	return pExt->ParentAttachment ? ReturnFalse : ContinueCheck;
}

DEFINE_HOOK(0x4817A8, CellClass_Incoming_CheckIfTechnoOccupies, 0x6)
{
	enum { ConditionIsTrue = 0x4817C3, ContinueCheck = 0x0 };

	GET(TechnoClass*, pTechno, ESI);

	auto const& pExt = TechnoExt::ExtMap.Find(pTechno);

	return pExt->ParentAttachment && pExt->ParentAttachment->GetType()->OccupiesCell ? ConditionIsTrue : ContinueCheck;
}

DEFINE_HOOK(0x4817C3, CellClass_Incoming_HandleScatterWithAttachments, 0x0)
{
	GET(TechnoClass*, pTechno, ESI);

	GET(CoordStruct*, pThreatCoord, EBP);
	GET(bool, isForced, EBX);
	GET_STACK(bool, isNoKidding, STACK_OFFSET(0x2C, 0xC));  // direct all complaints to tomsons26 for the variable naming

	// we already checked that this is something that occupies the cell, see the hook above - Kerbiter
	TechnoExt::GetTopLevelParent(pTechno)->Scatter(*pThreatCoord, isForced, isNoKidding);

	return 0x4817D9;
}

DEFINE_HOOK(0x51D0DD, InfantryClass_Scatter_CheckAttachments, 0x6)
{
	enum { Bail = 0x51D6E6, Continue = 0x0 };

	GET(InfantryClass*, pThis, ESI);

	return TechnoExt::HasAttachmentLoco(pThis) ? Bail : Continue;
}

// UpdateFiring
/*
DEFINE_HOOK(0x736FB6, UnitClass_FiringAI_ForbidAttachmentRotation, 0x6)
{
	enum { SkipBodyRotation = 0x737063, ContinueCheck = 0x0 };

	GET(UnitClass*, pThis, ESI);

	return TechnoExt::ExtMap.Find(pThis)->ParentAttachment ? SkipBodyRotation : ContinueCheck;
}
*/

DEFINE_HOOK(0x736A2F, UnitClass_RotationAI_ForbidAttachmentRotation, 0x7)
{
	enum { SkipBodyRotation = 0x736A8E, ContinueCheck = 0x0 };

	GET(UnitClass*, pThis, ESI);

	return TechnoExt::HasAttachmentLoco(pThis) && TechnoExt::ExtMap.Find(pThis)->ParentAttachment ? SkipBodyRotation : ContinueCheck;
}

Action __fastcall UnitClass_MouseOverCell_Wrapper(UnitClass* pThis, discard_t, CellStruct const* pCell, bool checkFog, bool ignoreForce)
{
	Action result = pThis->UnitClass::MouseOverCell(pCell, checkFog, ignoreForce);

	if (!TechnoExt::ExtMap.Find(pThis)->ParentAttachment)
		return result;

	switch (result)
	{
		case Action::GuardArea:
		case Action::AttackMoveNav:
		case Action::PatrolWaypoint:
		case Action::Harvest:
		case Action::Move:
			result = Action::NoMove;
			break;
		case Action::EnterTunnel:
			result = Action::NoEnterTunnel;
			break;
	}

	return result;
}

// MouseOverObject for entering bunkers, grinder, buildings etc
// is handled along with the shield logics in another file

DEFINE_FUNCTION_JUMP(VTABLE, 0x7F5CE0, UnitClass_MouseOverCell_Wrapper)

// YSort for attachments
int __fastcall TechnoClass_SortY_Wrapper(TechnoClass* pThis)
{
	if (const auto pParentAttachment = TechnoExt::ExtMap.Find(pThis)->ParentAttachment)
	{
		if (const auto pParentTechno = pParentAttachment->Parent)
		{
			const auto ySortPosition = pParentAttachment->GetType()->YSortPosition.Get();

			if (ySortPosition != AttachmentYSortPosition::Default)
				return pParentTechno->GetYSort() + (ySortPosition == AttachmentYSortPosition::OverParent ? 1 : -1);
		}
	}

	return pThis->ObjectClass::GetYSort();
}

DEFINE_FUNCTION_JUMP(CALL, 0x449413, TechnoClass_SortY_Wrapper)   // BuildingClass
DEFINE_FUNCTION_JUMP(VTABLE, 0x7E235C, TechnoClass_SortY_Wrapper) // AircraftClass
DEFINE_FUNCTION_JUMP(VTABLE, 0x7EB110, TechnoClass_SortY_Wrapper) // InfantryClass
DEFINE_FUNCTION_JUMP(VTABLE, 0x7F5D28, TechnoClass_SortY_Wrapper) // UnitClass

DEFINE_JUMP(LJMP, 0x568831, 0x568841); // Skip locomotion layer check in MapClass::PickUp
DEFINE_JUMP(LJMP, 0x4D37A2, 0x4D37AE); // Skip locomotion layer check in FootClass::Mark

DEFINE_HOOK(0x6DA3FF, TacticalClass_SelectAt_TransparentToMouse_TacticalSelectable, 0x6)
{
	enum { SkipTechno = 0x6DA440, ContinueCheck = 0x0 };

	GET(TechnoClass*, pTechno, EAX);

	auto const pExt = TechnoExt::ExtMap.Find(pTechno);

	return (pExt && pExt->ParentAttachment && pExt->ParentAttachment->GetType()->TransparentToMouse) ? SkipTechno : ContinueCheck;
}

// ShouldIgnoreByMouse
/*
DEFINE_HOOK(0x6DA4FB, TacticalClass_SelectAt_TransparentToMouse_OccupierPtr, 0x6)
{
	GET(CellClass*, pCell, EAX);

	ObjectClass* pFoundObject = nullptr;

	for (ObjectClass* pOccupier = pCell->FirstObject; pOccupier; pOccupier = pOccupier->NextObject)
	{
		// find first non-transparent to mouse techno and return it
		if (auto const pOccupierAsTechno = abstract_cast<TechnoClass*>(pOccupier))
		{
			auto const pExt = TechnoExt::ExtMap.Find(pOccupierAsTechno);
			if (pExt && pExt->ParentAttachment && pExt->ParentAttachment->GetType()->TransparentToMouse)
				continue;
		}

		pFoundObject = pOccupier;
		break;
	}

	R->EAX<ObjectClass*>(pFoundObject);
	return 0x6DA501;
}
*/

// this is probably not the best way to implement sight since we may be hijacking
// into some undesirable side effects, cause this is intended for air units that
// don't run Per Cell Process function, ergo, don't update their sight - Kerbiter
DEFINE_HOOK(0x4DA6A0, FootClass_AI_CheckLocoForSight, 0x0)
{
	enum { ContinueCheck = 0x4DA6AF, NoSightUpdate = 0x4DA7B0 };

	GET(FootClass*, pThis, ESI);

	return pThis->IsInAir() || TechnoExt::HasAttachmentLoco(pThis) ? ContinueCheck : NoSightUpdate;
}

DEFINE_HOOK(0x440951, BuildingClass_Unlimbo_AttachmentsFromUpgrade, 0x6)
{
	GET(BuildingClass*, pBuilding, EDI);
	GET(BuildingClass*, pUpgrade, ESI);

	TechnoExt::TransferAttachments(pUpgrade, pBuilding);

	return 0;
}
