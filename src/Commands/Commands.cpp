#include "Commands.h"

#include "ObjectInfo.h"
#include "NextIdleHarvester.h"
#include "QuickSave.h"
#include "DamageDisplay.h"
#include "FrameByFrame.h"
#include "FrameStep.h"
#include "ToggleDigitalDisplay.h"
#include "ToggleDesignatorRange.h"
#include "SaveVariablesToFile.h"
#include "AssignRallyPoint.h"
#include "SelectCaptured.h"
#include "SelectedInfo.h"
#include "HerosInfo.h"
#include "AutoBuilding.h"
#include "DistributionMode.h"
#include "ShowCurrentInfo.h"
#include "ManualReloadAmmo.h"
#include "ToggleSWSidebar.h"
#include "FireTacticalSW.h"
#include "AggressiveStance.h"
#include "CeaseFireStance.h"
#include "UnifiedTechnoColor.h"
#include "ToggleMessageList.h"

#include <CCINIClass.h>
#include <InputManagerClass.h>
#include <MouseClass.h>
#include <WWMouseClass.h>
#include <ShapeButtonClass.h>

#include <Utilities/Macro.h>
#include <Ext/Sidebar/SWSidebar/SWSidebarClass.h>
#include <Ext/Sidebar/SelectedButton/SelectedInfoClass.h>
#include <Misc/MessageColumn.h>

#pragma region HotkeyCommand

DEFINE_HOOK(0x533066, CommandClassCallback_Register, 0x6)
{
	// Load it after Ares'

	MakeCommand<NextIdleHarvesterCommandClass>();
	MakeCommand<QuickSaveCommandClass>();
	MakeCommand<ToggleDigitalDisplayCommandClass>();
	MakeCommand<ToggleDesignatorRangeCommandClass>();
	MakeCommand<SelectCapturedCommandClass>();
	MakeCommand<SelectedInfoCommandClass>();
	MakeCommand<SelectedExpandCommandClass>();
	MakeCommand<HerosInfoCommandClass>();
	MakeCommand<AssignRallyPointCommandClass>();
	MakeCommand<AssignSecondaryRallyPointCommandClass>();
	MakeCommand<AutoBuildingCommandClass>();
	MakeCommand<AutoBuildingCombatCommandClass>();
	MakeCommand<UnifiedTechnoColorCommandClass>();
	MakeCommand<ManualReloadAmmoCommandClass>();
	MakeCommand<AggressiveStanceClass>();
	MakeCommand<CeaseFireStanceClass>();
	MakeCommand<ToggleMessageListCommandClass>();
	MakeCommand<ToggleSWSidebar>();

	if (Phobos::Config::SuperWeaponSidebarCommands)
	{
		SWSidebarClass::Commands[0] = MakeCommand<FireTacticalSWCommandClass<0>>();
		SWSidebarClass::Commands[1] = MakeCommand<FireTacticalSWCommandClass<1>>();
		SWSidebarClass::Commands[2] = MakeCommand<FireTacticalSWCommandClass<2>>();
		SWSidebarClass::Commands[3] = MakeCommand<FireTacticalSWCommandClass<3>>();
		SWSidebarClass::Commands[4] = MakeCommand<FireTacticalSWCommandClass<4>>();
		SWSidebarClass::Commands[5] = MakeCommand<FireTacticalSWCommandClass<5>>();
		SWSidebarClass::Commands[6] = MakeCommand<FireTacticalSWCommandClass<6>>();
		SWSidebarClass::Commands[7] = MakeCommand<FireTacticalSWCommandClass<7>>();
		SWSidebarClass::Commands[8] = MakeCommand<FireTacticalSWCommandClass<8>>();
		SWSidebarClass::Commands[9] = MakeCommand<FireTacticalSWCommandClass<9>>();
	}

	if (Phobos::Config::AllowSwitchNoMoveCommand)
		MakeCommand<SwitchNoMoveCommandClass>();

	if (Phobos::Config::AllowDistributionCommand)
	{
		if (Phobos::Config::AllowDistributionCommand_SpreadMode)
			MakeCommand<DistributionModeSpreadCommandClass>();

		if (Phobos::Config::AllowDistributionCommand_FilterMode)
			MakeCommand<DistributionModeFilterCommandClass>();

		MakeCommand<DistributionModeHoldDownCommandClass>();
	}

	if (Phobos::Config::DevelopmentCommands)
	{
		MakeCommand<DamageDisplayCommandClass>();
		MakeCommand<SaveVariablesToFileCommandClass>();
		MakeCommand<ObjectInfoCommandClass>();
		MakeCommand<FrameByFrameCommandClass>();
		MakeCommand<FrameStepCommandClass<1>>(); // Single step in
		MakeCommand<FrameStepCommandClass<5>>(); // Speed 1
		MakeCommand<FrameStepCommandClass<10>>(); // Speed 2
		MakeCommand<FrameStepCommandClass<15>>(); // Speed 3
		MakeCommand<FrameStepCommandClass<30>>(); // Speed 4
		MakeCommand<FrameStepCommandClass<60>>(); // Speed 5
	}

	if (Phobos::Config::DebugToolEnable)
		MakeCommand<ShowCurrentInfoCommandClass>();

	return 0;
}

#pragma endregion

#pragma region MouseScroll

