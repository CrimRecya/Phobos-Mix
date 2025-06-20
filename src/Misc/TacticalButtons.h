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

	// Button index N/A : Show Current Info
	void CurrentSelectInfoDraw();

	// TODO New buttons

public:
	bool PressedInButtonsLayer { false }; // Check press

	// Button index N/A : Message Lists
	bool OnMessages { false };
	bool NewMsgList { false };

	// Button index N/A : Show Current Info

	// TODO New buttons

private:
	int ButtonIndex { -1 }; // -1 -> above no buttons, 0 -> above buttons background, POSITIVE -> above button who have this index
};
