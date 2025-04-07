#include "SkilledLocomotionClass.h"

#include <InfantryClass.h>
#include <UnitClass.h>
#include <HouseClass.h>
#include <AnimClass.h>
#include <OverlayTypeClass.h>
#include <SpawnManagerClass.h>

#include <Ext/Techno/Body.h>

// Virtual

bool SkilledLocomotionClass::Process()
{
	const auto pLinked = this->LinkedTo;
	const auto slopeIndex = pLinked->GetCell()->SlopeIndex;

	if (slopeIndex != this->PreviousRamp)
	{
		this->CurrentRamp = this->PreviousRamp;
		this->PreviousRamp = slopeIndex;
		this->SlopeTimer.Start(3);
	}

	do
	{
		if (this->TrackNumber == -1 || !this->IsDriving)
		{
			const auto pDest = pLinked->Destination;

			if ((!pDest || pDest->WhatAmI() != AbstractType::Cell
					|| pLinked->GetMapCoords() != static_cast<CellClass*>(pDest)->MapCoords)
				&& (pLinked->CurrentMission != Mission::Guard || this->IsDriving
					|| this->TargetCoord == CoordStruct::Empty || pLinked->Location != this->TargetCoord))
			{
				if (pLinked->PrimaryFacing.IsRotating())
				{
					this->IsRotating = true;
					break;
				}
				else if (this->IsRotating)
				{
					this->IsRotating = false;
					pLinked->UpdatePosition(PCPType::Rotation);

					if (!pLinked->IsAlive || pLinked->InLimbo || pLinked->IsFallingDown)
						return false;
				}

				const auto mission = pLinked->GetCurrentMission();

				if (mission == Mission::Guard && !this->Is_Moving()
					|| (mission == Mission::Unload // Unload bug fix
						&& (pLinked->GetTechnoType()->Passengers <= 0
							|| !pLinked->Passengers.GetFirstPassenger())))
				{
					break;
				}
				else if (!this->Is_Moving() && pLinked->PathDirections[0] == -1)
				{
					if (pLinked->IsSinking)
					{
						if (this->HeadToCoord != CoordStruct::Empty)
						{
							this->HeadToCoord = CoordStruct::Empty;
							this->IsDriving = false;
						}

						this->MovementSpeed = 0.0;
					}
					else if (const auto pDestination = pLinked->Destination)
					{
						this->Move_To(pDestination->GetDestination(pLinked));
					}

					break;
				}
				else if (pLinked->IsInPlayfield && mission != Mission::Enter && this->Is_Moving()
					&& !pLinked->IsInSameZoneAsCoords(this->TargetCoord))
				{
					if (this->HeadToCoord != CoordStruct::Empty)
					{
						this->HeadToCoord = CoordStruct::Empty;
						this->IsDriving = false;
					}

					if (!pLinked->unknown_abstract_array_588.Count)
					{
						pLinked->SetDestination(nullptr, true);
						break;
					}

					pLinked->AbortMotion();
					return pLinked->EnterIdleMode(false, true);
				}

				bool stop = false;
				this->MovingProcess2(&stop, true, false);

				if (stop || !pLinked->IsAlive)
					return false;

				this->MovingProcess(false);

				return !pLinked->IsAlive;
			}

			if (!pLinked->unknown_abstract_array_588.Count)
			{
				pLinked->SetDestination(nullptr, true);
				return false;
			}

			pLinked->AbortMotion();
			pLinked->EnterIdleMode(false, true);
			return false;
		}

		if (this->MovingProcess(false) || !pLinked->IsAlive)
			return false;

		if (this->TrackNumber == -1 && (this->Is_Moving() || pLinked->PathDirections[0] != -1))
		{
			if (pLinked->WhatAmI() == AbstractType::Unit)
			{
				if (static_cast<UnitClass*>(pLinked)->Unloading)
					break;

				if (const auto pDestination = abstract_cast<InfantryClass*>(pLinked->Destination))
				{
					const auto coord = pDestination->GetDestination(pLinked);

					if (coord != this->TargetCoord)
						this->Move_To(coord);
				}
			}

			bool stop = false;
			this->MovingProcess2(&stop, true, false);

			if (stop || !pLinked->IsAlive)
				return false;

			this->MovingProcess(true);

			if (!pLinked->IsAlive)
				return false;
		}
	}
	while (false);

	if (this->Is_Moving_Now() && !(Unsorted::CurrentFrame % 10) && !pLinked->OnBridge
		&& pLinked->GetCell()->LandType == LandType::Water)
	{
		// Customized wake
		if (const auto pAnimType = TechnoTypeExt::ExtMap.Find(pLinked->GetTechnoType())->Wake.Get(RulesClass::Instance->Wake))
			GameCreate<AnimClass>(pAnimType, pLinked->Location);
	}

	if (this->TargetCoord == CoordStruct::Empty && this->HeadToCoord == CoordStruct::Empty
		&& pLinked->PathDirections[0] == -1 && pLinked->SpeedPercentage > 0.0)
	{
		pLinked->SetSpeedPercentage(0.0);
	}

	return this->Is_Moving();
}

void SkilledLocomotionClass::Move_To(CoordStruct to)
{
	const auto pLinked = this->LinkedTo;

	if (!pLinked->IsUnderEMP() && !pLinked->IsParalyzed()
		&& !pLinked->IsBeingWarpedOut() && !pLinked->IsWarpingIn())
	{
		this->TargetCoord = to;

		if (to != CoordStruct::Empty && MapClass::Instance.GetCellAt(to)->ContainsBridge())
			this->TargetCoord.Z += CellClass::BridgeHeight;
	}
}

void SkilledLocomotionClass::Stop_Moving()
{
	if (this->HeadToCoord != CoordStruct::Empty && this->LinkedTo->GetTechnoType()->IsTrain)
	{
		const auto pLinked = static_cast<UnitClass*>(this->LinkedTo);

		if (!pLinked->IsFollowerCar)
		{
			if (auto pFollowerCar = pLinked->FollowerCar)
			{
				do
				{
					pFollowerCar->Locomotor->Stop_Moving();
					pFollowerCar = pFollowerCar->FollowerCar;
				}
				while (pFollowerCar && pFollowerCar != pFollowerCar->FollowerCar);
			}
		}
	}

	if (this->MovementSpeed >= 0.3)
		this->MovementSpeed = 0.3;

	this->TargetCoord = CoordStruct::Empty;
}

void SkilledLocomotionClass::Do_Turn(DirStruct dir)
{
	this->LinkedTo->PrimaryFacing.SetDesired(dir);
}

void SkilledLocomotionClass::Force_Track(int track, CoordStruct coord)
{
	this->TrackNumber = track;
	this->TrackIndex = 0;

	if (coord != CoordStruct::Empty)
	{
		this->HeadToCoord = coord;
		this->IsDriving = true;

		const auto pLinked = this->LinkedTo;
		const auto pCell = MapClass::Instance.GetCellAt(coord);

		if (!reinterpret_cast<bool(__thiscall*)(CellClass*, FootClass*)>(0x481A00)(pCell, pLinked) // CollectCrate
			|| pLinked->InLimbo)
		{
			if (pLinked->IsAlive)
			{
				this->HeadToCoord = CoordStruct::Empty;
				this->IsDriving = false;
			}
		}
		else
		{
			this->MarkOccupation(coord, MarkType::Down);
			this->TargetCoord = coord;
			this->MovementSpeed = 1.0;
		}
	}
}

void SkilledLocomotionClass::Mark_All_Occupation_Bits(MarkType mark)
{
	if (this->HeadToCoord != CoordStruct::Empty)
		this->MarkOccupation(this->HeadToCoord, mark);
}

