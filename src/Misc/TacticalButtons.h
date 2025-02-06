#pragma once

#include <Utilities/Macro.h>
#include <Ext/SWType/Body.h>
#include <ControlClass.h>

class TacticalButtonsClass
{
public:
	static TacticalButtonsClass Instance;

private:
	int CheckMouseOverButtons(const Point2D* pMousePosition);
	bool CheckMouseOverBackground(const Point2D* pMousePosition);

public:
	inline bool MouseIsOverButtons();
	inline bool MouseIsOverTactical();
	int GetButtonIndex();
	void SetMouseButtonIndex(const Point2D* pMousePosition);
	void PressDesignatedButton(int triggerIndex);

	// Button index N/A : Message Lists
	bool MouseIsOverMessageLists(const Point2D* pMousePosition);

	// Button index N/A : FPS Counter
	void FPSCounterDraw();

	// Button index N/A : Show Current Info
	void CurrentSelectPathDraw();
	void CurrentSelectInfoDraw();

	// Button index 1-8 : Heros buttons
	inline bool IndexInHerosButtons();
	BSurface* GetMissingCameo(SHPStruct* pSHP);
	void HerosDraw();
	void HeroSelect(int buttonIndex);
	void HeroSwitch();

	// Button index 11-100 : Select buttons
	inline bool IndexInSelectButtons();
	inline void AddToCurrentSelect(TechnoTypeExt::ExtData* pTypeExt, int count, int checkIndex);
	BSurface* SearchMissingCameo(AbstractType absType, SHPStruct* pSHP);
	void SelectedTrigger(int buttonIndex, bool select);
	void SelectedUpdate();
	void SelectedDraw();
	void SelectedSwitch();

	struct SelectRecordStruct
	{
		TechnoTypeExt::ExtData* TypeExt { nullptr };
		int Count { 0 };
	};

	// TODO New buttons

public:
	bool PressedInButtonsLayer { false }; // Check press

	// Button index N/A : Message Lists
	bool OnMessages { false };

	// Button index N/A : FPS Counter

	// Button index N/A : Show Current Info

	// Button index 1-8 : Heros buttons
	bool HeroVisible { true };
	const wchar_t* HoveredHero { nullptr };

	// Button index 11-100 : Select buttons
	bool UpdateSelect { false };
	int RecordIndex { 11 };
	std::vector<SelectRecordStruct> CurrentSelectCameo {};
	const wchar_t* HoveredSelected { nullptr };

	// TODO New buttons

private:
	int ButtonIndex { -1 }; // -1 -> above no buttons, 0 -> above buttons background, POSITIVE -> above button who have this index
};
