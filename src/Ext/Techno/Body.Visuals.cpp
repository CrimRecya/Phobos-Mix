#include "Body.h"

#include <SessionClass.h>
#include <TacticalClass.h>
#include <SpawnManagerClass.h>
#include <FactoryClass.h>
#include <SuperClass.h>
#include <TacticalClass.h>
#include <DriveLocomotionClass.h>
#include <ShipLocomotionClass.h>
#include <JumpjetLocomotionClass.h>

#include <Ext/SWType/Body.h>
#include <Ext/House/Body.h>
#include <Utilities/EnumFunctions.h>

void TechnoExt::DrawSelfHealPips(TechnoClass* pThis, Point2D* pLocation, RectangleStruct* pBounds)
{
	if (!RulesExt::Global()->GainSelfHealAllowMultiplayPassive && pThis->Owner->Type->MultiplayPassive)
		return;

	auto const pType = pThis->GetTechnoType();
	auto const pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	if (pTypeExt->SelfHealGainType.isset() && pTypeExt->SelfHealGainType.Get() == SelfHealGainType::NoHeal)
		return;

	bool drawPip = false;
	bool isInfantryHeal = false;
	int selfHealFrames = 0;
	bool hasInfantrySelfHeal = pTypeExt->SelfHealGainType.isset() && pTypeExt->SelfHealGainType.Get() == SelfHealGainType::Infantry;
	bool hasUnitSelfHeal = pTypeExt->SelfHealGainType.isset() && pTypeExt->SelfHealGainType.Get() == SelfHealGainType::Units;
	auto const whatAmI = pThis->WhatAmI();
	const bool isOrganic = (whatAmI == AbstractType::Infantry || (pType->Organic && whatAmI == AbstractType::Unit));

	auto hasSelfHeal = [pThis](const bool infantryHeal)
		{
			auto const pOwner = pThis->Owner;

			auto haveHeal = [infantryHeal](HouseClass* pHouse)
				{
					return (infantryHeal ? pHouse->InfantrySelfHeal > 0 : pHouse->UnitsSelfHeal > 0);
				};

			if (haveHeal(pOwner))
				return true;

			const bool isCampaign = SessionClass::IsCampaign();
			const bool fromPlayer = RulesExt::Global()->GainSelfHealFromPlayerControl && isCampaign && (pOwner->IsHumanPlayer || pOwner->IsInPlayerControl);
			const bool fromAllies = RulesExt::Global()->GainSelfHealFromAllies;

			if (fromPlayer || fromAllies)
			{
				auto checkHouse = [fromPlayer, fromAllies, isCampaign, pOwner](HouseClass* pHouse)
					{
						if (pHouse == pOwner)
							return false;

						return (fromPlayer && (pHouse->IsHumanPlayer || pHouse->IsInPlayerControl)) // pHouse->IsControlledByCurrentPlayer()
							|| (fromAllies && (!isCampaign || (!pHouse->IsHumanPlayer && !pHouse->IsInPlayerControl)) && pHouse->IsAlliedWith(pOwner));
					};

				for (auto pHouse : HouseClass::Array)
				{
					if (checkHouse(pHouse) && haveHeal(pHouse))
						return true;
				}
			}

			return false;
		};

	if ((hasInfantrySelfHeal || (isOrganic && !hasUnitSelfHeal)) && hasSelfHeal(true))
	{
		drawPip = true;
		selfHealFrames = RulesClass::Instance->SelfHealInfantryFrames;
		isInfantryHeal = true;
	}
	else if ((hasUnitSelfHeal || (whatAmI == AbstractType::Unit && !isOrganic)) && hasSelfHeal(false))
	{
		drawPip = true;
		selfHealFrames = RulesClass::Instance->SelfHealUnitFrames;
	}

	if (drawPip)
	{
		Valueable<Point2D> pipFrames;
		bool isSelfHealFrame = false;
		int xOffset = 0;
		int yOffset = 0;

		if (Unsorted::CurrentFrame % selfHealFrames <= 5
			&& pThis->Health < pType->Strength)
		{
			isSelfHealFrame = true;
		}

		if (whatAmI == AbstractType::Unit || whatAmI == AbstractType::Aircraft)
		{
			auto& offset = RulesExt::Global()->Pips_SelfHeal_Units_Offset.Get();
			pipFrames = RulesExt::Global()->Pips_SelfHeal_Units;
			xOffset = offset.X;
			yOffset = offset.Y + pType->PixelSelectionBracketDelta;
		}
		else if (whatAmI == AbstractType::Infantry)
		{
			auto& offset = RulesExt::Global()->Pips_SelfHeal_Infantry_Offset.Get();
			pipFrames = RulesExt::Global()->Pips_SelfHeal_Infantry;
			xOffset = offset.X;
			yOffset = offset.Y + pType->PixelSelectionBracketDelta;
		}
		else
		{
			auto pBldType = static_cast<BuildingClass*>(pThis)->Type;
			int fHeight = pBldType->GetFoundationHeight(false);
			int yAdjust = -Unsorted::CellHeightInPixels / 2;

			auto& offset = RulesExt::Global()->Pips_SelfHeal_Buildings_Offset.Get();
			pipFrames = RulesExt::Global()->Pips_SelfHeal_Buildings;
			xOffset = offset.X + Unsorted::CellWidthInPixels / 2 * fHeight;
			yOffset = offset.Y + yAdjust * fHeight + pBldType->Height * yAdjust;
		}

		int pipFrame = isInfantryHeal ? pipFrames.Get().X : pipFrames.Get().Y;

		Point2D position = { pLocation->X + xOffset, pLocation->Y + yOffset };

		auto flags = BlitterFlags::bf_400 | BlitterFlags::Centered;

		if (isSelfHealFrame)
			flags = flags | BlitterFlags::Darken;

		DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, FileSystem::PIPS_SHP,
		pipFrame, &position, pBounds, flags, 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}
}