bool SkilledLocomotionClass::Is_Moving_Here(CoordStruct to)
{
	const auto headToCoord = this->Head_To_Coord();

	if (headToCoord == CoordStruct::Empty)
		return false;

	if (!this->IsOnShortTrack)
	{
		const auto trackNum = this->TrackNumber;

		if (trackNum != -1)
		{
			if (const auto trackStructIndex = SkilledLocomotionClass::TrackData[TrackNumber].NormalTrackStructIndex)
			{
				const auto trackIdx = SkilledLocomotionClass::TrackStruct[trackStructIndex].TrackIndex3;

				if (trackIdx > -1 && this->TrackIndex < trackIdx)
				{
					const auto trackPt = SkilledLocomotionClass::TrackStruct[trackStructIndex].TrackPoint;
					const auto& trackPtr = trackPt[trackIdx];
					auto flag = trackPtr.Flag; // copy
					auto location = CoordStruct::Empty;
					this->GetTrackOffset(location, trackPtr.Point, flag);
					location.Z = this->LinkedTo->Location.Z;

					if (CellClass::Coord2Cell(location) == CellClass::Coord2Cell(to)
						&& std::abs(location.Z - to.Z) <= Unsorted::CellHeight)
					{
						return true;
					}
				}
			}
		}
	}

	return (CellClass::Coord2Cell(headToCoord) == CellClass::Coord2Cell(to)
		&& std::abs(headToCoord.Z - to.Z) <= Unsorted::CellHeight);
}

bool SkilledLocomotionClass::Will_Jump_Tracks()
{
	const auto pathDir = this->LinkedTo->PathDirections[0];

	if (pathDir < 0 || pathDir >= 8)
		return false;

	const auto& data = SkilledLocomotionClass::TrackData[this->TrackNumber];
	const auto dir = DirStruct(data.Face << 8).GetValue<3>();

	if (static_cast<int>(dir) == pathDir || !this->TrackIndex)
		return false;

	const auto trackStructIndex = this->IsOnShortTrack ? data.ShortTrackStructIndex : data.NormalTrackStructIndex;

	if (SkilledLocomotionClass::TrackStruct[trackStructIndex].TrackIndex1 != this->TrackIndex)
		return false;

	const auto dirIndex = SkilledLocomotionClass::TrackData[8 * dir + pathDir].NormalTrackStructIndex;

	return dirIndex && SkilledLocomotionClass::TrackStruct[dirIndex].TrackIndex2;
}

// No virtual

void SkilledLocomotionClass::MarkOccupation(const CoordStruct& to, const MarkType mark)
{
	if (to == CoordStruct::Empty)
		return;

	if (!this->IsOnShortTrack)
	{
		const auto trackNum = this->TrackNumber;

		if (trackNum != -1)
		{
			if (const auto trackStructIndex = SkilledLocomotionClass::TrackData[TrackNumber].NormalTrackStructIndex)
			{
				const auto trackIdx = SkilledLocomotionClass::TrackStruct[trackStructIndex].TrackIndex3;

				if (trackIdx > -1 && this->TrackIndex < trackIdx)
				{
					const auto trackPt = SkilledLocomotionClass::TrackStruct[trackStructIndex].TrackPoint;
					const auto& trackPtr = trackPt[trackIdx];
					auto flag = trackPtr.Flag; // copy
					auto location = CoordStruct::Empty;
					this->GetTrackOffset(location, trackPtr.Point, flag);

					if (mark == MarkType::Up)
					{
						const auto pLinked = this->LinkedTo;
						location.Z = pLinked->GetZ();
						pLinked->UnmarkAllOccupationBits(location);
					}
					else if (mark == MarkType::Down || mark == MarkType::ChangeRedraw)
					{
						const auto pLinked = this->LinkedTo;
						location.Z = pLinked->GetZ();
						pLinked->UnmarkAllOccupationBits(location);
					}
				}
			}
		}
	}

	if (mark == MarkType::Up)
		this->LinkedTo->UnmarkAllOccupationBits(to);
	else if (mark == MarkType::Down || mark == MarkType::ChangeRedraw)
		this->LinkedTo->MarkAllOccupationBits(to);
}

void SkilledLocomotionClass::GetTrackOffset(CoordStruct& buffer, const Point2D& base, int& flag)
{
	const auto dataFlag = SkilledLocomotionClass::TrackData[this->TrackNumber].Flag;
	auto pt = base;

	if (dataFlag & 1)
	{
		pt.X = base.Y;
		pt.Y = base.X;
		flag = static_cast<unsigned char>(0xC0 - flag);
	}

	if (dataFlag & 2)
	{
		pt.X = -pt.X;
		flag = static_cast<unsigned char>(-static_cast<char>(flag));
	}

	if (dataFlag & 4)
	{
		pt.Y = -pt.Y;
		flag = static_cast<unsigned char>(0x80 - flag);
	}

	buffer.X = this->HeadToCoord.X + pt.X;
	buffer.Y = this->HeadToCoord.Y + pt.Y;
}

