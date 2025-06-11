#include "Body.h"

#include <Utilities/AresHelper.h>
#include <Utilities/Macro.h>

enum class AttachCargoMode
{
	SingleObject,
	ObjectChain,

	DefaultBehavior = SingleObject,
};

namespace TechnoAttachmentTemp
{
	AttachCargoMode currentAttachMode = AttachCargoMode::DefaultBehavior;
}

#define DEFINE_ATTACH_WRAPPER(mode) \
void __fastcall CargoClass_Attach_##mode(PassengersClass* pThis, discard_t, FootClass* pThat) \
{ \
	TechnoAttachmentTemp::currentAttachMode = AttachCargoMode::mode; \
	pThis->AddPassenger(pThat); \
	TechnoAttachmentTemp::currentAttachMode = AttachCargoMode::DefaultBehavior; \
}

DEFINE_ATTACH_WRAPPER(SingleObject);
DEFINE_ATTACH_WRAPPER(ObjectChain);

DEFINE_FUNCTION_JUMP(CALL, 0x65DF88, CargoClass_Attach_ObjectChain);  // Create_Group
DEFINE_FUNCTION_JUMP(CALL, 0x65DCF0, CargoClass_Attach_ObjectChain);  // Do_Reinforcements, paradrop loading

DEFINE_HOOK(0x4733BD, CargoClass_Attach_HandleCurrentAttachMode, 0x6)
{
	enum { SkipAttachingChain = 0x4733FA, Continue = 0x0 };

	return TechnoAttachmentTemp::currentAttachMode == AttachCargoMode::SingleObject
		? SkipAttachingChain
		: Continue;
}

// Bugfix: TAction 7,80,107.
DEFINE_HOOK(0x65DF67, TeamTypeClass_CreateMembers_LoadOntoTransport, 0x6)
{
	if(!AresHelper::CanUseAres) // If you're not using Ares you don't deserve a fix
		return 0;

	GET(FootClass* const, pPayload, EAX);
	GET(FootClass* const, pTransport, ESI);
	GET(TeamClass* const, pTeam, EBP);
	GET(TeamTypeClass const*, pThis, EBX);

	auto unmarkPayloadCreated = [](FootClass* member){reinterpret_cast<char*>(member->align_154)[0x9E] = false;};

	if (!pTransport)
	{
		for (auto pNext = pPayload;
		pNext && pNext != pTransport && pNext->Team == pTeam;
		pNext = abstract_cast<FootClass*>(pNext->NextObject))
			unmarkPayloadCreated(pNext);

		return 0x65DFE8;
	}

	unmarkPayloadCreated(pTransport);

	if (!pPayload || !pThis->Full)
		return 0x65E004;

	const auto pType = pTransport->GetTechnoType();
	const bool isTransportOpenTopped = pType->OpenTopped;
	FootClass* pGunner = nullptr;

	for (auto pNext = pPayload;
		pNext && pNext != pTransport && pNext->Team == pTeam;
		pNext = abstract_cast<FootClass*>(pNext->NextObject))
	{
		pPayload->Transporter = pTransport;
		pGunner = pNext;

		if (isTransportOpenTopped)
			pTransport->EnteredOpenTopped(pNext);
	}

	// Add to transport - this will load the payload object and everything linked to it (rest of the team) in reverse order
	TechnoAttachmentTemp::currentAttachMode = AttachCargoMode::ObjectChain;
	pTransport->Passengers.AddPassenger(pPayload);
	TechnoAttachmentTemp::currentAttachMode = AttachCargoMode::DefaultBehavior;

	// Handle gunner change - this is the 'last' passenger because of reverse order
	if (pType->Gunner && pGunner)
		pTransport->ReceiveGunner(pGunner);

	return 0x65DF8D;
}
