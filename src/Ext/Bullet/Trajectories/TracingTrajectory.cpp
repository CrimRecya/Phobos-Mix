#include "TracingTrajectory.h"

#include <Ext/Bullet/Body.h>
#include <Ext/Techno/Body.h>

namespace detail
{
	template <>
	inline bool read<TraceTargetMode>(TraceTargetMode& value, INI_EX& parser, const char* pSection, const char* pKey)
	{
		if (parser.ReadString(pSection, pKey))
		{
			static std::pair<const char*, TraceTargetMode> FlagNames[] =
			{
				{"Connection", TraceTargetMode::Connection},
				{"Global", TraceTargetMode::Global},
				{"Body", TraceTargetMode::Body},
				{"Turret", TraceTargetMode::Turret},
				{"RotateCW", TraceTargetMode::RotateCW},
				{"RotateCCW", TraceTargetMode::RotateCCW},
			};
			for (auto [name, flag] : FlagNames)
			{
				if (_strcmpi(parser.value(), name) == 0)
				{
					value = flag;
					return true;
				}
			}
			Debug::INIParseFailed(pSection, pKey, parser.value(), "Expected a new trace target mode");
		}

		return false;
	}
}

std::unique_ptr<PhobosTrajectory> TracingTrajectoryType::CreateInstance(BulletClass* pBullet) const
{
	return std::make_unique<TracingTrajectory>(this, pBullet);
}

template<typename T>
void TracingTrajectoryType::Serialize(T& Stm)
{
	Stm
		.Process(this->TraceMode)
		.Process(this->TraceTheTarget)
		.Process(this->CreateAtTarget)
		.Process(this->SuicideAboveRange)
		;
}

bool TracingTrajectoryType::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->VirtualTrajectoryType::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool TracingTrajectoryType::Save(PhobosStreamWriter& Stm) const
{
	this->VirtualTrajectoryType::Save(Stm);
	const_cast<TracingTrajectoryType*>(this)->Serialize(Stm);
	return true;
}

void TracingTrajectoryType::Read(CCINIClass* const pINI, const char* pSection)
{
	this->PhobosTrajectoryType::Read(pINI, pSection);
	INI_EX exINI(pINI);
	// Virtual
	this->VirtualSourceCoord.Read(exINI, pSection, "Trajectory.Tracing.CreateCoord");
	this->VirtualTargetCoord.Read(exINI, pSection, "Trajectory.Tracing.AttachCoord");
	this->AllowFirerTurning.Read(exINI, pSection, "Trajectory.Tracing.AllowFirerTurning");
	// Tracing
	this->TraceMode.Read(exINI, pSection, "Trajectory.Tracing.TraceMode");
	this->TraceTheTarget.Read(exINI, pSection, "Trajectory.Tracing.TraceTheTarget");
	this->CreateAtTarget.Read(exINI, pSection, "Trajectory.Tracing.CreateAtTarget");
	this->SuicideAboveRange.Read(exINI, pSection, "Trajectory.Tracing.SuicideAboveRange");
}

template<typename T>
void TracingTrajectory::Serialize(T& Stm)
{
	Stm
		.Process(this->Type)
		;
}

bool TracingTrajectory::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->VirtualTrajectory::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool TracingTrajectory::Save(PhobosStreamWriter& Stm) const
{
	this->VirtualTrajectory::Save(Stm);
	const_cast<TracingTrajectory*>(this)->Serialize(Stm);
	return true;
}

void TracingTrajectory::OnUnlimbo()
{
	this->VirtualTrajectory::OnUnlimbo();
	// Waiting for launch trigger
	if (!BulletExt::ExtMap.Find(this->Bullet)->DispersedTrajectory)
		this->OpenFire();
}

bool TracingTrajectory::OnAIDetonateCheck()
{
	if (this->VirtualTrajectory::OnAIDetonateCheck())
		return true;

	const auto pBullet = this->Bullet;
	const auto pType = this->Type;
	const auto pFirer = pBullet->Owner;

	if (!pType->TraceTheTarget && !pFirer)
		return true;

	return this->ChangeVelocity();
}

void TracingTrajectory::OpenFire()
{
	const auto pBullet = this->Bullet;
	const auto pType = this->Type;
	const auto& coords = pType->VirtualSourceCoord.Get();
	CoordStruct offset = coords;

	if (coords.X != 0 || coords.Y != 0)
	{
		const auto rotateRadian = this->Get2DOpRadian(pBullet->SourceCoords, pBullet->TargetCoords);
		offset = PhobosTrajectory::Vector2Coord(PhobosTrajectory::HorizontalRotate(coords, rotateRadian));
	}

	if (pType->CreateAtTarget)
	{
		if (const auto pTarget = pBullet->Target)
			pBullet->SetLocation(pTarget->GetCoords() + offset);
		else
			pBullet->SetLocation(pBullet->TargetCoords + offset);
	}
	else
	{
		pBullet->SetLocation(pBullet->SourceCoords + offset);
	}

	const auto duration = pType->Duration.Get();

	if (duration > 0)
	{
		this->DurationTimer.Start(duration);
	}
	else if (!duration)
	{
		if (const auto pWeapon = pBullet->WeaponType)
			this->DurationTimer.Start((pWeapon->ROF > 10) ? pWeapon->ROF - 10 : 1);
		else
			this->DurationTimer.Start(120);
	}

	this->PhobosTrajectory::OpenFire();
}

