#pragma once
#include "UniqueTechnoButtonClass.h"
#include <Ext/Sidebar/Body.h>

class UniqueTechnoColumnClass
{
public:
	static UniqueTechnoColumnClass Instance;

	void InitClear();
	void InitIO();

	UniqueTechnoButtonClass* Buttons[8] { };
	int Hovering { -1 };
	bool Visible { true };
};