bool SkilledLocomotionClass::MovingProcess(bool fix)
{
	const auto pLinked = this->LinkedTo;
	const auto pType = pLinked->GetTechnoType();

	if ((!this->IsDriving || this->TrackNumber == -1) && pLinked->PathDirections[0] != 8
		|| this->IsRotating && !pType->Turret)
	{
		this->SpeedAccum = 0;
		return false;
	}

	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	if (pType->Accelerates)
	{
		if (this->TrackNumber < 64
			&& (pLinked->WhatAmI() != AbstractType::Unit
				|| !static_cast<UnitClass*>(pLinked)->Type->Passive))
		{
			auto coords = this->TargetCoord;
			const auto bridgeHeight = MapClass::Instance.GetCellAt(coords)->ContainsBridge() ? CellClass::BridgeHeight : 0;
			coords.Z = bridgeHeight + MapClass::Instance.GetCellFloorHeight(coords);

			const auto delta = pLinked->Location - coords;
			const auto distance = Game::F2I(delta.Magnitude());

			bool adjustedSpeed = false;
			auto speed = pLinked->SpeedPercentage;
			const auto defaultSpeed = pLinked->GetDefaultSpeed();

			if (distance >= pType->SlowdownDistance)
			{
				if (pLinked->IsSinking)
				{
					adjustedSpeed = true;
					speed -= defaultSpeed * 0.0015;

					if (speed < 0.1)
						speed = 0.1;
				}
			}
			else
			{
				adjustedSpeed = true;
				speed -= defaultSpeed * pType->unknown_double_300;

				if (speed < 0.3)
					speed = 0.3;
			}

			do
			{
				if (pLinked->unknown_bool_6B5 && !pTypeExt->SkipCrushSlowdown)
				{
					// Customized crush slow down speed
					speed = Math::min(pTypeExt->CrushSlowdownMultiplier, this->MovementSpeed);
				}
				else if (!adjustedSpeed)
				{
					if (pLinked->SpeedPercentage >= this->MovementSpeed)
					{
						if (pLinked->SpeedPercentage <= this->MovementSpeed)
							break;

						speed = pLinked->SpeedPercentage - defaultSpeed * pType->unknown_double_300;

						if (this->MovementSpeed > speed)
							speed = this->MovementSpeed;
					}
					else
					{
						speed = pType->AccelerationFactor + pLinked->SpeedPercentage;

						if (this->MovementSpeed < speed)
							speed = this->MovementSpeed;
					}
				}

				pLinked->SetSpeedPercentage(speed);
			}
			while (false);

			if (pLinked->WhatAmI() == AbstractType::Unit)
			{
				if (auto pFollowerCar = static_cast<UnitClass*>(pLinked)->FollowerCar)
				{
					do
					{
						pFollowerCar->SetSpeedPercentage(speed);
						pFollowerCar = pFollowerCar->FollowerCar;
					}
					while (pFollowerCar && pFollowerCar != pFollowerCar->FollowerCar);
				}
			}
		}
	}
	else
	{
		pLinked->SetSpeedPercentage(this->MovementSpeed);
	}

	auto speedAccum = this->SpeedAccum + (fix ? 0 : pLinked->GetCurrentSpeed());

	if (pLinked->PathDirections[0] == 8 && this->TrackNumber == -1)
	{
		pLinked->Mark(MarkType::Up);

		if (this->HeadToCoord != CoordStruct::Empty)
		{
			this->HeadToCoord = CoordStruct::Empty;
			this->IsDriving = false;
		}

		const int tubeIndex = pLinked->GetCell()->TubeIndex;

		if (tubeIndex >= 0 && tubeIndex < TubeClass::Array.Count)
		{
			const auto pTube = TubeClass::Array.Items[tubeIndex];
			this->HeadToCoord = CellClass::Cell2Coord(pTube->ExitCell);

			memmove(&pLinked->PathDirections[0], &pLinked->PathDirections[1], 0x5Cu);
			pLinked->PathDirections[23] = -1;
			pLinked->TubeIndex = static_cast<char>(tubeIndex);
			pLinked->unknown_bool_685 = false;

			const auto nextCell = pTube->EnterCell + CellSpread::GetNeighbourOffset(pTube->unknown_int_30[0] & 7);
			const auto pNextCell = MapClass::Instance.GetCellAt(nextCell);
			pLinked->unknown_coords_568 = pNextCell->GetCellCoords();

			const auto currentHeight = MapClass::Instance.GetCellFloorHeight(pLinked->Location);
			const auto exitHeight = MapClass::Instance.GetCellFloorHeight(this->HeadToCoord);
			pLinked->unknown_coords_568.Z = currentHeight + (exitHeight - currentHeight) / pTube->unknown_int_1C0;

			this->IsDriving = true;
			this->TrackNumber = -1;
			return false;
		}

		pLinked->PathDirections[0] = -1;
		this->TrackNumber = -1;
		this->HeadToCoord = CoordStruct::Empty;
		return false;
	}

	if (speedAccum > 7)
	{
		auto pTrackData = &SkilledLocomotionClass::TrackData[this->TrackNumber];
		int trackStructIndex = this->IsOnShortTrack ? pTrackData->ShortTrackStructIndex : pTrackData->NormalTrackStructIndex;
		auto pTrackPoints = SkilledLocomotionClass::TrackStruct[trackStructIndex].TrackPoint;
		const auto pathDir = pLinked->PathDirections[0];

		if (pathDir < -1 || pathDir > 8)
		{
			pLinked->PathDirections[0] = -1;
			return false;
		}

		bool dirChanged = false;

		if (pathDir != 8 && pathDir != -1)
			dirChanged = (static_cast<int>(DirStruct(pTrackData->Face << 8).GetValue<3>()) != pathDir);

		while (true)
		{
			const int trackIndex = this->TrackIndex;
			const auto& trackPoint = pTrackPoints[trackIndex];
			speedAccum -= 7;

			if (trackPoint.Point == Point2D::Empty && trackIndex)
				break;

			if (pLinked->IsStandingStill())
			{
				pLinked->UnmarkAllOccupationBits(pLinked->Location);
				pLinked->FrozenStill = false;
				pLinked->unknown_bool_6B7 = false;
			}

			CellStruct previousCell;

			if (trackIndex)
			{
				const auto& newTrackPoint = pTrackPoints[trackIndex - 1];
				auto prevFlag = newTrackPoint.Flag;
				auto location = CoordStruct::Empty;
				this->GetTrackOffset(location, newTrackPoint.Point, prevFlag);
				previousCell = CellClass::Coord2Cell(location);
			}
			else
			{
				previousCell = pLinked->GetMapCoords();
			}

			auto flag = trackPoint.Flag;
			auto newPos = CoordStruct::Empty;
			this->GetTrackOffset(newPos, trackPoint.Point, flag);
			newPos.Z = pLinked->Location.Z;

			if (CellClass::Coord2Cell(newPos) == CellClass::Coord2Cell(pLinked->Location))
			{
				const bool wasOnMap = pLinked->IsOnMap;
				pLinked->IsOnMap = false;
				pLinked->SetLocation(newPos);
				pLinked->IsOnMap = wasOnMap;
			}
			else
			{
				pLinked->Mark(MarkType::Up);
				pLinked->SetLocation(newPos);

				const auto pOldCell = MapClass::Instance.GetCellAt(previousCell);
				const auto pNewCell = MapClass::Instance.GetCellAt(newPos);

				if (pNewCell->Level == (pOldCell->Level - 4))
				{
					if (pNewCell->ContainsBridge())
						pLinked->OnBridge = true;
					else if (pOldCell->ContainsBridge())
						pLinked->OnBridge = false;
				}
				else if (!pNewCell->ContainsBridge())
				{
					if (pOldCell->ContainsBridge())
						pLinked->OnBridge = false;
				}

				if (pType->IsTrain && !static_cast<UnitClass*>(pLinked)->IsFollowerCar)
				{
					auto pObject = (pLinked->OnBridge
						|| (pLinked->Location.Z >= (CellClass::BridgeHeight + MapClass::Instance.GetCellFloorHeight(pLinked->Location))))
							? pNewCell->AltObject : pNewCell->FirstObject;

					while (pObject)
					{
						const auto pNext = pObject->NextObject;

						if (!pObject->IsCrushable(pLinked))
						{
							auto damage = 10000;
							pObject->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, true, nullptr);
							damage = 20;
							pLinked->ReceiveDamage(&damage, 0, RulesClass::Instance->C4Warhead, nullptr, true, false, nullptr);
						}

						pObject = pNext;
					}
				}

				if (!pLinked->IsAlive)
					return false;

				pLinked->Mark(MarkType::Down);

				do
				{
					if (pNewCell->OverlayTypeIndex != -1)
					{
						if (!this->IsRocking)
							break;

						if ((pType->Crusher || pLinked->HasAbility(Ability::Crusher))
							&& OverlayTypeClass::Array.Items[pNewCell->OverlayTypeIndex]->Wall && pType->TiltsWhenCrushes)
						{
							// Customized crush tilt speed
							pLinked->RockingForwardsPerFrame = static_cast<float>(pTypeExt->CrushForwardTiltPerFrame.Get(-0.05));
						}
					}
					else if (!this->IsRocking)
					{
						break;
					}

					if (pNewCell->GetUnit(false) && pType->MovementZone == MovementZone::CrusherAll)
					{
						pLinked->unknown_bool_6B5 = true;

						if (pType->TiltsWhenCrushes)
							pLinked->RockingForwardsPerFrame = static_cast<float>(pTypeExt->CrushForwardTiltPerFrame.Get(-0.05));
					}
				}
				while (false);
			}

			if (!pLinked->IsAlive)
				return false;

			const bool wasOnMap = pLinked->IsOnMap;
			pLinked->IsOnMap = false;
			pLinked->SetHeight(0);
			pLinked->IsOnMap = wasOnMap;
			pLinked->PrimaryFacing.SetCurrent(DirStruct((flag << 8) + (this->IsForward ? 0 : 32768)));

			if (trackIndex && SkilledLocomotionClass::TrackStruct[trackStructIndex].TrackIndex3 == trackIndex)
				pLinked->UnmarkAllOccupationBits(pLinked->Location);

			if (pathDir != 8 && pathDir != -1 && dirChanged
				&& SkilledLocomotionClass::TrackStruct[trackStructIndex].TrackIndex1 == trackIndex
				&& trackIndex)
			{
				const int newTrack = pathDir + 8 * DirStruct(pTrackData->Face << 8).GetValue<3>();
				const auto pNewTrackData = &SkilledLocomotionClass::TrackData[newTrack];

				if (pNewTrackData->NormalTrackStructIndex
					&& SkilledLocomotionClass::TrackStruct[pNewTrackData->NormalTrackStructIndex].TrackIndex2)
				{
					auto coords = this->HeadToCoord;
					coords.X += SkilledLocomotionClass::CoordDirections[pathDir].X;
					coords.Y += SkilledLocomotionClass::CoordDirections[pathDir].Y;
					const auto pCell = MapClass::Instance.GetCellAt(coords);

					switch (pLinked->IsCellOccupied(pCell, static_cast<FacingType>(pathDir), pLinked->GetCellLevel(), nullptr, true))
					{
						case Move::OK:
						case Move::MovingBlock:
						{
							if (pLinked->WhatAmI() == AbstractType::Unit && static_cast<UnitClass*>(pLinked)->Type->Passive)
								break;

							const auto speedPercent = pLinked->SpeedPercentage;
							this->IsOnShortTrack = false;
							this->TrackNumber = newTrack;
							pTrackData = pNewTrackData;
							dirChanged = false;
							trackStructIndex = pNewTrackData->NormalTrackStructIndex;
							this->TrackIndex = SkilledLocomotionClass::TrackStruct[trackStructIndex].TrackIndex2 - 1;
							pTrackPoints = SkilledLocomotionClass::TrackStruct[trackStructIndex].TrackPoint;

							if (this->HeadToCoord != CoordStruct::Empty)
							{
								this->HeadToCoord = CoordStruct::Empty;
								this->IsDriving = false;
							}

							this->IsDriving = true;
							pLinked->UpdatePosition(PCPType::End);
							this->IsDriving = false;

							if (!pLinked->IsAlive || pLinked->InLimbo || pLinked->IsFallingDown)
								return false;

							if (this->HeadToCoord != CoordStruct::Empty)
							{
								this->HeadToCoord = CoordStruct::Empty;
								this->IsDriving = false;
							}

							if (coords != CoordStruct::Empty)
							{
								this->IsDriving = true;
								this->HeadToCoord = coords;

								if (!reinterpret_cast<bool(__thiscall*)(CellClass*, FootClass*)>(0x481A00)(pCell, pLinked) // CollectCrate
									|| pLinked->InLimbo)
								{
									if (pLinked->IsAlive)
									{
										this->HeadToCoord = CoordStruct::Empty;
										this->IsDriving = false;
									}
								}
								else
								{
									this->MarkOccupation(coords, MarkType::Down);
									pLinked->SetSpeedPercentage(speedPercent);
									memmove(pLinked->PathDirections, &pLinked->PathDirections[1], 0x5Cu);
									pLinked->PathDirections[23] = -1;
								}
							}

							break;
						}

						case Move::Cloak:
						{
							reinterpret_cast<void(__thiscall*)(CellClass*)>(0x483480)(pCell); // RevealObject
							break;
						}

						case Move::ClosedGate:
						{
							MapClass::Instance.MakeTraversable(pLinked, CellClass::Coord2Cell(coords));
							break;
						}

						case Move::Temp:
						{
							const bool onBridge = pCell->ContainsBridge() && (std::abs(pLinked->Location.Z / Unsorted::CellHeight - pCell->Level) > 2);
							MapClass::Instance.GetCellAt(this->HeadToCoord)->ScatterContent(CoordStruct::Empty, true, true, onBridge);

							break;
						}

						default:
						{
							break;
						}
					}
				}
			}

			++this->TrackIndex;

			if (speedAccum <= 7)
				goto END_ACCUM;
		}

		const auto delta = this->HeadToCoord - pLinked->Location;
		const auto distance = std::abs(delta.X) + std::abs(delta.Y);
		speedAccum += Game::F2I((1.0 - distance / 11.0) * 7.0);

		pLinked->FrozenStill = true;
		pLinked->unknown_bool_6B7 = false;

		if (CellClass::Coord2Cell(this->HeadToCoord) == CellClass::Coord2Cell(pLinked->Location))
		{
			const bool wasOnMap = pLinked->IsOnMap;
			pLinked->IsOnMap = false;
			pLinked->SetLocation(this->HeadToCoord);
			pLinked->SetHeight(0);
			pLinked->IsOnMap = wasOnMap;
		}
		else
		{
			pLinked->Mark(MarkType::Up);
			pLinked->SetLocation(this->HeadToCoord);
			pLinked->SetHeight(0);
			pLinked->Mark(MarkType::Down);
		}

		// Clear head-to coordinates
		if (this->HeadToCoord != CoordStruct::Empty)
		{
			this->HeadToCoord = CoordStruct::Empty;
			this->IsDriving = false;
		}

		// Check if reached destination
		this->TrackNumber = -1;
		this->TrackIndex = 0;
		bool reachedDestination = false;

		if (const auto pDestination = pLinked->Destination)
		{
			if (pLinked->GetMapCoords() == CellClass::Coord2Cell(pDestination->GetDestination(pLinked))
				&& std::abs(pLinked->GetDestination(pLinked).Z - this->TargetCoord.Z) < 2 * Unsorted::CellHeight)
			{
				reachedDestination = true;
				this->TargetCoord = CoordStruct::Empty;

				if (this->HeadToCoord != CoordStruct::Empty)
					this->HeadToCoord = CoordStruct::Empty;

				this->IsDriving = false;
			}
		}

		pLinked->UpdatePosition(PCPType::End);

		if (!pLinked->IsAlive || pLinked->InLimbo || pLinked->IsFallingDown)
			return true;

		if (reachedDestination)
		{
			pLinked->AbortMotion();
			pLinked->PathDirections[0] = -1;

			if (pLinked->GetCurrentMission() == Mission::Move && pLinked->EnterIdleMode(false, true))
				return true;
		}

		if (pLinked->vt_entry_504())
			return true;

		if (!pLinked->IsAlive)
			return false;
	}