bool TracingTrajectory::ChangeVelocity()
{
	const auto pBullet = this->Bullet;
	const auto pType = this->Type;
	auto pFirer = pBullet->Owner;

	if (pFirer)
	{
		for (auto pTrans = pFirer->Transporter; pTrans; pTrans = pTrans->Transporter)
			pFirer = pTrans;
	}

	auto destination = (pType->TraceTheTarget || !pFirer) ? pBullet->TargetCoords : pFirer->GetCoords();
	const auto coords = pType->VirtualTargetCoord.Get();
	CoordStruct offset = coords;

	if (const auto pWeapon = pBullet->WeaponType)
	{
		const auto range = (pType->ApplyRangeModifiers && pFirer ? WeaponTypeExt::GetRangeWithModifiers(pWeapon, pFirer, pWeapon->Range) : pWeapon->Range) + 32;
		const auto source = (pFirer && !this->NotMainWeapon) ? pFirer->GetCoords() : pBullet->SourceCoords;
		const auto delta = destination - source;
		const auto distance = (this->NotMainWeapon || this->TargetInTheAir || (pFirer && pFirer->IsInAir())) ? Point2D{ delta.X, delta.Y }.Magnitude() : delta.Magnitude();

		if (static_cast<int>(distance) >= range)
		{
			if (pType->SuicideAboveRange)
				return true;
			else
				destination = source + delta * (range / distance);
		}
	}

	if (coords.X != 0 || coords.Y != 0)
	{
		switch (pType->TraceMode)
		{
		case TraceTargetMode::Global:
		{
			break;
		}
		case TraceTargetMode::Body:
		{
			if (const auto pTechno = abstract_cast<TechnoClass*>(pType->TraceTheTarget ? pBullet->Target : pBullet->Owner))
			{
				const auto rotateRadian = -(pTechno->PrimaryFacing.Current().GetRadian<32>());
				offset = PhobosTrajectory::Vector2Coord(PhobosTrajectory::HorizontalRotate(coords, rotateRadian));
			}
			else
			{
				offset.X = 0;
				offset.Y = 0;
			}

			break;
		}
		case TraceTargetMode::Turret:
		{
			if (const auto pTechno = abstract_cast<TechnoClass*>(pType->TraceTheTarget ? pBullet->Target : pBullet->Owner))
			{
				const auto rotateRadian = (pTechno->HasTurret() ? -(pTechno->TurretFacing().GetRadian<32>()) : -(pTechno->PrimaryFacing.Current().GetRadian<32>()));
				offset = PhobosTrajectory::Vector2Coord(PhobosTrajectory::HorizontalRotate(coords, rotateRadian));
			}
			else
			{
				offset.X = 0;
				offset.Y = 0;
			}

			break;
		}
		case TraceTargetMode::RotateCW:
		{
			const auto distanceCoords = pBullet->Location - destination;
			const auto radius = Point2D{coords.X,coords.Y}.Magnitude();

			if ((radius * 1.2) > Point2D{distanceCoords.X,distanceCoords.Y}.Magnitude())
			{
				auto rotateRadian = Math::atan2(distanceCoords.Y, distanceCoords.X);

				if (std::abs(radius) > 1e-10)
					rotateRadian += (pType->Speed / radius);

				offset.X = static_cast<int>(radius * Math::cos(rotateRadian));
				offset.Y = static_cast<int>(radius * Math::sin(rotateRadian));
			}
			else
			{
				offset.X = 0;
				offset.Y = 0;
			}

			break;
		}
		case TraceTargetMode::RotateCCW:
		{
			const auto distanceCoords = pBullet->Location - destination;
			const auto radius = Point2D{coords.X,coords.Y}.Magnitude();

			if ((radius * 1.2) > Point2D{distanceCoords.X,distanceCoords.Y}.Magnitude())
			{
				auto rotateRadian = Math::atan2(distanceCoords.Y, distanceCoords.X);

				if (std::abs(radius) > 1e-10)
					rotateRadian -= (pType->Speed / radius);

				offset.X = static_cast<int>(radius * Math::cos(rotateRadian));
				offset.Y = static_cast<int>(radius * Math::sin(rotateRadian));
			}
			else
			{
				offset.X = 0;
				offset.Y = 0;
			}

			break;
		}
		default:
		{
			const auto rotateRadian = this->Get2DOpRadian(pBullet->SourceCoords, pBullet->TargetCoords);
			offset = PhobosTrajectory::Vector2Coord(PhobosTrajectory::HorizontalRotate(coords, rotateRadian));
			break;
		}
		}
	}

	const auto distanceCoords = ((destination + offset) - pBullet->Location);
	const auto distance = distanceCoords.Magnitude();
	this->MovingVelocity = BulletVelocity { static_cast<double>(distanceCoords.X), static_cast<double>(distanceCoords.Y), static_cast<double>(distanceCoords.Z) };
	this->MovingSpeed = distance;

	if (pType->Speed <= distance)
		this->MultiplyBulletVelocity(pType->Speed / distance, false);

	return false;
}
