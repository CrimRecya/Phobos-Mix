#pragma once
#include "UniqueTechnoButtonClass.h"
#include <Ext/Sidebar/Body.h>

class UniqueTechnoColumnClass
{
public:
	static UniqueTechnoColumnClass Instance;

	static constexpr int StartID = 2200;

	void InitClear();
	void InitIO();

	void SwitchVisible();

	UniqueTechnoButtonClass* Buttons[8] { };
	int Hovering { -1 };
	bool Visible { true };
};
