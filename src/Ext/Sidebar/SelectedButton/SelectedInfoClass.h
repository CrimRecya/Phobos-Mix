#pragma once
#include "SelectedColumnClass.h"
#include "SelectedCameoClass.h"
#include "SelectedButtonClass.h"

#include <Ext/Techno/Body.h>
#include <Ext/Sidebar/Body.h>

enum class TechnoStatus : int
{
	Sleep = 0,
	Attack = 1,
	Move = 2,
	QMove = 3,
	Retreat = 4,
	Guard = 5,
	Sticky = 6,
	Enter = 7,
	Capture = 8,
	Eaten = 9,
	Harvest = 10,
	AreaGuard = 11,
	Return = 12,
	Stop = 13,
	Ambush = 14,
	Hunt = 15,
	Unload = 16,
	Sabotage = 17,
	Construction = 18,
	Selling = 19,
	Repair = 20,
	Rescue = 21,
	Missile = 22,
	Harmless = 23,
	Open = 24,
	Patrol = 25,
	Paradrop = 26,
	AttackMove = 27,
	Wait = 28,
	Produce = 29,
	Deactive = 30,
	Locomotor = 31,
	FollowGuard = 32,
	Unknown = 33,
	None = 34,
};

class SelectedInfoClass
{
public:
	static SelectedInfoClass Instance;

	static constexpr int StartID = 2300;
	static constexpr const wchar_t* Status[35] =
	{
		L"Sleep", L"Attack", L"Move", L"QueueMove", L"Retreat",
		L"Guard", L"Sticky", L"Enter", L"Capture", L"Eaten",
		L"Harvest", L"AreaGuard", L"Return", L"Stop", L"Ambush",
		L"Hunt", L"Unload", L"Sabotage", L"Construction", L"Selling",
		L"Repair", L"Rescue", L"Missile", L"Harmless", L"Open",
		L"Patrol", L"Paradrop", L"AttackMove", L"Wait", L"Produce",
		L"Deactive", L"Locomotor", L"FollowGuard", L"Unknown", L"None"
	};
	static constexpr const char* StatusEntry[35] =
	{
		"Status:Sleep", "Status:Attack", "Status:Move", "Status:QueueMove", "Status:Retreat",
		"Status:Guard", "Status:Sticky", "Status:Enter", "Status:Capture", "Status:Eaten",
		"Status:Harvest", "Status:AreaGuard", "Status:Return", "Status:Stop", "Status:Ambush",
		"Status:Hunt", "Status:Unload", "Status:Sabotage", "Status:Construction", "Status:Selling",
		"Status:Repair", "Status:Rescue", "Status:Missile", "Status:Harmless", "Status:Open",
		"Status:Patrol", "Status:Paradrop", "Status:AttackMove", "Status:Wait", "Status:Produce",
		"Status:Deactive", "Status:Locomotor", "Status:FollowGuard", "Status:Unknown", "Status:None"
	};

	void InitClear();
	void InitIO();

	void SwitchExpand();
	void SwitchVisible();
	void UpdateVisible();
	void UpdateSelected();
	void DrawInfo();

	static BSurface* SearchMissingCameo(AbstractType absType, SHPStruct* pSHP);
	static void GetValuesForDisplay(TechnoClass* pThis, ObjectTypeClass* pFakeType, DisplayInfoType infoType, int& value, int& maxValue);
	static TechnoStatus GetCurrentStatus(TechnoClass* pThis);

	int GetMaxCameo() const;
	bool CanScrollLeft() const;
	bool CanScrollRight() const;
	void ScrollLeft();
	void ScrollRight();

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
	std::vector<TechnoExt::ExtData*> CurrentSelectTechno { };
	int MaxCameo { 0 };
	int Current { 0 };

	bool ShouldUpdate { false };
	bool SingleSelect { true };
	bool IsHovering { false };
};
