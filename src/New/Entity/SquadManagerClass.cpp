#include "SquadManagerClass.h"

void SquadManagerClass::AddMember(TechnoClass* pTechno)
{
	this->Members.emplace_back(pTechno);
}

void SquadManagerClass::RemoveMember(TechnoClass* pTechno)
{
	this->Members.erase(std::remove(this->Members.begin(), this->Members.end(), pTechno), this->Members.end());
}

template <typename T>
bool SquadManagerClass::Serialize(T& Stm)
{
	return Stm
		.Process(this->Members)
		.Process(this->Selecting)
		.Success();
};

bool SquadManagerClass::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	return this->Serialize(Stm);
}

bool SquadManagerClass::Save(PhobosStreamWriter& Stm) const
{
	return const_cast<SquadManagerClass*>(this)->Serialize(Stm);
}