END_ACCUM:
	this->SpeedAccum = speedAccum;

	if (this->SpeedAccum <= 0)
		return false;

	if (this->TrackNumber <= -1)
		return false;

	const auto pTrackData = &SkilledLocomotionClass::TrackData[this->TrackNumber];
	const int trackStructIndex = this->IsOnShortTrack ? pTrackData->ShortTrackStructIndex : pTrackData->NormalTrackStructIndex;
	const auto pTrackPoint = &SkilledLocomotionClass::TrackStruct[trackStructIndex].TrackPoint[this->TrackIndex];

	if (pTrackPoint->Point == Point2D::Empty && this->TrackIndex)
		return false;

	int flag = pTrackPoint->Flag;
	auto location = CoordStruct::Empty;
	this->GetTrackOffset(location, pTrackPoint->Point, flag);
	const auto record = location;
	const auto pTrackCell = MapClass::Instance.GetCellAt(location);
	location -= pLinked->Location;

	const auto ratio = static_cast<float>(this->SpeedAccum * 0.1428571428571428);
	auto movement = CoordStruct::Empty;
	SkilledLocomotionClass::CoordLerp(&movement, CoordStruct::Empty, location, ratio);
	auto newPos = pLinked->Location + movement;

	const auto pOldCell = MapClass::Instance.GetCellAt(pLinked->Location);
	auto pNewCell = MapClass::Instance.GetCellAt(newPos);

	if (pNewCell != pTrackCell && pNewCell != pOldCell && this->SpeedAccum > 3)
	{
		newPos.X = record.X;
		newPos.Y = record.Y;
		pNewCell = pTrackCell;
	}

	newPos.Z = pLinked->Location.Z;

	if (CellClass::Coord2Cell(newPos) == CellClass::Coord2Cell(pLinked->Location))
	{
		const bool wasOnMap = pLinked->IsOnMap;
		pLinked->IsOnMap = false;
		pLinked->SetLocation(newPos);
		pLinked->IsOnMap = wasOnMap;
	}
	else
	{
		pLinked->Mark(MarkType::Up);
		pLinked->SetLocation(newPos);

		if (pNewCell->Level == (pOldCell->Level - 4))
		{
			if (pNewCell->ContainsBridge())
				pLinked->OnBridge = true;
			else if (pOldCell->ContainsBridge())
				pLinked->OnBridge = false;
		}
		else if (!pNewCell->ContainsBridge())
		{
			if (pOldCell->ContainsBridge())
				pLinked->OnBridge = false;
		}

		pLinked->Mark(MarkType::Down);
	}

	return false;
}

