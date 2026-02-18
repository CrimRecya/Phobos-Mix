#pragma once

#include <CellClass.h>

#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>

class CellExt
{
public:
	using base_type = CellClass;

	static constexpr DWORD Canary = 0x13371337;
	static constexpr size_t ExtPointerOffset = 0x144;

	struct RadLevel
	{
		RadSiteClass* Rad { nullptr };
		int Level { 0 };

		RadLevel() = default;
		RadLevel(RadSiteClass* pRad, int level) : Rad(pRad), Level(level)
		{ }

		bool Load(PhobosStreamReader& stm, bool registerForChange);
		bool Save(PhobosStreamWriter& stm) const;

	private:
		template <typename T>
		bool Serialize(T& stm);
	};

	class ExtData final : public Extension<CellClass>
	{
	public:
		std::vector<RadSiteClass*> RadSites;
		std::vector<RadLevel> RadLevels;

		int InfantryCount;

		UnitClass* IncomingUnit;
		UnitClass* IncomingUnitAlt;

		int SmudgeGenerate;
		BlitterFlags SmudgeState;

		ExtData(CellClass* OwnerObject) : Extension<CellClass>(OwnerObject)
			, RadSites {}
			, RadLevels {}
			, InfantryCount { 0 }
			, IncomingUnit { nullptr }
			, IncomingUnitAlt { nullptr }
			, SmudgeGenerate { 0 }
			, SmudgeState { BlitterFlags::None }
		{ }

		virtual ~ExtData() = default;

		virtual void InvalidatePointer(void* ptr, bool bRemoved) override {} // Useless
		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;
		virtual void Initialize() override;

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<CellExt>
	{
	public:
		ExtContainer();
		~ExtContainer();
	};

	static ExtContainer ExtMap;
};