static void MouseWheelDownCommand()
{
	if (DistributionModeHoldDownCommandClass::Enabled && Phobos::Config::AllowDistributionCommand_SpreadModeScroll)
		DistributionModeHoldDownCommandClass::DistributionSpreadModeReduce();

	if (SelectedInfoClass::Instance.IsHovering)
		SelectedInfoClass::Instance.ScrollRight();

	if (MessageColumnClass::Instance.IsHovering())
		MessageColumnClass::Instance.ScrollDown();
}

static void MouseWheelUpCommand()
{
	if (DistributionModeHoldDownCommandClass::Enabled && Phobos::Config::AllowDistributionCommand_SpreadModeScroll)
		DistributionModeHoldDownCommandClass::DistributionSpreadModeExpand();

	if (SelectedInfoClass::Instance.IsHovering)
		SelectedInfoClass::Instance.ScrollLeft();

	if (MessageColumnClass::Instance.IsHovering())
		MessageColumnClass::Instance.ScrollUp();
}

DEFINE_HOOK(0x777998, Game_WndProc_ScrollMouseWheel, 0x6)
{
	GET(const WPARAM, WParam, ECX);

	if (WParam & 0x80000000u)
		MouseWheelDownCommand();
	else
		MouseWheelUpCommand();

	return 0;
}

static inline bool CheckSkipScrollSidebar()
{
	return !Phobos::Config::ScrollSidebarStripWhenHoldAlt && InputManagerClass::Instance->IsForceMoveKeyPressed()
		|| !Phobos::Config::ScrollSidebarStripWhenHoldCtrl && InputManagerClass::Instance->IsForceFireKeyPressed()
		|| !Phobos::Config::ScrollSidebarStripWhenHoldShift && InputManagerClass::Instance->IsForceSelectKeyPressed()
		|| !Phobos::Config::ScrollSidebarStripInTactical && WWMouseClass::Instance->XY1.X < Make_Global<int>(0xB0CE30) // TacticalClass::view_bound.Width
		|| DistributionModeHoldDownCommandClass::Enabled
		|| SelectedInfoClass::Instance.IsHovering
		|| MessageColumnClass::Instance.IsHovering();
}

DEFINE_HOOK(0x533F50, Game_ScrollSidebar_Skip, 0x5)
{
	enum { SkipScrollSidebar = 0x533FC3 };
	return CheckSkipScrollSidebar() ? SkipScrollSidebar : 0;
}

#pragma endregion

#pragma region ShapeButton

int ShapeButtonHelper::NewButtonIndexes[ShapeButtonHelper::NewButtonCount] =
{
	-1, // DistributionMode
	-1, // ManualReload
	-1, // AggressiveStance
	-1  // CeaseFire
};

DEFINE_HOOK(0x6CFD08, ShapeButtonClass_FindIndex_FindNewButton, 0x5)
{
	enum { SetButtonIndex = 0x6CFD0D };

	GET(const char*, name, ECX);

	for (int i = 0; i < ShapeButtonHelper::NewButtonCount; ++i)
	{
		if (_strcmpi(name, ShapeButtonHelper::NewButtonNames[i]) == 0)
		{
			R->EAX(i + ShapeButtonHelper::OldButtonCount);
			return SetButtonIndex;
		}
	}

	return 0;
}

DEFINE_HOOK(0x6D0233, TabClass_Init_InitNewButtonIndex, 0x6)
{
	for (int i = 0; i < ShapeButtonHelper::NewButtonCount; ++i)
		ShapeButtonHelper::NewButtonIndexes[i] = ShapeButtonClass::FindIndex(ShapeButtonHelper::NewButtonNames[i]);

	return 0;
}

DEFINE_HOOK(0x6D0827, TabClass_Update_UpdateNewButton, 0x6)
{
	GET(const int, index, EAX);

	if (ShapeButtonHelper::NewButtonIndexes[0] == index)
	{
		if (ShapeButtonClass::GetButton(index)->IsOn)
			DistributionModeHoldDownCommandClass::DistributionModeOn();
		else
			DistributionModeHoldDownCommandClass::DistributionModeOff();
	}

	if (ShapeButtonHelper::NewButtonIndexes[1] == index)
		ManualReloadAmmoCommandClass::ManualReloadExecute();

	if (ShapeButtonHelper::NewButtonIndexes[2] == index)
		AggressiveStanceClass::AggressiveExecute();

	if (ShapeButtonHelper::NewButtonIndexes[3] == index)
		CeaseFireStanceClass::CeaseFireExecute();

	return 0;
}

DEFINE_HOOK(0x6D10DF, TabClass_InitButtonIO_InitNewHoldDownButton, 0x6)
{
	const int distributionModeButtonIndex = ShapeButtonHelper::NewButtonIndexes[0];

	if (distributionModeButtonIndex != -1)
	{
		if (const auto pButton = ShapeButtonClass::GetButton(distributionModeButtonIndex))
		{
			// Clicking the button is different from holding down the hotkey
			pButton->ToggleType = 1;
			pButton->UseFlash = true;
		}
	}

	return 0;
}

DEFINE_HOOK(0x6D14DD, TabClass_InitToolTip_InitNewButtonToolTip, 0x5)
{
	for (int i = 0; i < ShapeButtonHelper::NewButtonCount; ++i)
		ShapeButtonClass::SetToolTip(ShapeButtonClass::GetButton(ShapeButtonHelper::NewButtonIndexes[i]), ShapeButtonHelper::NewButtonTipNames[i]);

	return 0;
}

#pragma endregion