bool SkilledLocomotionClass::MovingProcess2(bool* pStop, bool force, bool check)
{
	const auto pLinked = this->LinkedTo;
	int pathDir = pLinked->PathDirections[0];

	if (!this->Is_Moving() && pathDir == -1)
	{
		this->IsTurretLockedDown = false;

		if (this->HeadToCoord != CoordStruct::Empty)
		{
			this->HeadToCoord = CoordStruct::Empty;
			this->IsDriving = false;
		}

		if (pLinked->GetCurrentMission() == Mission::Move)
			*pStop = pLinked->EnterIdleMode(false, true);

		return false;
	}

	if (this->TargetCoord == CoordStruct::Empty || pLinked->IsBeingWarpedOut() || pLinked->IsWarpingIn())
		return false;

	if (const auto pSpawnManager = pLinked->SpawnManager)
	{
		if (pSpawnManager->CountLaunchingSpawns())
			return true;
	}

	if (pLinked->IsUnderEMP() || pLinked->IsParalyzed())
		return true;

	const auto pType = pLinked->GetTechnoType();

	do
	{
		if (pathDir != -1)
		{
			if (const auto pDest = pLinked->Destination)
			{
				const auto absType = pDest->WhatAmI();

				if (absType == AbstractType::Infantry || absType == AbstractType::Unit)
				{
					const int distance = Game::F2I((pLinked->Location - this->TargetCoord).Magnitude()) / 256;

					if (distance < 24)
					{
						pLinked->PathDirections[distance] = -1;
						pathDir = pLinked->PathDirections[0];
					}
				}
			}

			if (pathDir != -1)
				break;
		}

		auto& timer = pLinked->PathDelayTimer;

		if (timer.GetTimeLeft() > 0)
			return false;

		timer.Start(Game::F2I(RulesClass::Instance->PathDelay * 900.0));

		if (!reinterpret_cast<bool(__thiscall*)(FootClass*, CellStruct, bool, int)>(0x4D3920)(pLinked,
			CellClass::Coord2Cell(this->TargetCoord), false, 0)) // UpdatePathfinding
		{
			if (!this->LinkedTo)
			{
				*pStop = true;
				return false;
			}

			if (!pLinked->IsInSameZoneAsCoords(this->TargetCoord))
			{
				pLinked->SetDestination(nullptr, true);
				return false;
			}

			if (this->TargetCoord == CoordStruct::Empty)
				return false;

			const auto mission = pLinked->GetCurrentMission();

			if (mission != Mission::Enter
				&& (pLinked->Location - this->TargetCoord).Magnitude() < RulesClass::Instance->CloseEnough
				&& (mission == Mission::Move || mission == Mission::Area_Guard))
			{
				if (this->HeadToCoord != CoordStruct::Empty)
				{
					this->HeadToCoord = CoordStruct::Empty;
					this->IsDriving = false;
				}

				if (pLinked->unknown_abstract_array_588.Count)
				{
					pLinked->AbortMotion();

					if (pLinked->EnterIdleMode(false, true))
						return true;
				}
				else
				{
					pLinked->SetDestination(nullptr, true);
				}

				if (!pLinked->IsAlive)
					return false;
			}
			else
			{
				auto primaryFace = pLinked->PrimaryFacing.Current();
				const auto primaryDir = (primaryFace.GetValue<3>() + (this->IsForward ? 0 : 4)) & 7;
				const auto faceCell = pLinked->GetMapCoords() + CellSpread::GetNeighbourOffset(primaryDir);

				if (MapClass::Instance.IsWithinUsableArea(faceCell, true))
				{
					const auto pCell = MapClass::Instance.GetCellAt(faceCell);
					const auto move = pLinked->IsCellOccupied(pCell, static_cast<FacingType>(primaryDir), pLinked->GetCellLevel(), nullptr, true);

					if (move == Move::ClosedGate)
					{
						MapClass::Instance.MakeTraversable(pLinked, faceCell);
					}
					else if (move == Move::Temp)
					{
						if (const auto pCellTechno = pCell->FindTechnoNearestTo(Point2D::Empty,
							(pLinked->Location.Z > (MapClass::Instance.GetCellFloorHeight(CellClass::Cell2Coord(faceCell)) + 2 * Unsorted::CellHeight)), nullptr))
						{
							if (pLinked->Owner->IsAlliedWith(pCellTechno) && !pType->IsTrain)
							{
								if ((pLinked->Location - this->TargetCoord).Magnitude() < RulesClass::Instance->CloseEnough
									&& !pLinked->HasAnyLink()
									&& std::abs(this->TargetCoord.Z - pLinked->Location.Z) < (2 * Unsorted::CellHeight)
									&& MapClass::Instance.GetCellAt(pLinked->Location)->LandType != LandType::Tunnel)
								{
									if (this->HeadToCoord != CoordStruct::Empty)
									{
										this->HeadToCoord = CoordStruct::Empty;
										this->IsDriving = false;
									}

									if (!pLinked->unknown_abstract_array_588.Count)
									{
										pLinked->SetDestination(nullptr, true);
										return false;
									}

									pLinked->AbortMotion();
									return pLinked->EnterIdleMode(false, true);
								}

								const bool onBridge = pCell->ContainsBridge() && (std::abs(pLinked->Location.Z / Unsorted::CellHeight - pCell->Level) > 2);
								pCell->ScatterContent(CoordStruct::Empty, true, true, onBridge);
							}
						}
					}
				}

				const auto pathWaitTimes = pLinked->unknown_int_64C;

				if (pathWaitTimes <= 0)
				{
					if (this->HeadToCoord != CoordStruct::Empty)
					{
						this->HeadToCoord = CoordStruct::Empty;
						this->IsDriving = false;
					}

					if (pLinked->unknown_abstract_array_588.Count)
					{
						pLinked->AbortMotion();

						if (pLinked->EnterIdleMode(false, true))
							return true;
					}
					else
					{
						pLinked->SetDestination(nullptr, true);
					}

					if (!pLinked->IsAlive)
						return false;

					if (pLinked->ShouldScanForTarget)
						VocClass::PlayGlobal(RulesClass::Instance->ScoldSound, 0x2000, 1.0);

					pLinked->ShouldScanForTarget = false;
				}
				else
				{
					pLinked->unknown_int_64C = pathWaitTimes - 1;
				}
			}

			if (!this->Is_Moving())
			{
				if (const auto pTarget = pLinked->Target)
				{
					if (pLinked->IsCloseEnoughToAttack(pTarget))
					{
						pLinked->unknown_bool_688 = true;

						if (const auto pTeam = pLinked->Team)
							reinterpret_cast<void(__thiscall*)(TeamClass*)>(0x6EC3A0)(pTeam); // AbandonTarget

						pLinked->SetTarget(nullptr);
					}
				}
			}

			if (this->HeadToCoord != CoordStruct::Empty)
			{
				this->HeadToCoord = CoordStruct::Empty;
				this->IsDriving = false;
			}

			this->TrackNumber = -1;
			this->IsTurretLockedDown = false;
			return false;
		}

		const auto nowDir = pLinked->PathDirections[0];

		if (nowDir == 8)
			return false;

		const auto pathCell = pLinked->GetMapCoords() + CellSpread::GetNeighbourOffset(nowDir & 7);

		if (MapClass::Instance.IsWithinUsableArea(pathCell, true))
		{
			const auto pCell = MapClass::Instance.GetCellAt(pathCell);
			const auto move = pLinked->IsCellOccupied(pCell, static_cast<FacingType>(nowDir), pLinked->GetCellLevel(), nullptr, true);

			if (move == Move::ClosedGate)
			{
				MapClass::Instance.MakeTraversable(pLinked, pathCell);
			}
			else if (move == Move::Temp)
			{
				if (const auto pCellTechno = pCell->FindTechnoNearestTo(Point2D::Empty,
					(pLinked->Location.Z > (MapClass::Instance.GetCellFloorHeight(CellClass::Cell2Coord(pathCell)) + 2 * Unsorted::CellHeight)), nullptr))
				{
					if (pLinked->Owner->IsAlliedWith(pCellTechno) && !pType->IsTrain)
					{
						if ((pLinked->Location - this->TargetCoord).Magnitude() < RulesClass::Instance->CloseEnough
							&& !pLinked->HasAnyLink()
							&& std::abs(this->TargetCoord.Z - pLinked->Location.Z) < (2 * Unsorted::CellHeight)
							&& MapClass::Instance.GetCellAt(pLinked->Location)->LandType != LandType::Tunnel)
						{
							if (this->HeadToCoord != CoordStruct::Empty)
							{
								this->HeadToCoord = CoordStruct::Empty;
								this->IsDriving = false;
							}

							if (!pLinked->unknown_abstract_array_588.Count)
							{
								pLinked->SetDestination(nullptr, true);
								return false;
							}

							pLinked->AbortMotion();
							return pLinked->EnterIdleMode(false, true);
						}

						const bool onBridge = pCell->ContainsBridge() && (std::abs(pLinked->Location.Z / Unsorted::CellHeight - pCell->Level) > 2);
						pCell->ScatterContent(CoordStruct::Empty, true, true, onBridge);
					}
				}
			}
		}

		pLinked->unknown_int_64C = 10;
		pathDir = pLinked->PathDirections[0];
	}
	while (false);

	if (pathDir == 8)
		return false;

	auto nextPos = pLinked->Location;
	nextPos.X += SkilledLocomotionClass::CoordDirections[pathDir & 7].X;
	nextPos.Y += SkilledLocomotionClass::CoordDirections[pathDir & 7].Y;

	const int cellLevel = MapClass::Instance.GetCellAt(pLinked->Location)->Level + (pLinked->OnBridge ? 4 : 0);
	auto pNextCell = MapClass::Instance.GetCellAt(nextPos);

	if (pLinked->OnBridge != pNextCell->ContainsBridge())
		pLinked->unknown_bool_68B = true;

	if (!pLinked->vt_entry_29C())
		return true;

	auto nextCell = CellClass::Coord2Cell(nextPos);

	if (!MapClass::Instance.MakeTraversable(pLinked, nextCell))
		return true;

	// Reverse movement
	const int desiredRaw = pathDir << 13;

	if (const auto pTarget = pLinked->Target)
	{
		const auto tgtDir = (pLinked->DistanceFrom(pTarget) > 16 * Unsorted::LeptonsPerCell)
			? pLinked->GetTargetDirection(pTarget) : pLinked->PrimaryFacing.Current();
		const auto deltaCurDir = std::abs(static_cast<short>(static_cast<short>(desiredRaw) - static_cast<short>(tgtDir.Raw)));
		const auto deltaOppDir = std::abs(static_cast<short>(static_cast<short>(desiredRaw + 32768) - static_cast<short>(tgtDir.Raw)));
		this->IsForward = deltaCurDir <= deltaOppDir;
	}
	else if (Unsorted::CurrentFrame - TechnoExt::ExtMap.Find(pLinked)->LastHurtFrame <= 150)
	{
		const auto curDir = pLinked->PrimaryFacing.Current();
		const auto deltaCurDir = std::abs(static_cast<short>(static_cast<short>(desiredRaw) - static_cast<short>(curDir.Raw)));
		const auto deltaOppDir = std::abs(static_cast<short>(static_cast<short>(desiredRaw + 32768) - static_cast<short>(curDir.Raw)));
		this->IsForward = deltaCurDir <= deltaOppDir;
	}
	else
	{
		this->IsForward = true;
	}

	const auto desDir = DirStruct(this->IsForward ? desiredRaw : (desiredRaw + 32768));

	if (pLinked->PrimaryFacing.Current() != desDir)
	{
		this->Do_Turn(desDir);
		return true;
	}

	pLinked->Mark(MarkType::Up);
	auto moveResult = pLinked->IsCellOccupied(pNextCell, static_cast<FacingType>(pathDir), cellLevel, nullptr, true);
	pLinked->Mark(MarkType::Down);

	if (moveResult < Move::No && pType->IsTrain
		|| (moveResult == Move::Destroyable || moveResult == Move::FriendlyDestroyable)
			&& pType->Crusher && !pNextCell->OverlayTypeIndex)
	{
		moveResult = Move::OK;
	}

	bool crushableOverlay = false;

	do
	{
		if (pNextCell->OverlayTypeIndex != -1)
		{
			if (moveResult == Move::OK)
			{
				const auto pOverlay = OverlayTypeClass::Array.Items[pNextCell->OverlayTypeIndex];

				if (pOverlay->Crushable || pOverlay->Wall && pType->MovementZone == MovementZone::CrusherAll)
					crushableOverlay = true;

				break;
			}
		}
		else if (moveResult == Move::OK)
		{
			break;
		}

		if (moveResult == Move::ClosedGate)
		{
			MapClass::Instance.MakeTraversable(pLinked, nextCell);
		}
		else if (moveResult == Move::Temp)
		{
			if (!pType->IsTrain)
			{
				if (force)
				{
					pLinked->PathDirections[0] = -1;
					pLinked->PathDelayTimer.Start(0);
					return this->MovingProcess2(pStop, false, false);
				}

				if ((pLinked->Location - this->TargetCoord).Magnitude() < RulesClass::Instance->CloseEnough
					&& std::abs(this->TargetCoord.Z - pLinked->Location.Z) < (2 * Unsorted::CellHeight)
					&& MapClass::Instance.GetCellAt(pLinked->Location)->LandType != LandType::Tunnel)
				{
					if (this->HeadToCoord != CoordStruct::Empty)
					{
						this->HeadToCoord = CoordStruct::Empty;
						this->IsDriving = false;
					}

					if (pLinked->unknown_abstract_array_588.Count)
					{
						pLinked->AbortMotion();

						if (pLinked->EnterIdleMode(false, true))
							return true;
					}
					else
					{
						pLinked->SetDestination(nullptr, true);
					}
				}
				else
				{
					const bool onBridge = pNextCell->ContainsBridge() && (std::abs(pLinked->Location.Z / Unsorted::CellHeight - pNextCell->Level) > 2);
					pNextCell->ScatterContent(CoordStruct::Empty, true, true, onBridge);
				}
			}
		}
		else if (moveResult == Move::Cloak)
		{
			reinterpret_cast<void(__thiscall*)(CellClass*)>(0x483480)(pNextCell); // RevealObject

			if (force)
			{
				pLinked->PathDirections[0] = -1;
				return this->MovingProcess2(pStop, false, false);
			}

			if (this->HeadToCoord != CoordStruct::Empty)
			{
				this->HeadToCoord = CoordStruct::Empty;
				this->IsDriving = false;
			}

			if (!pLinked->unknown_abstract_array_588.Count)
			{
				pLinked->SetDestination(nullptr, true);
				return false;
			}

			pLinked->AbortMotion();
			return pLinked->EnterIdleMode(false, true);
		}

		if (this->HeadToCoord != CoordStruct::Empty)
		{
			this->HeadToCoord = CoordStruct::Empty;
			this->IsDriving = false;
		}

		do
		{
			if (moveResult == Move::MovingBlock)
			{
				if (!pLinked->unknown_bool_6B7)
				{
					pLinked->unknown_bool_6B7 = true;
					pLinked->BlockagePathTimer.Start(RulesClass::Instance->BlockagePathDelay);
				}

				if (!pLinked->PathDelayTimer.GetTimeLeft())
				{
					const int findMode = static_cast<int>(pLinked->unknown_bool_6B7 && !pLinked->BlockagePathTimer.HasTimeLeft()) + 1;
					const bool pathFound = reinterpret_cast<bool(__thiscall*)(FootClass*, CellStruct, bool, int)>(0x4D3920)(pLinked,
						CellClass::Coord2Cell(this->TargetCoord), false, findMode); // UpdatePathfinding

					if (!this->LinkedTo)
					{
						*pStop = true;
						return false;
					}

					if (pathFound || pLinked->IsInSameZoneAsCoords(this->TargetCoord))
					{
						pLinked->PathDelayTimer.Start(Game::F2I(RulesClass::Instance->PathDelay * 900.0));
						return true;
					}

					pLinked->SetDestination(nullptr, true);
					return false;
				}

				if (pLinked->ShouldScanForTarget)
					VocClass::PlayGlobal(RulesClass::Instance->ScoldSound, 0x2000u, 1.0);

				break;
			}

			if (moveResult != Move::Destroyable && moveResult != Move::FriendlyDestroyable)
			{
				if (pLinked->ShouldScanForTarget)
					VocClass::PlayGlobal(RulesClass::Instance->ScoldSound, 0x2000u, 1.0);

				break;
			}

			if (force)
			{
				pLinked->PathDirections[0] = -1;
				pLinked->PathDelayTimer.Start(0);
				return this->MovingProcess2(pStop, false, false);
			}

			if (const auto pObject = pNextCell->GetSomeObject(CoordStruct::Empty, false))
			{
				if (!pLinked->Owner->IsAlliedWith(pObject))
					pLinked->Override_Mission(Mission::Attack, pObject, nullptr);
			}
			else if (pNextCell->OverlayTypeIndex != -1 && OverlayTypeClass::Array.Items[pNextCell->OverlayTypeIndex]->Wall)
			{
				pLinked->Override_Mission(Mission::Attack, pNextCell, nullptr);
			}
		}
		while (false);

		if (moveResult != Move::No)
		{
			pLinked->ShouldScanForTarget = false;
			this->TrackNumber = -1;
			return true;
		}

		if (force)
		{
			pLinked->PathDirections[0] = -1;
			pLinked->PathDelayTimer.Start(0);
			return this->MovingProcess2(pStop, false, false);
		}

		if (this->HeadToCoord != CoordStruct::Empty)
		{
			this->HeadToCoord = CoordStruct::Empty;
			this->IsDriving = false;
		}

		if (!pLinked->unknown_abstract_array_588.Count)
		{
			pLinked->SetDestination(nullptr, true);
			return false;
		}

		pLinked->AbortMotion();
		return pLinked->EnterIdleMode(false, true);
	}
	while (false);

	const bool different = std::abs(cellLevel - pNextCell->Level) >= 2;
	const auto landType = different ? LandType::Road : pNextCell->LandType;
	const auto landLevel = different ? cellLevel : pNextCell->Level;

	double speedFactor = GroundType::Array[static_cast<int>(landType)].Cost[static_cast<int>(pType->SpeedType)];

	if (speedFactor > 1.0)
		speedFactor = 1.0;

	int currentHeight = MapClass::Instance.GetCellFloorHeight(pLinked->Location);
	int nextHeight = MapClass::Instance.GetCellFloorHeight(pNextCell->GetCellCoords());

	if (nextHeight > currentHeight)
	{
		if (pLinked->WhatAmI() == AbstractType::Unit)
		{
			if (pType->SpeedType == SpeedType::Track)
				speedFactor *= RulesClass::Instance->TrackedUphill;
			else
				speedFactor *= RulesClass::Instance->WheeledUphill;
		}
	}
	else if (nextHeight < currentHeight)
	{
		if (pLinked->WhatAmI() == AbstractType::Unit)
		{
			if (pType->SpeedType == SpeedType::Track)
				speedFactor *= RulesClass::Instance->TrackedDownhill;
			else
				speedFactor *= RulesClass::Instance->WheeledDownhill;
		}
	}

	if (speedFactor == 0.0)
		speedFactor = 0.5;

	const auto pTypeExt = TechnoTypeExt::ExtMap.Find(pType);

	// Customized backward speed
	if (!this->IsForward)
		speedFactor *= 0.85;

	// Customized damaged speed
	const auto ratio = pLinked->GetHealthPercentage();

	if (ratio <= RulesClass::Instance->ConditionRed)
		speedFactor *= pTypeExt->VehicleDamagedSpeedMultiplier_Red.Get(RulesExt::Global()->VehicleDamagedSpeedMultiplier_Red);
	else if (ratio <= RulesClass::Instance->ConditionYellow)
		speedFactor *= pTypeExt->VehicleDamagedSpeedMultiplier_Yellow.Get(RulesExt::Global()->VehicleDamagedSpeedMultiplier_Yellow);

	if (this->TrackNumber >= 64)
		pLinked->SetSpeedPercentage(speedFactor);
	else
		this->MovementSpeed = speedFactor;

	pLinked->vt_entry_534(reinterpret_cast<DWORD>(&nextCell), true); // TryCrushCell
	auto nextDir = pLinked->PathDirections[1];

	do
	{
		if (nextDir == -1)
		{
			if (Game::F2I((pLinked->Location - this->TargetCoord).Magnitude()) <= 512)
			{
				nextDir = pathDir;
				break;
			}
			else
			{
				const bool pathFound = reinterpret_cast<bool(__thiscall*)(FootClass*, CellStruct, bool, int)>(0x4D3920)(pLinked,
					CellClass::Coord2Cell(this->TargetCoord), pType->IsTrain, 0); // UpdatePathfinding

				if (!pathFound)
				{
					if (!this->LinkedTo)
					{
						*pStop = true;
						return false;
					}

					if (!pLinked->IsInSameZoneAsCoords(this->TargetCoord))
						pLinked->SetDestination(nullptr, true);
				}

				nextDir = pLinked->PathDirections[1];
			}
		}

		if (nextDir == 8 || nextDir == -1 || check)
			nextDir = pathDir;
	}
	while (false);

	do
	{
		if (nextDir != -1)
		{
			const auto pAdjCell = pNextCell->GetNeighbourCell(static_cast<FacingType>(nextDir));

			if (pAdjCell->OverlayTypeIndex != -1)
			{
				const auto pOverlay = OverlayTypeClass::Array.Items[pAdjCell->OverlayTypeIndex];

				if (pOverlay->Crushable || ((pOverlay->Wall || pAdjCell->GetUnit(false)) && pType->MovementZone == MovementZone::CrusherAll))
				{
					this->IsRocking = true;
					nextDir = pathDir;
					break;
				}
			}
		}

		if (crushableOverlay)
		{
			this->IsRocking = true;
			nextDir = pathDir;
			break;
		}

		this->IsRocking = false;
	}
	while (false);

	this->IsOnShortTrack = false;
	this->TrackNumber = nextDir + 8 * pathDir;

	if (!SkilledLocomotionClass::TrackData[this->TrackNumber].NormalTrackStructIndex)
		this->TrackNumber = 9 * pathDir;

	if (SkilledLocomotionClass::TrackData[this->TrackNumber].Flag & 8)
	{
		auto nextMoveResult = Move::No;

		if (reinterpret_cast<bool(__thiscall*)(CellClass*, FootClass*)>(0x481A00)(pNextCell, pLinked) // CollectCrate
			|| pLinked->InLimbo)
		{
			if (!pLinked->IsAlive)
				return false;

			nextPos.X += SkilledLocomotionClass::CoordDirections[nextDir & 7].X;
			nextPos.Y += SkilledLocomotionClass::CoordDirections[nextDir & 7].Y;
			nextCell = CellClass::Coord2Cell(nextPos);
			pNextCell = MapClass::Instance.GetCellAt(nextCell);
			nextMoveResult = pLinked->IsCellOccupied(pNextCell, static_cast<FacingType>(nextDir), landLevel, nullptr, true);

			if (nextMoveResult < Move::No && pType->IsTrain
				|| (nextMoveResult == Move::FriendlyDestroyable || nextMoveResult == Move::Destroyable)
					&& pType->Crusher && !pNextCell->OverlayTypeIndex)
			{
				nextMoveResult = Move::OK;
			}
		}
		else if (!pLinked->IsAlive)
		{
			return false;
		}

		if (nextMoveResult != Move::OK)
		{
			if (nextMoveResult == Move::ClosedGate)
			{
				MapClass::Instance.MakeTraversable(pLinked, nextCell);
			}
			else if (nextMoveResult == Move::MovingBlock)
			{
				return this->MovingProcess2(pStop, force, true);
			}
			else if (nextMoveResult == Move::Temp)
			{
				if (!pType->IsTrain)
				{
					if (force)
					{
						pLinked->PathDirections[0] = -1;
						pLinked->PathDelayTimer.Start(0);
						return this->MovingProcess2(pStop, false, false);
					}

					if ((pLinked->Location - this->TargetCoord).Magnitude() < RulesClass::Instance->CloseEnough
						&& std::abs(this->TargetCoord.Z - pLinked->Location.Z) < (2 * Unsorted::CellHeight)
						&& MapClass::Instance.GetCellAt(pLinked->Location)->LandType != LandType::Tunnel)
					{
						if (this->HeadToCoord != CoordStruct::Empty)
						{
							this->HeadToCoord = CoordStruct::Empty;
							this->IsDriving = false;
						}

						if (pLinked->unknown_abstract_array_588.Count)
						{
							pLinked->AbortMotion();

							if (pLinked->EnterIdleMode(false, true))
								return true;
						}
						else
						{
							pLinked->SetDestination(nullptr, true);
						}
					}
					else
					{
						const bool onBridge = pNextCell->ContainsBridge() && (std::abs(pLinked->Location.Z / Unsorted::CellHeight - pNextCell->Level) > 2);
						pNextCell->ScatterContent(CoordStruct::Empty, true, true, onBridge);
					}
				}
			}
			else if (nextMoveResult == Move::Cloak)
			{
				reinterpret_cast<void(__thiscall*)(CellClass*)>(0x483480)(pNextCell); // RevealObject

				if (force)
				{
					pLinked->PathDirections[0] = -1;
					return this->MovingProcess2(pStop, false, false);
				}

				if (this->HeadToCoord != CoordStruct::Empty)
				{
					this->HeadToCoord = CoordStruct::Empty;
					this->IsDriving = false;
				}

				if (!pLinked->unknown_abstract_array_588.Count)
				{
					pLinked->SetDestination(nullptr, true);
					return false;
				}

				pLinked->AbortMotion();
				return pLinked->EnterIdleMode(false, true);
			}
			else if (nextMoveResult == Move::No)
			{
				if (force)
				{
					pLinked->PathDirections[0] = -1;
					pLinked->PathDelayTimer.Start(0);
					return this->MovingProcess2(pStop, false, false);
				}

				if (this->HeadToCoord != CoordStruct::Empty)
				{
					this->HeadToCoord = CoordStruct::Empty;
					this->IsDriving = false;
				}

				if (!pLinked->unknown_abstract_array_588.Count)
				{
					pLinked->SetDestination(nullptr, true);
					return false;
				}

				pLinked->AbortMotion();
				return pLinked->EnterIdleMode(false, true);
			}

			pLinked->PathDirections[0] = -1;
			this->TrackNumber = -1;
			nextPos = CoordStruct::Empty;

			if (nextMoveResult == Move::Destroyable || nextMoveResult == Move::FriendlyDestroyable)
				return this->MovingProcess2(pStop, force, true);
		}
		else
		{
			memmove(&pLinked->PathDirections[0], &pLinked->PathDirections[2], 0x58u);
			pLinked->PathDirections[22] = -1;
			pLinked->unknown_bool_68B = true;
		}
	}
	else
	{
		memmove(&pLinked->PathDirections[0], &pLinked->PathDirections[1], 0x5Cu);
	}

	pLinked->PathDirections[23] = -1;
	pLinked->CurrentMapCoords = nextCell;
	pLinked->ShouldScanForTarget = false;
	this->TrackIndex = 0;

	if (this->HeadToCoord != CoordStruct::Empty)
	{
		this->HeadToCoord = CoordStruct::Empty;
		this->IsDriving = false;
	}

	if (nextPos != CoordStruct::Empty)
	{
		this->IsDriving = true;
		this->HeadToCoord = nextPos;

		if (reinterpret_cast<bool(__thiscall*)(CellClass*, FootClass*)>(0x481A00)(pNextCell, pLinked) // CollectCrate
			&& !pLinked->InLimbo)
		{
			this->MarkOccupation(nextPos, MarkType::Down);
			return false;
		}
		else if (pLinked->IsAlive)
		{
			this->HeadToCoord = CoordStruct::Empty;
			this->IsDriving = false;
		}
	}

	this->TrackNumber = -1;
	pLinked->PathDirections[0] = -1;
	pLinked->SetSpeedPercentage(0.0);
	return false;
}

CoordStruct* __fastcall SkilledLocomotionClass::CoordLerp(CoordStruct* pBuffer, const CoordStruct& crd1, const CoordStruct& crd2, float alpha)
{
	const float i_alpha = 1.0f - alpha;
	pBuffer->X = Game::F2I(crd2.X * alpha + crd1.X * i_alpha);
	pBuffer->Y = Game::F2I(crd2.Y * alpha + crd1.Y * i_alpha);
	pBuffer->Z = Game::F2I(crd2.Z * alpha + crd1.Z * i_alpha);
	return pBuffer;
}
