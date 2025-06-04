#pragma once

#include "Savegame.h"
#include "Constructs.h"

#include <algorithm>
#include <vector>

#include "Swizzle.h"

template <typename T>
class EnumerableEntity
{
public:
	inline static std::vector<std::unique_ptr<T>> Array;

	static T* Allocate()
	{
		Array.emplace_back(std::make_unique<T>());
		return Array.back().get();
	}

	static void Remove(T* item)
	{
		Array.erase(std::remove_if(Array.begin(), Array.end(), [&item](const auto& it){ return item == it.get(); }), Array.end());
	}

	static void Clear()
	{
		Array.clear();
	}

	static bool LoadGlobals(PhobosStreamReader& Stm)
	{
		Clear();

		size_t Count = 0;
		if (!Stm.Load(Count))
			return false;

		Array.reserve(Count);
		for (size_t i = 0; i < Count; ++i)
		{
			void* oldPtr = nullptr;

			if (!Stm.Load(oldPtr))
				return false;

			auto newPtr = Allocate();
			PhobosSwizzle::RegisterChange(oldPtr, newPtr);

			newPtr->LoadFromStream(Stm);
		}

		return true;
	}

	static bool SaveGlobals(PhobosStreamWriter& Stm)
	{
		Stm.Save(Array.size());

		for (auto const& item : Array)
		{
			// write old pointer and name, then delegate
			Stm.Save(item.get());
			item->SaveToStream(Stm);
		}

		return true;
	}
/*
	static void PointerGotInvalid(void* ptr, bool removed)
	{
		for (const auto& i : Array)
			i->InvalidatePointer(ptr, removed);
	}
	virtual void InvalidatePointer(void* ptr, bool removed) = 0;
*/
public:
	EnumerableEntity()
	{ }

	void LoadFromStream(PhobosStreamReader& Stm) = delete;

	void SaveToStream(PhobosStreamWriter& Stm) = delete;
};
