#pragma once

#include <TechnoClass.h>

#include <Utilities/EnumerableEntity.h>

class SquadManagerClass final : public EnumerableEntity<SquadManagerClass>
{
public:
	std::vector<TechnoClass*> Members;
	bool Selecting;

	SquadManagerClass() : EnumerableEntity<SquadManagerClass>()
		, Members { }
		, Selecting { false }
	{ };

	void AddMember(TechnoClass* const pTechno);
	void RemoveMember(TechnoClass* const pTechno);

	bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	bool Save(PhobosStreamWriter& Stm) const;

private:
	template <typename T>
	bool Serialize(T& Stm);
};
