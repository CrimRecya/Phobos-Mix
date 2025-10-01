#include <New/Entity/AttachmentClass.h>

#include <Ext/Scenario/Body.h>
#include <Utilities/Macro.h>

static bool __forceinline ShouldRemoveSmudgeCell(const int index, const int time, const int current)
{
	const auto cell = CellStruct{static_cast<short>(index & 511), static_cast<short>(index >> 9) };

	if (const auto pCell = MapClass::Instance.TryGetCellAt(cell))
	{
		if (pCell->SmudgeTypeIndex != -1)
		{
			const auto pCellExt = CellExt::ExtMap.Find(pCell);

			if ((pCellExt->SmudgeGenerate + time) > current)
				return false;

			const auto state = pCellExt->SmudgeState;

			if (state != BlitterFlags::TransLucent75)
			{
				pCellExt->SmudgeGenerate = current;
				pCellExt->SmudgeState = static_cast<BlitterFlags>(static_cast<size_t>(state) + 2u);
				pCell->MarkForRedraw();
				return false;
			}

			pCell->SmudgeTypeIndex = -1;
			pCell->MarkForRedraw();
		}
	}

	return true;
}

DEFINE_HOOK(0x55B6B3, LogicClass_AI_After, 0x5)
{
	for (auto const& attachment : AttachmentClass::Array)
		attachment->AI();

	const int time = RulesExt::Global()->SmudgeUpdateTime;

	if (time > 0)
	{
		auto& s = ScenarioExt::Global()->Smudges;

		if (!s.empty())
		{
			const int current = Unsorted::CurrentFrame;

			for (auto it = s.begin(); it != s.end(); )
			{
				if (ShouldRemoveSmudgeCell(*it, time, current))
					it = s.erase(it);
				else
					++it;
			}
		}
	}

	return 0;
}
