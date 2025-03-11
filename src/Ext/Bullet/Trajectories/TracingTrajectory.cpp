#include "TracingTrajectory.h"

#include <Ext/Bullet/Body.h>

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

	if (!BulletExt::ExtMap.Find(this->Bullet)->DispersedTrajectory)
		this->OpenFire();
}

bool TracingTrajectory::OnAIDetonateCheck()
{
	if (this->VirtualTrajectory::OnAIDetonateCheck())
		return true;

	const auto pBullet = this->Bullet;
	const auto pType = this->Type;
	auto pFirer = pBullet->Owner;

	if (!pType->TraceTheTarget && !pFirer)
		return true;

	if (pType->SuicideAboveRange)
	{
		if (const auto pWeapon = pBullet->WeaponType)
		{
			auto source = (pFirer && !this->NotMainWeapon) ? pFirer->GetCoords() : pBullet->SourceCoords;
			auto target = pBullet->TargetCoords;

			if (this->NotMainWeapon || this->TargetInTheAir || (pFirer && pFirer->IsInAir()))
			{
				source.Z = 0;
				target.Z = 0;
			}

			if (static_cast<int>(source.DistanceFrom(target)) >= (pWeapon->Range + Unsorted::LeptonsPerCell))
				return true;
		}
	}

	this->ChangeVelocity();

	return false;
}

bool TracingTrajectory::OpenFire()
{
	const auto pBullet = this->Bullet;
	const auto pType = this->Type;
	const auto& theCoords = pType->VirtualSourceCoord.Get();
	auto theOffset = theCoords;

	if (theCoords.X != 0 || theCoords.Y != 0)
	{
		const auto& theSource = pBullet->SourceCoords;
		const auto& theTarget = pBullet->TargetCoords;
		const double rotateRadian = this->Get2DOpRadian(theSource, theTarget);
		theOffset.X = static_cast<int>(theCoords.X * Math::cos(rotateRadian) + theCoords.Y * Math::sin(rotateRadian));
		theOffset.Y = static_cast<int>(theCoords.X * Math::sin(rotateRadian) - theCoords.Y * Math::cos(rotateRadian));
	}

	if (pType->CreateAtTarget)
	{
		if (const auto pTarget = pBullet->Target)
			pBullet->SetLocation(pTarget->GetCoords() + theOffset);
		else
			pBullet->SetLocation(pBullet->TargetCoords + theOffset);
	}
	else
	{
		pBullet->SetLocation(pBullet->SourceCoords + theOffset);
	}

	auto duration = pType->Duration.Get();

	if (duration <= 0)
	{
		if (const auto pWeapon = pBullet->WeaponType)
			duration = (pWeapon->ROF > 10) ? pWeapon->ROF - 10 : 1;
		else
			duration = 120;
	}

	this->DurationTimer.Start(duration);
}

void TracingTrajectory::ChangeVelocity()
{
	const auto pBullet = this->Bullet;
	const auto pType = this->Type;
	auto pFirer = pBullet->Owner;

	if (pFirer)
	{
		for (auto pTrans = pFirer->Transporter; pTrans; pTrans = pTrans->Transporter)
			pFirer = pTrans;
	}

	const auto destination = (pType->TraceTheTarget || !pFirer) ? pBullet->TargetCoords : pFirer->GetCoords();
	const auto theCoords = pType->VirtualTargetCoord.Get();
	auto theOffset = theCoords;

	if (theCoords.X != 0 || theCoords.Y != 0)
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
				const auto rotateAngle = -(pTechno->PrimaryFacing.Current().GetRadian<32>());

				theOffset.X = static_cast<int>(theCoords.X * Math::cos(rotateAngle) + theCoords.Y * Math::sin(rotateAngle));
				theOffset.Y = static_cast<int>(theCoords.X * Math::sin(rotateAngle) - theCoords.Y * Math::cos(rotateAngle));
			}
			else
			{
				theOffset.X = 0;
				theOffset.Y = 0;
			}

			break;
		}
		case TraceTargetMode::Turret:
		{
			if (const auto pTechno = abstract_cast<TechnoClass*>(pType->TraceTheTarget ? pBullet->Target : pBullet->Owner))
			{
				const auto rotateAngle = (pTechno->HasTurret() ? -(pTechno->TurretFacing().GetRadian<32>()) : -(pTechno->PrimaryFacing.Current().GetRadian<32>()));

				theOffset.X = static_cast<int>(theCoords.X * Math::cos(rotateAngle) + theCoords.Y * Math::sin(rotateAngle));
				theOffset.Y = static_cast<int>(theCoords.X * Math::sin(rotateAngle) - theCoords.Y * Math::cos(rotateAngle));
			}
			else
			{
				theOffset.X = 0;
				theOffset.Y = 0;
			}

			break;
		}
		case TraceTargetMode::RotateCW:
		{
			const auto distanceCoords = pBullet->Location - destination;
			const auto radius = Point2D{theCoords.X,theCoords.Y}.Magnitude();

			if ((radius * 1.2) > Point2D{distanceCoords.X,distanceCoords.Y}.Magnitude())
			{
				auto rotateAngle = Math::atan2(distanceCoords.Y, distanceCoords.X);

				if (std::abs(radius) > 1e-10)
					rotateAngle += (pType->Speed / radius);

				theOffset.X = static_cast<int>(radius * Math::cos(rotateAngle));
				theOffset.Y = static_cast<int>(radius * Math::sin(rotateAngle));
			}
			else
			{
				theOffset.X = 0;
				theOffset.Y = 0;
			}

			break;
		}
		case TraceTargetMode::RotateCCW:
		{
			const auto distanceCoords = pBullet->Location - destination;
			const auto radius = Point2D{theCoords.X,theCoords.Y}.Magnitude();

			if ((radius * 1.2) > Point2D{distanceCoords.X,distanceCoords.Y}.Magnitude())
			{
				auto rotateAngle = Math::atan2(distanceCoords.Y, distanceCoords.X);

				if (std::abs(radius) > 1e-10)
					rotateAngle -= (pType->Speed / radius);

				theOffset.X = static_cast<int>(radius * Math::cos(rotateAngle));
				theOffset.Y = static_cast<int>(radius * Math::sin(rotateAngle));
			}
			else
			{
				theOffset.X = 0;
				theOffset.Y = 0;
			}

			break;
		}
		default:
		{
			const auto& theSource = pBullet->SourceCoords;
			const auto& theTarget = pBullet->TargetCoords;

			const auto rotateAngle = Math::atan2(theTarget.Y - theSource.Y , theTarget.X - theSource.X);

			theOffset.X = static_cast<int>(theCoords.X * Math::cos(rotateAngle) + theCoords.Y * Math::sin(rotateAngle));
			theOffset.Y = static_cast<int>(theCoords.X * Math::sin(rotateAngle) - theCoords.Y * Math::cos(rotateAngle));
			break;
		}
		}
	}

	const auto distanceCoords = ((destination + theOffset) - pBullet->Location);
	const auto distance = distanceCoords.Magnitude();
	this->MovingVelocity = BulletVelocity { static_cast<double>(distanceCoords.X), static_cast<double>(distanceCoords.Y), static_cast<double>(distanceCoords.Z) };
	this->MovingSpeed = distance;

	if (distance <= pType->Speed)
		this->MultiplyBulletVelocity(pType->Speed / distance, false);
}
