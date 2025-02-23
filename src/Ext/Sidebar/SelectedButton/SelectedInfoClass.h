#pragma once
#include "SelectedColumnClass.h"
#include "SelectedCameoClass.h"
#include "SelectedButtonClass.h"

#include <Ext/Techno/Body.h>
#include <Ext/Sidebar/Body.h>

class SelectedInfoClass
{
public:
	static SelectedInfoClass Instance;

	static constexpr int StartID = 2300;

	void InitClear();
	void InitIO();

	void SwitchVisible();
	void UpdateVisible();
	void UpdateSelected();
	void AddToSelected(TechnoTypeExt::ExtData* pTypeExt, int count, int checkIndex);

	static BSurface* SearchMissingCameo(AbstractType absType, SHPStruct* pSHP);
	static void GetValuesForDisplay(TechnoClass* pThis, ObjectTypeClass* pFakeType, DisplayInfoType infoType, int& value, int& maxValue);

	inline int GetMaxCameo() const;
	void DrawInfo();

	struct SelectRecordStruct
	{
		TechnoTypeExt::ExtData* TypeExt { nullptr };
		int Count { 0 };
	};

	SelectedColumnClass* MainColumn { nullptr };

	SelectedButtonClass* PushButton { nullptr };
	SelectedButtonClass* AmmoButton { nullptr };

	SelectedMainCameoClass* MainCameo { nullptr };

	SelectedNotButtonClass* InfoIconA { nullptr };
	SelectedNotButtonClass* InfoIconD { nullptr };
	SelectedNotButtonClass* InfoIconS { nullptr };

	SelectedBottomClass* MainBottom { nullptr };

	SelectedCameoClass* Cameos[20] { };
	std::vector<SelectRecordStruct> CurrentSelectCameo { };
	int Hovering { -1 };
	int MaxCameo { 0 };

	bool ShouldUpdate { false };
	bool SingleSelect { true };
	bool IsHovering { false };
};
