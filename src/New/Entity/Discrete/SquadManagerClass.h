#pragma once

#include <memory>
#include <vector>

#include <TechnoClass.h>

#include <Utilities/EnumerableEntity.h>
#include <Utilities/TemplateDef.h>

class SquadManagerClass final : public EnumerableEntity<SquadManagerClass>
{
public:
	std::vector<TechnoClass*> Members;
	bool Selecting;

	SquadManagerClass() : EnumerableEntity<SquadManagerClass>()
		, Members { }
		, Selecting { false }
	{ };

	~SquadManagerClass() { };

	void AddMember(TechnoClass* const pTechno);
	void RemoveMember(TechnoClass* const pTechno);

	bool LoadFromStream(PhobosStreamReader& Stm);
	bool SaveToStream(PhobosStreamWriter& Stm);

private:
	template <typename T>
	bool Serialize(T& Stm);
};
