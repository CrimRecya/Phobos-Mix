#include "Body.h"

#include <Ext/Anim/Body.h>
#include <Ext/House/Body.h>
#include <Ext/Scenario/Body.h>
#include <Misc/FlyingStrings.h>
#include <Utilities/AresFunctions.h>

void FootExt::UpdateTiberiumEater()
{
	const auto pEaterType = this->TypeExtData->TiberiumEaterType.get();

	if (!pEaterType)
		return;

	const int transDelay = pEaterType->TransDelay;

	if (transDelay && this->TiberiumEater_Timer.InProgress())
		return;

	const auto pThis = this->OwnerObject();
	const auto pOwner = pThis->Owner;
	bool active = false;
	const bool displayCash = pEaterType->Display && pThis->IsClearlyVisibleTo(HouseClass::CurrentPlayer);
	int facing = pThis->PrimaryFacing.Current().GetFacing<8>();

	if (facing >= 7)
		facing = 0;
	else
		facing++;

	const int cellCount = static_cast<int>(pEaterType->Cells.size());
	const int locationZ = pThis->Location.Z;
	const int numOrePurifiers = pOwner->NumOrePurifiers;
	const float cashMultiplier = pEaterType->CashMultiplier;
	const float purifierBonus = RulesClass::Instance->PurifierBonus;
	const bool animMove = pEaterType->AnimMove;
	const auto displayToHouse = pEaterType->DisplayToHouse;
	const auto amountPerCell = pEaterType->AmountPerCell;
	const auto displayOffset = pEaterType->DisplayOffset;
	const auto& animsAll = pEaterType->Anims;
	auto* scenarioRandom = &ScenarioClass::Instance->Random;

	for (int idx = 0; idx < cellCount; idx++)
	{
		const auto& cellOffset = pEaterType->Cells[idx];
		const auto pos = TechnoExt::GetFLHAbsoluteCoords(pThis, CoordStruct { cellOffset.X, cellOffset.Y, 0 }, false);
		const auto pCell = MapClass::Instance.TryGetCellAt(pos);

		if (!pCell)
			continue;

		if (const int contained = pCell->GetContainedTiberiumValue())
		{
			const int tiberiumIdx = pCell->GetContainedTiberiumIndex();
			const int tiberiumValue = TiberiumClass::Array[tiberiumIdx]->Value;
			const int tiberiumAmount = static_cast<int>(static_cast<double>(contained) / tiberiumValue);
			const int amount = amountPerCell > 0 ? std::min(amountPerCell.Get(), tiberiumAmount) : tiberiumAmount;
			pCell->ReduceTiberium(amount);
			const float multiplier = cashMultiplier * (1.0f + numOrePurifiers * purifierBonus);
			const int value = static_cast<int>(std::round(amount * tiberiumValue * multiplier));
			pOwner->TransactMoney(value);
			active = true;

			if (displayCash)
			{
				auto cellCoords = pCell->GetCoords();
				cellCoords.Z = std::max(locationZ, cellCoords.Z);
				FlyingStrings::AddMoneyString(value, pThis, pOwner, displayToHouse, cellCoords, displayOffset);
			}

			const auto& anims = pEaterType->Anims_Tiberiums[tiberiumIdx].GetElements(animsAll);
			const int animCount = static_cast<int>(anims.size());

			if (animCount == 0)
				continue;

			AnimTypeClass* pAnimType = nullptr;

			switch (animCount)
			{
			case 1:
				pAnimType = anims[0];
				break;

			case 8:
				pAnimType = anims[facing];
				break;

			default:
				pAnimType = anims[scenarioRandom->RandomRanged(0, animCount - 1)];
				break;
			}

			if (pAnimType)
			{
				const auto pAnim = GameCreate<AnimClass>(pAnimType, pos);
				AnimExt::SetAnimOwnerHouseKind(pAnim, pThis->Owner, nullptr, false, true);

				if (animMove)
					pAnim->SetOwnerObject(pThis);
			}
		}
	}

	if (active && transDelay)
		this->TiberiumEater_Timer.Start(pEaterType->TransDelay);
}

void FootExt::UpdateOnTunnelEnter()
{
	if (!this->IsInTunnel)
	{
		if (const auto pShieldData = this->Shield.get())
			pShieldData->SetAnimationVisibility(false);

		for (const auto& pTrail : this->LaserTrails)
		{
			pTrail->Visible = false;
			pTrail->LastLocation = { };
		}

		this->IsInTunnel = true;
	}
}

void FootExt::UpdateOnTunnelExit()
{
	this->IsInTunnel = false;

	if (const auto pShieldData = this->Shield.get())
		pShieldData->SetAnimationVisibility(true);
}

void FootExt::UpdateWarpInDelay()
{
	if (this->HasRemainingWarpInDelay)
	{
		if (this->LastWarpInDelay)
		{
			this->LastWarpInDelay--;
		}
		else
		{
			this->HasRemainingWarpInDelay = false;
			this->IsBeingChronoSphered = false;
			this->OwnerObject()->WarpingOut = false;
		}
	}
}

// =============================
// load / save

template <typename T>
void FootExt::Serialize(T& Stm)
{
	Stm
		.Process(this->LastKillWasTeamTarget)
		.Process(this->LastWarpDistance)
		.Process(this->JumpjetSpeed)
		.Process(this->IsInTunnel)
		.Process(this->OriginalPassengerOwner)
		.Process(this->HasRemainingWarpInDelay)
		.Process(this->LastWarpInDelay)
		.Process(this->IsBeingChronoSphered)
		.Process(this->LastSensorsMapCoords)
		.Process(this->TiberiumEater_Timer)
		.Process(this->ResetLocomotor)
		.Process(this->JumpjetStraightAscend)
		.Process(this->AttackMoveFollowerTempCount)
		//.Process(this->IsOwnerChangeFromRevertOnExit) Temporary flag, does not need to be serialized.
		;
}

void FootExt::LoadFromStream(PhobosStreamReader& Stm)
{
	TechnoExt::LoadFromStream(Stm);
	this->Serialize(Stm);
}

void FootExt::SaveToStream(PhobosStreamWriter& Stm)
{
	TechnoExt::SaveToStream(Stm);
	this->Serialize(Stm);
}