void TechnoExt::DrawInsignia(TechnoClass* pThis, Point2D* pLocation, RectangleStruct* pBounds)
{
	auto pTechnoType = pThis->GetTechnoType();
	auto pOwner = pThis->Owner;

	if (pThis->IsDisguised() && !pThis->IsClearlyVisibleTo(HouseClass::CurrentPlayer) && !(HouseClass::IsCurrentPlayerObserver()
		|| EnumFunctions::CanTargetHouse(RulesExt::Global()->DisguiseBlinkingVisibility, HouseClass::CurrentPlayer, pOwner)))
	{
		if (auto const pType = TechnoTypeExt::GetTechnoType(pThis->Disguise))
		{
			pTechnoType = pType;
			pOwner = pThis->DisguisedAsHouse;
		}
	}

	TechnoTypeExt::ExtData* pTechnoTypeExt = TechnoTypeExt::ExtMap.Find(pTechnoType);

	const bool isVisibleToPlayer = (pOwner && pOwner->IsAlliedWith(HouseClass::CurrentPlayer))
		|| HouseClass::IsCurrentPlayerObserver()
		|| pTechnoTypeExt->Insignia_ShowEnemy.Get(RulesExt::Global()->EnemyInsignia);

	if (!isVisibleToPlayer)
		return;

	Point2D offset = *pLocation;
	SHPStruct* pShapeFile = FileSystem::PIPS_SHP;
	int defaultFrameIndex = -1;
	bool isCustomInsignia = false;

	if (SHPStruct* pCustomShapeFile = pTechnoTypeExt->Insignia.Get(pThis))
	{
		pShapeFile = pCustomShapeFile;
		defaultFrameIndex = 0;
		isCustomInsignia = true;
	}

	VeterancyStruct* pVeterancy = &pThis->Veterancy;
	auto insigniaFrames = pTechnoTypeExt->InsigniaFrames.Get();
	int insigniaFrame = insigniaFrames.X;
	int frameIndex = pTechnoTypeExt->InsigniaFrame.Get(pThis);

	if (pTechnoType->Passengers > 0 && pTechnoTypeExt->Insignia_Passengers.size() > 0)
	{
		int passengersIndex = pTechnoTypeExt->Passengers_BySize ? pThis->Passengers.GetTotalSize() : pThis->Passengers.NumPassengers;
		passengersIndex = Math::min(passengersIndex, pTechnoType->Passengers);

		if (auto const pCustomShapeFile = pTechnoTypeExt->Insignia_Passengers[passengersIndex].Get(pThis))
		{
			pShapeFile = pCustomShapeFile;
			defaultFrameIndex = 0;
			isCustomInsignia = true;
		}

		const int frame = pTechnoTypeExt->InsigniaFrame_Passengers[passengersIndex].Get(pThis);

		if (frame != -1)
			frameIndex = frame;

		auto const& frames = pTechnoTypeExt->InsigniaFrames_Passengers[passengersIndex];

		if (frames != Vector3D<int>(-1, -1, -1))
			insigniaFrames = frames.Get();
	}

	if (pTechnoType->Gunner && pTechnoTypeExt->Insignia_Weapon.size() > 0)
	{
		const int weaponIndex = pThis->CurrentWeaponNumber;

		if (auto const pCustomShapeFile = pTechnoTypeExt->Insignia_Weapon[weaponIndex].Get(pThis))
		{
			pShapeFile = pCustomShapeFile;
			defaultFrameIndex = 0;
			isCustomInsignia = true;
		}

		const int frame = pTechnoTypeExt->InsigniaFrame_Weapon[weaponIndex].Get(pThis);

		if (frame != -1)
			frameIndex = frame;

		auto const& frames = pTechnoTypeExt->InsigniaFrames_Weapon[weaponIndex];

		if (frames != Vector3D<int>(-1, -1, -1))
			insigniaFrames = frames.Get();
	}

	if (pVeterancy->IsVeteran())
	{
		defaultFrameIndex = !isCustomInsignia ? 14 : defaultFrameIndex;
		insigniaFrame = insigniaFrames.Y;
	}
	else if (pVeterancy->IsElite())
	{
		defaultFrameIndex = !isCustomInsignia ? 15 : defaultFrameIndex;
		insigniaFrame = insigniaFrames.Z;
	}

	frameIndex = frameIndex == -1 ? insigniaFrame : frameIndex;

	if (frameIndex == -1)
		frameIndex = defaultFrameIndex;

	if (frameIndex != -1 && pShapeFile)
	{
		switch (pThis->WhatAmI())
		{
		case AbstractType::Infantry:
			offset += RulesExt::Global()->DrawInsignia_AdjustPos_Infantry;
			break;
		case AbstractType::Building:
			if (RulesExt::Global()->DrawInsignia_AdjustPos_BuildingsAnchor.isset())
				offset = GetBuildingSelectBracketPosition(pThis, RulesExt::Global()->DrawInsignia_AdjustPos_BuildingsAnchor) + RulesExt::Global()->DrawInsignia_AdjustPos_Buildings;
			else
				offset += RulesExt::Global()->DrawInsignia_AdjustPos_Buildings;
			break;
		default:
			offset += RulesExt::Global()->DrawInsignia_AdjustPos_Units;
			break;
		}

		offset.Y += RulesExt::Global()->DrawInsignia_UsePixelSelectionBracketDelta ? pTechnoType->PixelSelectionBracketDelta : 0;

		DSurface::Temp->DrawSHP(
			FileSystem::PALETTE_PAL, pShapeFile, frameIndex, &offset, pBounds, BlitterFlags(0xE00), 0, -2, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}

	return;
}

void TechnoExt::DrawFactoryProgress(BuildingClass* pThis, RectangleStruct* pBounds, Point2D basePosition)
{
	const RulesExt::ExtData* const pRulesExt = RulesExt::Global();

	if (!pRulesExt->FactoryProgressDisplay)
		return;

	const auto pType = pThis->Type;
	const auto pHouse = pThis->Owner;
	FactoryClass* pPrimaryFactory = nullptr;
	FactoryClass* pSecondaryFactory = nullptr;

	if (pHouse->IsControlledByHuman())
	{
		if (!pThis->IsPrimaryFactory)
			return;

		switch (pType->Factory)
		{
		case AbstractType::BuildingType:
			pPrimaryFactory = pHouse->Primary_ForBuildings;
			pSecondaryFactory = pHouse->Primary_ForDefenses;
			break;
		case AbstractType::InfantryType:
			pPrimaryFactory = pHouse->Primary_ForInfantry;
			break;
		case AbstractType::UnitType:
			pPrimaryFactory = pType->Naval ? pHouse->Primary_ForShips : pHouse->Primary_ForVehicles;
			break;
		case AbstractType::AircraftType:
			pPrimaryFactory = pHouse->Primary_ForAircraft;
			break;
		default:
			return;
		}
	}
	else // AIs have no Primary factories
	{
		pPrimaryFactory = pThis->Factory;

		if (!pPrimaryFactory)
			return;
	}

	const bool havePrimary = pPrimaryFactory && pPrimaryFactory->Object;
	const bool haveSecondary = pSecondaryFactory && pSecondaryFactory->Object;

	if (!havePrimary && !haveSecondary)
		return;

	const auto maxLength = pType->GetFoundationHeight(false) * 15 >> 1;
	const auto location = basePosition + Point2D { 6, (3 + pType->PixelSelectionBracketDelta) } + pRulesExt->FactoryProgressDisplay_Offset.Get();

	if (havePrimary)
	{
		auto position = location;

		DrawFrameStruct pDraw
		{
			static_cast<int>((static_cast<double>(pPrimaryFactory->GetProgress()) / 54) * maxLength),
			pRulesExt->FactoryProgressDisplay_Pips,
			pRulesExt->ProgressDisplay_Buildings_PipsShape.Get(),
			0,
			-1,
			nullptr,
			maxLength,
			0,
			&position,
			pBounds
		};

		TechnoExt::DrawVanillaStyleBuildingBar(&pDraw);
	}

	if (haveSecondary)
	{
		auto position = havePrimary ? location + Point2D { 6, 3 } : location;

		DrawFrameStruct pDraw
		{
			static_cast<int>((static_cast<double>(pSecondaryFactory->GetProgress()) / 54) * maxLength),
			pRulesExt->FactoryProgressDisplay_Pips,
			pRulesExt->ProgressDisplay_Buildings_PipsShape.Get(),
			0,
			-1,
			nullptr,
			maxLength,
			0,
			&position,
			pBounds
		};

		TechnoExt::DrawVanillaStyleBuildingBar(&pDraw);
	}
}

void TechnoExt::DrawSuperProgress(BuildingClass* pThis, RectangleStruct* pBounds, Point2D basePosition)
{
	const auto pRulesExt = RulesExt::Global();

	if (!pRulesExt->MainSWProgressDisplay)
		return;

	const auto pType = pThis->Type;
	const auto pOwner = pThis->Owner;
	const auto superIndex = pType->SuperWeapon;
	const auto pSuper = (superIndex != -1) ? pOwner->Supers.GetItem(superIndex) : nullptr;

	if (!pSuper || !SWTypeExt::ExtMap.Find(pSuper->Type)->IsAvailable(pOwner))
		return;

	const auto maxLength = pType->GetFoundationHeight(false) * 15 >> 1;
	auto position = basePosition + Point2D { 6, (3 + pType->PixelSelectionBracketDelta) } + pRulesExt->MainSWProgressDisplay_Offset.Get();

	DrawFrameStruct pDraw
	{
		static_cast<int>((static_cast<double>(pSuper->AnimStage()) / 54) * maxLength),
		pRulesExt->MainSWProgressDisplay_Pips,
		pRulesExt->ProgressDisplay_Buildings_PipsShape.Get(),
		0,
		-1,
		nullptr,
		maxLength,
		0,
		&position,
		pBounds
	};

	TechnoExt::DrawVanillaStyleBuildingBar(&pDraw);
}

void TechnoExt::DrawIronCurtainProgress(TechnoClass* pThis, RectangleStruct* pBounds, Point2D basePosition, bool isBuilding, bool isInfantry)
{
	if (!pThis->IsIronCurtained())
		return;

	const auto pRulesExt = RulesExt::Global();

	if (!pRulesExt->InvulnerableDisplay)
		return;

	const auto timer = &pThis->IronCurtainTimer;

	if (isBuilding)
	{
		const auto pBuilding = static_cast<BuildingClass*>(pThis);
		const auto pType = pBuilding->Type;
		const auto maxLength = pType->GetFoundationHeight(false) * 15 >> 1;
		const auto offset = pRulesExt->InvulnerableDisplay_Buildings_Offset.Get();
		auto position = basePosition + Point2D { offset.X, pType->PixelSelectionBracketDelta + offset.Y };

		DrawFrameStruct pDraw
		{
			static_cast<int>((static_cast<double>(timer->GetTimeLeft()) / timer->TimeLeft) * maxLength + 0.99),
			(pThis->ForceShielded ? pRulesExt->InvulnerableDisplay_Buildings_Pips.Get().X : pRulesExt->InvulnerableDisplay_Buildings_Pips.Get().Y),
			pRulesExt->ProgressDisplay_Buildings_PipsShape.Get(),
			0,
			-1,
			nullptr,
			maxLength,
			-1,
			&position,
			pBounds
		};

		if (offset == Point2D::Empty && (pThis->IsSelected || pThis->IsMouseHovering)) // Layer fix
		{
			RulesClass* const pRules = RulesClass::Instance;
			const auto ratio = pBuilding->GetHealthPercentage();
			pDraw.MidLength = static_cast<int>(ratio * maxLength);
			pDraw.MidFrame = (ratio > pRules->ConditionYellow) ? 1 : (ratio > pRules->ConditionRed ? 2 : 4);
			pDraw.MidPipSHP = FileSystem::PIPS_SHP;
			pDraw.BrdFrame = 0;
		}

		TechnoExt::DrawVanillaStyleBuildingBar(&pDraw);
	}
	else
	{
		const int maxLength = isInfantry ? 8 : 17;
		auto position = basePosition + Point2D { 0, pThis->GetTechnoType()->PixelSelectionBracketDelta + 2 } + pRulesExt->InvulnerableDisplay_Others_Offset.Get();

		DrawFrameStruct pDraw
		{
			static_cast<int>((static_cast<double>(timer->GetTimeLeft()) / timer->TimeLeft) * maxLength + 0.99),
			(pThis->ForceShielded ? pRulesExt->InvulnerableDisplay_Others_Pips.Get().X : pRulesExt->InvulnerableDisplay_Others_Pips.Get().Y),
			pRulesExt->ProgressDisplay_Others_PipsShape.Get(),
			0,
			-1,
			nullptr,
			maxLength,
			-1,
			&position,
			pBounds
		};

		TechnoExt::DrawVanillaStyleFootBar(&pDraw);
	}
}

void TechnoExt::DrawTemporalProgress(TechnoClass* pThis, RectangleStruct* pBounds, Point2D basePosition, bool isBuilding, bool isInfantry)
{
	const auto pTemporal = pThis->TemporalTargetingMe;

	if (!pTemporal)
		return;

	const auto pRulesExt = RulesExt::Global();

	if (!pRulesExt->TemporalLifeDisplay)
		return;

	if (isBuilding)
	{
		const auto pBuilding = static_cast<BuildingClass*>(pThis);
		const auto pType = pBuilding->Type;
		const auto maxLength = pType->GetFoundationHeight(false) * 15 >> 1;
		const auto offset = pRulesExt->TemporalLifeDisplay_Buildings_Offset.Get();
		auto position = basePosition + Point2D { offset.X, pType->PixelSelectionBracketDelta + offset.Y };

		DrawFrameStruct pDraw
		{
			static_cast<int>((static_cast<double>(pTemporal->WarpRemaining) / (pType->Strength * 10)) * maxLength + 0.99),
			pRulesExt->TemporalLifeDisplay_Buildings_Pips,
			pRulesExt->ProgressDisplay_Buildings_PipsShape.Get(),
			0,
			-1,
			nullptr,
			maxLength,
			-1,
			&position,
			pBounds
		};

		if (offset == Point2D::Empty && (pThis->IsSelected || pThis->IsMouseHovering)) // Layer fix
		{
			RulesClass* const pRules = RulesClass::Instance;
			const auto ratio = pBuilding->GetHealthPercentage();
			pDraw.MidLength = static_cast<int>(ratio * maxLength);
			pDraw.MidFrame = (ratio > pRules->ConditionYellow) ? 1 : (ratio > pRules->ConditionRed ? 2 : 4);
			pDraw.MidPipSHP = FileSystem::PIPS_SHP;
			pDraw.BrdFrame = 0;
		}

		TechnoExt::DrawVanillaStyleBuildingBar(&pDraw);
	}
	else
	{
		const int maxLength = isInfantry ? 8 : 17;
		const auto pType = pThis->GetTechnoType();
		auto position = basePosition + Point2D { 0, (pType->PixelSelectionBracketDelta + 2) } + pRulesExt->TemporalLifeDisplay_Others_Offset.Get();

		DrawFrameStruct pDraw
		{
			static_cast<int>((static_cast<double>(pTemporal->WarpRemaining) / (pType->Strength * 10)) * maxLength + 0.99),
			pRulesExt->TemporalLifeDisplay_Others_Pips,
			pRulesExt->ProgressDisplay_Others_PipsShape.Get(),
			0,
			-1,
			nullptr,
			maxLength,
			-1,
			&position,
			pBounds
		};

		TechnoExt::DrawVanillaStyleFootBar(&pDraw);
	}
}

void TechnoExt::DrawVanillaStyleFootBar(DrawFrameStruct* pDraw)
{
	const auto pLocation = pDraw->Location;
	const auto pBounds = pDraw->Bounds;

	if (pDraw->BrdFrame >= 0)
	{
		pLocation->X += 17;
		DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, FileSystem::PIPBRD_SHP, pDraw->BrdFrame, pLocation, pBounds, BlitterFlags(0xE00), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
		pLocation->X -= 15;
	}
	else
	{
		pLocation->X += 2;
	}

	pLocation->Y += 1;

	const auto topLength = pDraw->TopLength;
	const auto midLength = pDraw->MidLength;
	const auto maxLength = pDraw->MaxLength;

	auto length = topLength > maxLength ? maxLength : topLength;

	if (pDraw->TopFrame >= 0 && pDraw->TopPipSHP)
	{
		for (auto drawIdx = length; drawIdx > 0 ; --drawIdx, pLocation->X += 2)
			DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, pDraw->TopPipSHP, pDraw->TopFrame, pLocation, pBounds, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}

	length = midLength > maxLength ? maxLength - length : midLength - length;

	if (pDraw->MidFrame >= 0 && pDraw->MidPipSHP)
	{
		for (auto drawIdx = length; drawIdx > 0 ; --drawIdx, pLocation->X += 2)
			DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, pDraw->MidPipSHP, pDraw->MidFrame, pLocation, pBounds, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}
}

void TechnoExt::DrawVanillaStyleBuildingBar(DrawFrameStruct* pDraw)
{
	const auto pLocation = pDraw->Location;
	++pLocation->X;
	const auto pBounds = pDraw->Bounds;

	const auto topLength = pDraw->TopLength;
	const auto midLength = pDraw->MidLength;
	const auto maxLength = pDraw->MaxLength;

	auto length = topLength > maxLength ? maxLength : topLength;

	if (pDraw->TopFrame >= 0 && pDraw->TopPipSHP)
	{
		for (auto drawIdx = length; drawIdx > 0 ; --drawIdx, pLocation->X -= 4, pLocation->Y += 2)
			DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, pDraw->TopPipSHP, pDraw->TopFrame, pLocation, pBounds, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}

	length = midLength > maxLength ? maxLength - length : midLength - length;

	if (pDraw->MidFrame >= 0 && pDraw->MidPipSHP)
	{
		for (auto drawIdx = length; drawIdx > 0 ; --drawIdx, pLocation->X -= 4, pLocation->Y += 2)
			DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, pDraw->MidPipSHP, pDraw->MidFrame, pLocation, pBounds, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}

	length = length >= 0 ? maxLength - midLength : maxLength - topLength;

	if (pDraw->BrdFrame >= 0)
	{
		for (auto drawIdx = length; drawIdx > 0 ; --drawIdx, pLocation->X -= 4, pLocation->Y += 2)
			DSurface::Temp->DrawSHP(FileSystem::PALETTE_PAL, FileSystem::PIPS_SHP, pDraw->BrdFrame, pLocation, pBounds, BlitterFlags(0x600), 0, 0, ZGradient::Ground, 1000, 0, 0, 0, 0, 0);
	}
}

Point2D TechnoExt::GetScreenLocation(TechnoClass* pThis)
{
	return TacticalClass::Instance->CoordsToClient(pThis->GetCoords()).first;
}

Point2D TechnoExt::GetFootSelectBracketPosition(TechnoClass* pThis, Anchor anchor)
{
	int length = 17;
	Point2D position = GetScreenLocation(pThis);

	if (pThis->WhatAmI() == AbstractType::Infantry)
		length = 8;

	RectangleStruct bracketRect =
	{
		position.X - length + (length == 8) + 1,
		position.Y - 28 + (length == 8),
		length * 2,
		length * 3
	};

	return anchor.OffsetPosition(bracketRect);
}

Point2D TechnoExt::GetBuildingSelectBracketPosition(TechnoClass* pThis, BuildingSelectBracketPosition bracketPosition)
{
	const auto pBuildingType = static_cast<BuildingTypeClass*>(pThis->GetTechnoType());
	Point2D position = GetScreenLocation(pThis);
	CoordStruct dim2 = CoordStruct::Empty;
	pBuildingType->Dimension2(&dim2);
	dim2 = { -dim2.X / 2, dim2.Y / 2, dim2.Z };
	Point2D positionFix = TacticalClass::CoordsToScreen(dim2);

	const int foundationWidth = pBuildingType->GetFoundationWidth();
	const int foundationHeight = pBuildingType->GetFoundationHeight(false);
	const int height = pBuildingType->Height * 12;
	const int lengthW = foundationWidth * 7 + foundationWidth / 2;
	const int lengthH = foundationHeight * 7 + foundationHeight / 2;

	position.X += positionFix.X + 3 + lengthH * 4;
	position.Y += positionFix.Y + 4 - lengthH * 2;

	switch (bracketPosition)
	{
	case BuildingSelectBracketPosition::Top:
		break;
	case BuildingSelectBracketPosition::LeftTop:
		position.X -= lengthH * 4;
		position.Y += lengthH * 2;
		break;
	case BuildingSelectBracketPosition::LeftBottom:
		position.X -= lengthH * 4;
		position.Y += lengthH * 2 + height;
		break;
	case BuildingSelectBracketPosition::Bottom:
		position.Y += lengthW * 2 + lengthH * 2 + height;
		break;
	case BuildingSelectBracketPosition::RightBottom:
		position.X += lengthW * 4;
		position.Y += lengthW * 2 + height;
		break;
	case BuildingSelectBracketPosition::RightTop:
		position.X += lengthW * 4;
		position.Y += lengthW * 2;
	default:
		break;
	}

	return position;
}

void TechnoExt::DrawSelectBox(TechnoClass* pThis, const Point2D* pLocation, const RectangleStruct* pBounds, bool drawBefore)
{
	const auto whatAmI = pThis->WhatAmI();
	const auto pType = pThis->GetTechnoType();
	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);
	SelectBoxTypeClass* pSelectBox = nullptr;

	if (pTypeExt->SelectBox.isset())
		pSelectBox = pTypeExt->SelectBox.Get();
	else if (whatAmI == InfantryClass::AbsID)
		pSelectBox = RulesExt::Global()->DefaultInfantrySelectBox.Get();
	else if (whatAmI != BuildingClass::AbsID)
		pSelectBox = RulesExt::Global()->DefaultUnitSelectBox.Get();

	if (!pSelectBox || pSelectBox->DrawAboveTechno == drawBefore)
		return;

	const bool canSee = HouseClass::IsCurrentPlayerObserver() ? pSelectBox->VisibleToHouses_Observer : EnumFunctions::CanTargetHouse(pSelectBox->VisibleToHouses, pThis->Owner, HouseClass::CurrentPlayer);

	if (!canSee)
		return;

	const double healthPercentage = pThis->GetHealthPercentage();
	const auto defaultFrame = whatAmI == InfantryClass::AbsID ? Vector3D<int> { 1, 1, 1 } : Vector3D<int> { 0, 0, 0 };

	const auto pSurface = DSurface::Temp;
	const auto flags = (drawBefore ? BlitterFlags::Flat | BlitterFlags::Alpha : BlitterFlags::Nonzero | BlitterFlags::MultiPass) | BlitterFlags::Centered | pSelectBox->Translucency;
	const int zAdjust = drawBefore ? pThis->GetZAdjustment() - 2 : 0;
	const auto pGroundShape = pSelectBox->GroundShape.Get();

	if ((pGroundShape || pSelectBox->GroundLine) && whatAmI != BuildingClass::AbsID && (pSelectBox->Ground_AlwaysDraw || pThis->IsInAir()))
	{
		CoordStruct coords = pThis->GetCenterCoords();
		coords.Z = MapClass::Instance.GetCellFloorHeight(coords);
		auto [point, visible] = TacticalClass::Instance->CoordsToClient(coords);

		if (visible && pGroundShape)
		{
			const auto pPalette = pSelectBox->GroundPalette.GetOrDefaultConvert(FileSystem::PALETTE_PAL);

			const Vector3D<int> frames = pSelectBox->GroundFrames.Get(defaultFrame);
			const int frame = healthPercentage > RulesClass::Instance->ConditionYellow ? frames.X : healthPercentage > RulesClass::Instance->ConditionRed ? frames.Y : frames.Z;

			const Point2D drawPoint = point + pSelectBox->GroundOffset;
			pSurface->DrawSHP(pPalette, pGroundShape, frame, &drawPoint, pBounds, flags, 0, zAdjust, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
		}

		if (pSelectBox->GroundLine)
		{
			Point2D start = *pLocation; // Copy to prevent be modified
			const int color = Drawing::RGB_To_Int(pSelectBox->GroundLineColor.Get(healthPercentage));

			if (pSelectBox->GroundLine_Dashed)
				pSurface->DrawDashed(&start, &point, color, 0);
			else if (Line_In_Bounds(&start, &point, &DSurface::ViewBounds))
				pSurface->DrawLine(&start, &point, color);
		}
	}

	if (const auto pShape = pSelectBox->Shape.Get())
	{
		const auto pPalette = pSelectBox->Palette.GetOrDefaultConvert(FileSystem::PALETTE_PAL);

		const Vector3D<int> frames = pSelectBox->Frames.Get(defaultFrame);
		const int frame = healthPercentage > RulesClass::Instance->ConditionYellow ? frames.X : healthPercentage > RulesClass::Instance->ConditionRed ? frames.Y : frames.Z;

		const Point2D offset = whatAmI == InfantryClass::AbsID ? Point2D { 8, -3 } : Point2D { 1, -4 };
		Point2D drawPoint = *pLocation + offset + pSelectBox->Offset;

		if (pSelectBox->DrawAboveTechno)
			drawPoint.Y += pType->PixelSelectionBracketDelta;

		pSurface->DrawSHP(pPalette, pShape, frame, &drawPoint, pBounds, flags, 0, zAdjust, ZGradient::Ground, 1000, 0, nullptr, 0, 0, 0);
	}
}

void TechnoExt::ProcessDigitalDisplays(TechnoClass* pThis)
{
	if (!Phobos::Config::DigitalDisplay_Enable)
		return;

	const auto pType = pThis->GetTechnoType();
	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	if (pTypeExt->DigitalDisplay_Disable)
		return;

	int length = 17;
	ValueableVector<DigitalDisplayTypeClass*>* pDisplayTypes = nullptr;
	const auto whatAmI = pThis->WhatAmI();

	if (!pTypeExt->DigitalDisplayTypes.empty())
	{
		pDisplayTypes = &pTypeExt->DigitalDisplayTypes;
	}
	else
	{
		switch (whatAmI)
		{
		case AbstractType::Building:
		{
			pDisplayTypes = &RulesExt::Global()->Buildings_DefaultDigitalDisplayTypes;
			const auto pBuildingType = static_cast<BuildingTypeClass*>(pThis->GetTechnoType());
			const int height = pBuildingType->GetFoundationHeight(false);
			length = height * 7 + height / 2;
			break;
		}
		case AbstractType::Infantry:
		{
			pDisplayTypes = &RulesExt::Global()->Infantry_DefaultDigitalDisplayTypes;
			length = 8;
			break;
		}
		case AbstractType::Unit:
		{
			pDisplayTypes = &RulesExt::Global()->Vehicles_DefaultDigitalDisplayTypes;
			break;
		}
		case AbstractType::Aircraft:
		{
			pDisplayTypes = &RulesExt::Global()->Aircraft_DefaultDigitalDisplayTypes;
			break;
		}
		default:
			return;
		}
	}

	const auto pShield = TechnoExt::ExtMap.Find(pThis)->Shield.get();
	const bool hasShield = pShield && !pShield->IsBrokenAndNonRespawning();
	const bool isBuilding = whatAmI == AbstractType::Building;
	const bool isInfantry = whatAmI == AbstractType::Infantry;

	for (DigitalDisplayTypeClass*& pDisplayType : *pDisplayTypes)
	{
		if (HouseClass::IsCurrentPlayerObserver() && !pDisplayType->VisibleToHouses_Observer)
			continue;

		if (!HouseClass::IsCurrentPlayerObserver() && !EnumFunctions::CanTargetHouse(pDisplayType->VisibleToHouses, pThis->Owner, HouseClass::CurrentPlayer))
			continue;

		if (!pDisplayType->VisibleInSpecialState && (pThis->TemporalTargetingMe || pThis->IsIronCurtained()))
			continue;

		int value = -1;
		int maxValue = 0;

		GetValuesForDisplay(pThis, pDisplayType->InfoType, value, maxValue, pDisplayType->InfoIndex);

		if (value <= -1 || maxValue <= 0)
			continue;

		const auto divisor = pDisplayType->ValueScaleDivisor.Get(pDisplayType->ValueAsTimer ? 15 : 1);

		if (divisor > 1)
		{
			value = Math::max(value / divisor, value ? 1 : 0);
			maxValue = Math::max(maxValue / divisor, 1);
		}

		Point2D position = whatAmI == AbstractType::Building
			? GetBuildingSelectBracketPosition(pThis, pDisplayType->AnchorType_Building)
			: GetFootSelectBracketPosition(pThis, pDisplayType->AnchorType);
		position.Y += pType->PixelSelectionBracketDelta;

		if (pDisplayType->InfoType == DisplayInfoType::Shield)
			position.Y += pShield->GetType()->BracketDelta;

		pDisplayType->Draw(position, length, value, maxValue, isBuilding, isInfantry, hasShield);
	}
}

void TechnoExt::GetValuesForDisplay(TechnoClass* pThis, DisplayInfoType infoType, int& value, int& maxValue, int infoIndex)
{
	const auto pType = pThis->GetTechnoType();

	switch (infoType)
	{
	case DisplayInfoType::Health:
	{
		value = pThis->Health;
		maxValue = pType->Strength;

		if (pThis->Disguised && !pThis->Owner->IsAlliedWith(HouseClass::CurrentPlayer))
			TechnoExt::GetDigitalDisplayFakeHealth(pThis, value, maxValue);

		break;
	}
	case DisplayInfoType::Shield:
	{
		const auto pShield = TechnoExt::ExtMap.Find(pThis)->Shield.get();

		if (!pShield || pShield->IsBrokenAndNonRespawning())
			return;

		value = pShield->GetHP();
		maxValue = pShield->GetType()->Strength.Get();
		break;
	}
	case DisplayInfoType::Ammo:
	{
		value = pThis->Ammo;
		maxValue = pType->Ammo;
		break;
	}
	case DisplayInfoType::MindControl:
	{
		const auto pCaptureManager = pThis->CaptureManager;

		if (!pCaptureManager)
			return;

		value = pCaptureManager->ControlNodes.Count;
		maxValue = pCaptureManager->MaxControlNodes;
		break;
	}
	case DisplayInfoType::Spawns:
	{
		const auto pSpawnManager = pThis->SpawnManager;

		if (!pSpawnManager || !pType->Spawns)
			return;

		if (infoIndex == 1)
			value = pSpawnManager->CountDockedSpawns();
		else if (infoIndex == 2)
			value = pSpawnManager->CountLaunchingSpawns();
		else
			value = pSpawnManager->CountAliveSpawns();

		maxValue = pType->SpawnsNumber;
		break;
	}
	case DisplayInfoType::Passengers:
	{
		value = pThis->Passengers.NumPassengers;
		maxValue = pType->Passengers;
		break;
	}
	case DisplayInfoType::Tiberium:
	{
		if (infoIndex && infoIndex <= TiberiumClass::Array.Count)
			value = static_cast<int>(pThis->Tiberium.GetAmount(infoIndex - 1));
		else
			value = static_cast<int>(pThis->Tiberium.GetTotalAmount());

		maxValue = pType->Storage;
		break;
	}
	case DisplayInfoType::Experience:
	{
		value = static_cast<int>(pThis->Veterancy.Veterancy * RulesClass::Instance->VeteranRatio * pType->GetCost());
		maxValue = static_cast<int>(2.0 * RulesClass::Instance->VeteranRatio * pType->GetCost());
		break;
	}
	case DisplayInfoType::Occupants:
	{
		if (pThis->WhatAmI() != AbstractType::Building)
			return;

		const auto pBuildingType = static_cast<BuildingTypeClass*>(pType);

		if (!pBuildingType->CanBeOccupied)
			return;

		value = static_cast<BuildingClass*>(pThis)->Occupants.Count;
		maxValue = pBuildingType->MaxNumberOccupants;
		break;
	}
	case DisplayInfoType::GattlingStage:
	{
		if (!pType->IsGattling)
			return;

		value = pThis->GattlingValue ? pThis->CurrentGattlingStage + 1 : 0;
		maxValue = pType->WeaponStages;
		break;
	}
	case DisplayInfoType::ROF:
	{
		if (!pThis->IsArmed())
			return;

		const auto& timer = pThis->RearmTimer;
		value = timer.GetTimeLeft();
		maxValue = timer.TimeLeft;
		break;
	}
	case DisplayInfoType::Reload:
	{
		if (pType->Ammo <= 0)
			return;

		const auto& timer = pThis->ReloadTimer;
		value = (pThis->Ammo >= pType->Ammo) ? 0 : timer.GetTimeLeft();
		maxValue = timer.TimeLeft ? timer.TimeLeft : ((pThis->Ammo || pType->EmptyReload <= 0) ? pType->Reload : pType->EmptyReload);
		break;
	}
	case DisplayInfoType::SpawnTimer:
	{
		const auto pSpawnManager = pThis->SpawnManager;

		if (!pSpawnManager || !pType->Spawns || pType->SpawnsNumber <= 0)
			return;

		if (infoIndex && infoIndex <= pSpawnManager->SpawnedNodes.Count)
		{
			value = pSpawnManager->SpawnedNodes[infoIndex - 1]->SpawnTimer.GetTimeLeft();
		}
		else
		{
			for (int i = 0; i < pSpawnManager->SpawnedNodes.Count; ++i)
			{
				const auto pSpawnNode = pSpawnManager->SpawnedNodes[i];

				if (pSpawnNode->Status == SpawnNodeStatus::Dead)
				{
					const int time = pSpawnNode->SpawnTimer.GetTimeLeft();

					if (!value || time < value)
						value = time;
				}
			}
		}

		maxValue = pSpawnManager->RegenRate;
		break;
	}
	case DisplayInfoType::GattlingTimer:
	{
		if (!pType->IsGattling)
			return;

		const auto thisStage = pThis->CurrentGattlingStage;
		const auto& stage = pThis->Veterancy.IsElite() ? pType->EliteStage : pType->WeaponStage;

		value = pThis->GattlingValue;
		maxValue = stage[thisStage];

		if (thisStage > 0)
		{
			value -= stage[thisStage - 1];
			maxValue -= stage[thisStage - 1];
		}

		break;
	}
	case DisplayInfoType::ProduceCash:
	{
		if (pThis->WhatAmI() != AbstractType::Building || static_cast<BuildingTypeClass*>(pType)->ProduceCashAmount <= 0)
			return;

		const auto& timer = static_cast<BuildingClass*>(pThis)->CashProductionTimer;
		value = timer.GetTimeLeft();
		maxValue = timer.TimeLeft;
		break;
	}
	case DisplayInfoType::PassengerKill:
	{
		const auto pExt = TechnoExt::ExtMap.Find(pThis);

		if (!pExt->TypeExtData->PassengerDeletionType)
			return;

		const auto& timer = pExt->PassengerDeletionTimer;
		value = timer.GetTimeLeft();
		maxValue = timer.TimeLeft;
		break;
	}
	case DisplayInfoType::AutoDeath:
	{
		const auto pExt = TechnoExt::ExtMap.Find(pThis);
		const auto pTypeExt = pExt->TypeExtData;

		if (!pTypeExt->AutoDeath_Behavior.isset())
			return;

		if (pTypeExt->AutoDeath_AfterDelay > 0)
		{
			const auto& timer = pExt->AutoDeathTimer;
			value = timer.GetTimeLeft();
			maxValue = timer.TimeLeft;
		}
		else if (pTypeExt->AutoDeath_OnAmmoDepletion)
		{
			value = pThis->Ammo;
			maxValue = pType->Ammo;
		}

		break;
	}
	case DisplayInfoType::SuperWeapon:
	{
		if (pThis->WhatAmI() != AbstractType::Building)
			return;

		auto getSuperTimer = [pThis, pType, infoIndex]() -> CDTimerClass*
		{
			const auto pHouse = pThis->Owner;
			const auto pBuildingType = static_cast<BuildingTypeClass*>(pType);
			const auto pBuildingTypeExt = BuildingTypeExt::ExtMap.Find(pBuildingType);

			if (infoIndex && infoIndex <= pBuildingTypeExt->GetSuperWeaponCount())
			{
				if (infoIndex == 1)
				{
					if (pBuildingType->SuperWeapon != -1)
						return &pHouse->Supers.GetItem(pBuildingType->SuperWeapon)->RechargeTimer;
				}
				else if (infoIndex == 2)
				{
					if (pBuildingType->SuperWeapon2 != -1)
						return &pHouse->Supers.GetItem(pBuildingType->SuperWeapon2)->RechargeTimer;
				}
				else
				{
					const auto& superWeapons = pBuildingTypeExt->SuperWeapons;
					return &pHouse->Supers.GetItem(superWeapons[infoIndex - 3])->RechargeTimer;
				}

				return nullptr;
			}

			if (pBuildingType->SuperWeapon != -1)
				return &pHouse->Supers.GetItem(pBuildingType->SuperWeapon)->RechargeTimer;
			else if (pBuildingType->SuperWeapon2 != -1)
				return &pHouse->Supers.GetItem(pBuildingType->SuperWeapon2)->RechargeTimer;

			const auto& superWeapons = pBuildingTypeExt->SuperWeapons;
			return superWeapons.size() > 0 ? &pHouse->Supers.GetItem(superWeapons[0])->RechargeTimer : nullptr;
		};
		if (const auto pTimer = getSuperTimer())
		{
			value = pTimer->GetTimeLeft();
			maxValue = pTimer->TimeLeft;
		}

		break;
	}
	case DisplayInfoType::IronCurtain:
	{
		if (!pThis->IsIronCurtained())
			return;

		const auto& timer = pThis->IronCurtainTimer;
		value = timer.GetTimeLeft();
		maxValue = timer.TimeLeft;
		break;
	}
	case DisplayInfoType::TemporalLife:
	{
		const auto pTemporal = pThis->TemporalTargetingMe;

		if (!pTemporal)
			return;

		value = pTemporal->WarpRemaining;
		maxValue = pType->Strength * 10;
		break;
	}
	case DisplayInfoType::FactoryProcess:
	{
		if (pThis->WhatAmI() != AbstractType::Building)
			return;

		auto getFactory = [pThis, pType, infoIndex]() -> FactoryClass*
		{
			const auto pHouse = pThis->Owner;
			const auto pBuildingType = static_cast<BuildingTypeClass*>(pType);

			if (infoIndex == 1)
			{
				if (!pHouse->IsControlledByHuman())
					return static_cast<BuildingClass*>(pThis)->Factory;
				else if (pThis->IsPrimaryFactory)
					return pHouse->GetPrimaryFactory(pBuildingType->Factory, pBuildingType->Naval, BuildCat::DontCare);
			}
			else if (infoIndex == 2)
			{
				if (pHouse->IsControlledByHuman() && pThis->IsPrimaryFactory && pBuildingType->Factory == AbstractType::BuildingType)
					return pHouse->Primary_ForDefenses;
			}
			else if (!pHouse->IsControlledByHuman())
			{
				return static_cast<BuildingClass*>(pThis)->Factory;
			}
			else if (pThis->IsPrimaryFactory)
			{
				const auto pFactory = pHouse->GetPrimaryFactory(pBuildingType->Factory, pBuildingType->Naval, BuildCat::DontCare);

				if (pFactory && pFactory->Object)
					return pFactory;
				else if (pBuildingType->Factory == AbstractType::BuildingType)
					return pHouse->Primary_ForDefenses;
			}

			return nullptr;
		};
		if (const auto pFactory = getFactory())
		{
			if (pFactory->Object)
			{
				value = pFactory->GetProgress();
				maxValue = 54;
			}
		}

		break;
	}
	default:
	{
		value = pThis->Health;
		maxValue = pType->Strength;

		if (pThis->Disguised && !pThis->Owner->IsAlliedWith(HouseClass::CurrentPlayer))
			TechnoExt::GetDigitalDisplayFakeHealth(pThis, value, maxValue);

		break;
	}
	}
}

void TechnoExt::GetDigitalDisplayFakeHealth(TechnoClass* pThis, int& value, int& maxValue)
{
	if (TechnoExt::ExtMap.Find(pThis)->TypeExtData->DigitalDisplay_Health_FakeAtDisguise)
	{
		if (const auto pType = TechnoTypeExt::GetTechnoType(pThis->Disguise))
		{
			const int newMaxValue = pType->Strength;
			const double ratio = static_cast<double>(value) / maxValue;
			value = static_cast<int>(ratio * newMaxValue);
			maxValue = newMaxValue;
		}
	}
}

void TechnoExt::DrawExtraImage(TechnoClass* pThis, CellClass* pCell, const CoordStruct& coords, DirStruct dir)
{
	if (!pThis->IsAlive || pThis->Health <= 0 || pThis->IsSinking || pThis->IsCrashing)
		return;

	const auto level = pCell->Level * Unsorted::LevelHeight;

	if (coords.Z < level)
		return;

	const auto pair = TacticalClass::Instance->CoordsToClient(coords);

	if (!pair.second)
		return;

	const bool inAir = coords.Z > (level + Unsorted::CellHeight);
	const auto slope = inAir ? 0 : pCell->SlopeIndex;
	const auto action = (!inAir && !pCell->ContainsBridge() && pCell->LandType == LandType::Water) ? Sequence::Swim : Sequence::Ready;
	TechnoExt::DrawExtraImage(pThis, pair.first, DSurface::ViewBounds, dir, true, action, slope);
}

void TechnoExt::DrawExtraImage(TechnoClass* pThis, const Point2D& location, const RectangleStruct& bounds, DirStruct dir, bool transparent, Sequence action, int tilt)
{
	if (bounds.Width <= 0 || bounds.Height <= 0)
		return;

	const auto& viewBounds = DSurface::ViewBounds;

	if (viewBounds.Width <= 0 || viewBounds.Height <= 0)
		return;

	auto renderBounds = viewBounds;

	if (viewBounds.X < bounds.X)
	{
		renderBounds.X = bounds.X;
		renderBounds.Width += viewBounds.X - bounds.X;
	}

	const int right = bounds.X + bounds.Width;
	if (renderBounds.X + renderBounds.Width > right)
		renderBounds.Width = right - renderBounds.X;

	if (renderBounds.Width <= 0)
		return;

	if (viewBounds.Y < bounds.Y)
	{
		renderBounds.Y = bounds.Y;
		renderBounds.Height += viewBounds.Y - bounds.Y;
	}

	const int down = bounds.Y + bounds.Height;
	if (renderBounds.Y + renderBounds.Height > down)
		renderBounds.Height = down - renderBounds.Y;

	if (renderBounds.Height <= 0)
		return;

	auto renderLocation = location;

	if (bounds.X > DSurface::ViewBounds.X)
		renderLocation.X += DSurface::ViewBounds.X - renderBounds.X;

	if (bounds.Y > DSurface::ViewBounds.Y)
		renderLocation.Y += DSurface::ViewBounds.Y - renderBounds.Y;

	switch (pThis->WhatAmI())
	{
	case AbstractType::Unit:

		TechnoExt::DrawExtraImage(static_cast<UnitClass*>(pThis), &renderLocation, &renderBounds, dir, transparent, tilt);
		break;

	case AbstractType::Infantry:

		TechnoExt::DrawExtraImage(static_cast<InfantryClass*>(pThis), &renderLocation, &renderBounds, dir, transparent, action);
		break;

	case AbstractType::Aircraft:

		TechnoExt::DrawExtraImage(static_cast<AircraftClass*>(pThis), &renderLocation, &renderBounds, dir, transparent);
		break;

	// case AbstractType::Building:

	default:
		break;
	}
}

void TechnoExt::DrawExtraImage(UnitClass* pThis, Point2D* pLocation, RectangleStruct* pBounds, DirStruct dir, bool transparent, int tilt)
{
	const int select = TacticalClass::Instance->SelectableCount;
	TacticalClass::Instance->SelectableCount = 500;
	const bool lightning = LightningStorm::Active;
	LightningStorm::Active = false;
	const auto psy = PsyDom::Status;
	PsyDom::Status = PsychicDominatorStatus::Inactive;
	const auto nuke = NukeFlash::Status;
	NukeFlash::Status = NukeFlashStatus::Inactive;

	const bool warp = pThis->BeingWarpedOut;
	pThis->BeingWarpedOut = transparent;

	const auto pExt = TechnoExt::ExtMap.Find(pThis);
	const auto pType = pThis->Type;

	const bool shadow = pType->NoShadow;
	pType->NoShadow = true;

	const bool berzerk = pThis->Berzerk;
	pThis->Berzerk = false;
	const auto airstrike = pExt->AirstrikeTargetingMe;
	pExt->AirstrikeTargetingMe = nullptr;
	const int iron = pThis->IronCurtainTimer.TimeLeft;
	pThis->IronCurtainTimer.TimeLeft = 0;
	const int flash = pThis->Flashing.DurationRemaining;
	pThis->Flashing.DurationRemaining = 0;
	const bool disguised = pThis->Disguised;
	pThis->Disguised = false;
	const auto disguise = pThis->DisguiseCreationFrame;
	pThis->DisguiseCreationFrame = Unsorted::CurrentFrame + 1;
	const bool warping = pThis->WarpingOut;
	pThis->WarpingOut = false;
	const auto temporal = pThis->TemporalTargetingMe;
	pThis->TemporalTargetingMe = nullptr;
	const auto cloak = pThis->CloakState;
	pThis->CloakState = CloakState::Uncloaked;
	const float arf = pThis->AngleRotatedForwards;
	pThis->AngleRotatedForwards = 0.0f;
	const float ars = pThis->AngleRotatedSideways;
	pThis->AngleRotatedSideways = 0.0f;

	const bool deployed = pThis->Deployed;
	pThis->Deployed = false;
	const bool unloading = pThis->Unloading;
	pThis->Unloading = false;

	const bool bridge = pThis->OnBridge;
	pThis->OnBridge = false;
	const bool map = pThis->IsOnMap;
	pThis->IsOnMap = false;
	const auto coords = pThis->Location;
	pThis->Location = CoordStruct { INT_MAX, INT_MAX, INT_MAX };
	const char tube = pThis->TubeIndex;
	pThis->TubeIndex = -1;

	if (tilt < 0 || tilt > 20) tilt = 0;
	const auto slope = MapClass::InvalidCell.SlopeIndex;
	MapClass::InvalidCell.SlopeIndex = static_cast<BYTE>(tilt);
	struct LocomotionRampTemp { int CurrentRamp; int Rate; };
	struct LocomotionFaceTemp { DirStruct DesiredFacing; DirStruct ROT; double Speed; };
	const auto iLoco = pThis->Locomotor.GetInterfacePtr();
	const auto pDrLoco = locomotion_cast<DriveLocomotionClass*>(iLoco);
	const auto pShLoco = locomotion_cast<ShipLocomotionClass*>(iLoco);
	const auto pAdLoco = locomotion_cast<AdvancedDriveLocomotionClass*>(iLoco);
	const auto pJjLoco = locomotion_cast<JumpjetLocomotionClass*>(iLoco);
	const auto rampTemp = (pDrLoco ? LocomotionRampTemp(pDrLoco->PreviousRamp, pDrLoco->SlopeTimer.Rate)
		: (pShLoco ? LocomotionRampTemp(pShLoco->PreviousRamp, pShLoco->SlopeTimer.Rate)
		: (pAdLoco ? LocomotionRampTemp(pAdLoco->CurrentRamp, pAdLoco->SlopeTimer.Rate) : LocomotionRampTemp())));
	const auto faceTemp = pJjLoco
		? LocomotionFaceTemp(pJjLoco->LocomotionFacing.DesiredFacing, pJjLoco->LocomotionFacing.ROT, pJjLoco->CurrentSpeed)
		: LocomotionFaceTemp();
	if (pDrLoco)
	{
		pDrLoco->PreviousRamp = tilt;
		pDrLoco->SlopeTimer.Rate = 0;
	}
	else if (pShLoco)
	{
		pShLoco->PreviousRamp = tilt;
		pShLoco->SlopeTimer.Rate = 0;
	}
	else if (pAdLoco)
	{
		pAdLoco->CurrentRamp = tilt;
		pAdLoco->SlopeTimer.Rate = 0;
	}
	else if (pJjLoco)
	{
		pJjLoco->LocomotionFacing.DesiredFacing = dir;
		pJjLoco->LocomotionFacing.ROT = DirStruct(0);
		pJjLoco->CurrentSpeed = 0.0;
	}

	const auto bodyDes = pThis->PrimaryFacing.DesiredFacing;
	const auto bodyROT = pThis->PrimaryFacing.ROT;
	const auto turretDes = pThis->SecondaryFacing.DesiredFacing;
	const auto turretROT = pThis->SecondaryFacing.ROT;
	pThis->PrimaryFacing.DesiredFacing = dir;
	pThis->PrimaryFacing.ROT = DirStruct(0);
	pThis->SecondaryFacing.DesiredFacing = dir;
	pThis->SecondaryFacing.ROT = DirStruct(0);

	pThis->DrawIt(pLocation, pBounds);

	pThis->PrimaryFacing.DesiredFacing = bodyDes;
	pThis->PrimaryFacing.ROT = bodyROT;
	pThis->SecondaryFacing.DesiredFacing = turretDes;
	pThis->SecondaryFacing.ROT = turretROT;

	if (pDrLoco)
	{
		pDrLoco->PreviousRamp = rampTemp.CurrentRamp;
		pDrLoco->SlopeTimer.Rate = rampTemp.Rate;
	}
	else if (pShLoco)
	{
		pShLoco->PreviousRamp = rampTemp.CurrentRamp;
		pShLoco->SlopeTimer.Rate = rampTemp.Rate;
	}
	else if (pAdLoco)
	{
		pAdLoco->CurrentRamp = rampTemp.CurrentRamp;
		pAdLoco->SlopeTimer.Rate = rampTemp.Rate;
	}
	else if (pJjLoco)
	{
		pJjLoco->LocomotionFacing.DesiredFacing = faceTemp.DesiredFacing;
		pJjLoco->LocomotionFacing.ROT = faceTemp.ROT;
		pJjLoco->CurrentSpeed = faceTemp.Speed;
	}
	MapClass::InvalidCell.SlopeIndex = slope;

	pThis->OnBridge = bridge;
	pThis->IsOnMap = map;
	pThis->Location = coords;
	pThis->TubeIndex = tube;

	pThis->Deployed = deployed;
	pThis->Unloading = unloading;

	pThis->Berzerk = berzerk;
	pExt->AirstrikeTargetingMe = airstrike;
	pThis->IronCurtainTimer.TimeLeft = iron;
	pThis->Flashing.DurationRemaining = flash;
	pThis->Disguised = disguised;
	pThis->DisguiseCreationFrame = disguise;
	pThis->WarpingOut = warping;
	pThis->TemporalTargetingMe = temporal;
	pThis->CloakState = cloak;
	pThis->AngleRotatedForwards = arf;
	pThis->AngleRotatedSideways = ars;

	pType->NoShadow = shadow;

	pThis->BeingWarpedOut = warp;

	LightningStorm::Active = lightning;
	PsyDom::Status = psy;
	NukeFlash::Status = nuke;
	TacticalClass::Instance->SelectableCount = select;
}

void TechnoExt::DrawExtraImage(InfantryClass* pThis, Point2D* pLocation, RectangleStruct* pBounds, DirStruct dir, bool transparent, Sequence action)
{
	const int select = TacticalClass::Instance->SelectableCount;
	TacticalClass::Instance->SelectableCount = 500;
	const bool lightning = LightningStorm::Active;
	LightningStorm::Active = false;
	const auto psy = PsyDom::Status;
	PsyDom::Status = PsychicDominatorStatus::Inactive;
	const auto nuke = NukeFlash::Status;
	NukeFlash::Status = NukeFlashStatus::Inactive;

	const bool warp = pThis->BeingWarpedOut;
	pThis->BeingWarpedOut = transparent;

	const auto pExt = TechnoExt::ExtMap.Find(pThis);
	const auto pType = pThis->Type;

	const bool shadow = pType->NoShadow;
	pType->NoShadow = true;

	const bool berzerk = pThis->Berzerk;
	pThis->Berzerk = false;
	const auto airstrike = pExt->AirstrikeTargetingMe;
	pExt->AirstrikeTargetingMe = nullptr;
	const int iron = pThis->IronCurtainTimer.TimeLeft;
	pThis->IronCurtainTimer.TimeLeft = 0;
	const int flash = pThis->Flashing.DurationRemaining;
	pThis->Flashing.DurationRemaining = 0;
	const bool disguised = pThis->Disguised;
	pThis->Disguised = false;
	const auto disguise = pThis->DisguiseCreationFrame;
	pThis->DisguiseCreationFrame = Unsorted::CurrentFrame + 1;
	const bool warping = pThis->WarpingOut;
	pThis->WarpingOut = false;
	const auto temporal = pThis->TemporalTargetingMe;
	pThis->TemporalTargetingMe = nullptr;
	const auto cloak = pThis->CloakState;
	pThis->CloakState = CloakState::Uncloaked;

	const bool bridge = pThis->OnBridge;
	pThis->OnBridge = false;
	const bool map = pThis->IsOnMap;
	pThis->IsOnMap = false;
	const auto coords = pThis->Location;
	pThis->Location = CoordStruct { INT_MAX, INT_MAX, INT_MAX };
	const char tube = pThis->TubeIndex;
	pThis->TubeIndex = -1;

	const int anim = pThis->Animation.Value;
	const auto sequence = pThis->SequenceAnim;
	pThis->Animation.Value = 0;
	pThis->SequenceAnim = action;

	struct LocomotionFaceTemp { DirStruct DesiredFacing; DirStruct ROT; };
	const auto pJjLoco = locomotion_cast<JumpjetLocomotionClass*>(pThis->Locomotor);
	const auto faceTemp = pJjLoco ? LocomotionFaceTemp(pJjLoco->LocomotionFacing.DesiredFacing, pJjLoco->LocomotionFacing.ROT) : LocomotionFaceTemp();
	if (pJjLoco)
	{
		pJjLoco->LocomotionFacing.DesiredFacing = dir;
		pJjLoco->LocomotionFacing.ROT = DirStruct(0);
	}

	const auto bodyDes = pThis->PrimaryFacing.DesiredFacing;
	const auto bodyROT = pThis->PrimaryFacing.ROT;
	pThis->PrimaryFacing.DesiredFacing = dir;
	pThis->PrimaryFacing.ROT = DirStruct(0);

	pThis->DrawIt(pLocation, pBounds);

	pThis->PrimaryFacing.DesiredFacing = bodyDes;
	pThis->PrimaryFacing.ROT = bodyROT;

	if (pJjLoco)
	{
		pJjLoco->LocomotionFacing.DesiredFacing = faceTemp.DesiredFacing;
		pJjLoco->LocomotionFacing.ROT = faceTemp.ROT;
	}

	pThis->Animation.Value = anim;
	pThis->SequenceAnim = sequence;

	pThis->OnBridge = bridge;
	pThis->IsOnMap = map;
	pThis->Location = coords;
	pThis->TubeIndex = tube;

	pThis->Berzerk = berzerk;
	pExt->AirstrikeTargetingMe = airstrike;
	pThis->IronCurtainTimer.TimeLeft = iron;
	pThis->Flashing.DurationRemaining = flash;
	pThis->Disguised = disguised;
	pThis->DisguiseCreationFrame = disguise;
	pThis->WarpingOut = warping;
	pThis->TemporalTargetingMe = temporal;
	pThis->CloakState = cloak;

	pType->NoShadow = shadow;

	pThis->BeingWarpedOut = warp;

	LightningStorm::Active = lightning;
	PsyDom::Status = psy;
	NukeFlash::Status = nuke;
	TacticalClass::Instance->SelectableCount = select;
}

void TechnoExt::DrawExtraImage(AircraftClass* pThis, Point2D* pLocation, RectangleStruct* pBounds, DirStruct dir, bool transparent)
{
	const int select = TacticalClass::Instance->SelectableCount;
	TacticalClass::Instance->SelectableCount = 500;
	const bool lightning = LightningStorm::Active;
	LightningStorm::Active = false;
	const auto psy = PsyDom::Status;
	PsyDom::Status = PsychicDominatorStatus::Inactive;
	const auto nuke = NukeFlash::Status;
	NukeFlash::Status = NukeFlashStatus::Inactive;

	const bool warp = pThis->BeingWarpedOut;
	pThis->BeingWarpedOut = transparent;

	const auto pExt = TechnoExt::ExtMap.Find(pThis);
	const auto pType = pThis->Type;

	const bool shadow = pType->NoShadow;
	pType->NoShadow = true;
	const bool drop = pType->IsDropship;
	pType->IsDropship = true;

	const bool berzerk = pThis->Berzerk;
	pThis->Berzerk = false;
	const auto airstrike = pExt->AirstrikeTargetingMe;
	pExt->AirstrikeTargetingMe = nullptr;
	const int iron = pThis->IronCurtainTimer.TimeLeft;
	pThis->IronCurtainTimer.TimeLeft = 0;
	const int flash = pThis->Flashing.DurationRemaining;
	pThis->Flashing.DurationRemaining = 0;
	const bool disguised = pThis->Disguised;
	pThis->Disguised = false;
	const auto disguise = pThis->DisguiseCreationFrame;
	pThis->DisguiseCreationFrame = Unsorted::CurrentFrame + 1;
	const bool warping = pThis->WarpingOut;
	pThis->WarpingOut = false;
	const auto temporal = pThis->TemporalTargetingMe;
	pThis->TemporalTargetingMe = nullptr;
	const auto cloak = pThis->CloakState;
	pThis->CloakState = CloakState::Uncloaked;
	const float arf = pThis->AngleRotatedForwards;
	pThis->AngleRotatedForwards = 0.0f;
	const float ars = pThis->AngleRotatedSideways;
	pThis->AngleRotatedSideways = 0.0f;

	const bool bridge = pThis->OnBridge;
	pThis->OnBridge = false;
	const bool map = pThis->IsOnMap;
	pThis->IsOnMap = false;
	const auto coords = pThis->Location;
	pThis->Location = CoordStruct { INT_MAX, INT_MAX, INT_MAX };
	const char tube = pThis->TubeIndex;
	pThis->TubeIndex = -1;

	struct LocomotionFaceTemp { DirStruct DesiredFacing; DirStruct ROT; double Speed; };
	const auto iLoco = pThis->Locomotor.GetInterfacePtr();
	const auto pJjLoco = locomotion_cast<JumpjetLocomotionClass*>(iLoco);
	const auto faceTemp = pJjLoco
		? LocomotionFaceTemp(pJjLoco->LocomotionFacing.DesiredFacing, pJjLoco->LocomotionFacing.ROT, pJjLoco->CurrentSpeed)
		: LocomotionFaceTemp();
	if (pJjLoco)
	{
		pJjLoco->LocomotionFacing.DesiredFacing = dir;
		pJjLoco->LocomotionFacing.ROT = DirStruct(0);
		pJjLoco->CurrentSpeed = 0.0;
	}

	const auto flyDes = pThis->PrimaryFacing.DesiredFacing;
	const auto flyROT = pThis->PrimaryFacing.ROT;
	const auto bodyDes = pThis->SecondaryFacing.DesiredFacing;
	const auto bodyROT = pThis->SecondaryFacing.ROT;
	pThis->PrimaryFacing.DesiredFacing = dir;
	pThis->PrimaryFacing.ROT = DirStruct(0);
	pThis->SecondaryFacing.DesiredFacing = dir;
	pThis->SecondaryFacing.ROT = DirStruct(0);

	pThis->DrawIt(pLocation, pBounds);

	pThis->PrimaryFacing.DesiredFacing = flyDes;
	pThis->PrimaryFacing.ROT = flyROT;
	pThis->SecondaryFacing.DesiredFacing = bodyDes;
	pThis->SecondaryFacing.ROT = bodyROT;

	if (pJjLoco)
	{
		pJjLoco->LocomotionFacing.DesiredFacing = faceTemp.DesiredFacing;
		pJjLoco->LocomotionFacing.ROT = faceTemp.ROT;
		pJjLoco->CurrentSpeed = faceTemp.Speed;
	}

	pThis->OnBridge = bridge;
	pThis->IsOnMap = map;
	pThis->Location = coords;
	pThis->TubeIndex = tube;

	pThis->Berzerk = berzerk;
	pExt->AirstrikeTargetingMe = airstrike;
	pThis->IronCurtainTimer.TimeLeft = iron;
	pThis->Flashing.DurationRemaining = flash;
	pThis->Disguised = disguised;
	pThis->DisguiseCreationFrame = disguise;
	pThis->WarpingOut = warping;
	pThis->TemporalTargetingMe = temporal;
	pThis->CloakState = cloak;
	pThis->AngleRotatedForwards = arf;
	pThis->AngleRotatedSideways = ars;

	pType->NoShadow = shadow;
	pType->IsDropship = drop;

	pThis->BeingWarpedOut = warp;

	LightningStorm::Active = lightning;
	PsyDom::Status = psy;
	NukeFlash::Status = nuke;
	TacticalClass::Instance->SelectableCount = select;
}
