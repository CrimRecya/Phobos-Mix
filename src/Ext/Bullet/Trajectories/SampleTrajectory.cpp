#include "SampleTrajectory.h"

#include <Ext/Bullet/Body.h>

// Create
std::unique_ptr<PhobosTrajectory> SampleTrajectoryType::CreateInstance(BulletClass* pBullet) const
{
	return std::make_unique<SampleTrajectory>(this, pBullet);
}

// Save and Load for type
template<typename T>
void SampleTrajectoryType::Serialize(T& Stm)
{
	Stm
		.Process(this->TargetSnapDistance)
		;
}

bool SampleTrajectoryType::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->PhobosTrajectoryType::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool SampleTrajectoryType::Save(PhobosStreamWriter& Stm) const
{
	this->PhobosTrajectoryType::Save(Stm);
	const_cast<SampleTrajectoryType*>(this)->Serialize(Stm);
	return true;
}

// INI reading stuff
void SampleTrajectoryType::Read(CCINIClass* const pINI, const char* pSection)
{
	this->PhobosTrajectoryType::Read(pINI, pSection);
	INI_EX exINI(pINI);

	this->TargetSnapDistance.Read(exINI, pSection, "Trajectory.Sample.TargetSnapDistance");
}

// Save and Load for entity
template<typename T>
void SampleTrajectory::Serialize(T& Stm)
{
	Stm
		.Process(this->Type)
		.Process(this->TargetSnapDistance)
		;
}

bool SampleTrajectory::Load(PhobosStreamReader& Stm, bool RegisterForChange)
{
	this->PhobosTrajectory::Load(Stm, false);
	this->Serialize(Stm);
	return true;
}

bool SampleTrajectory::Save(PhobosStreamWriter& Stm) const
{
	this->PhobosTrajectory::Save(Stm);
	const_cast<SampleTrajectory*>(this)->Serialize(Stm);
	return true;
}

// Record some information for your bullet.
void SampleTrajectory::OnUnlimbo()
{
	this->PhobosTrajectory::OnUnlimbo();

	if (!BulletExt::ExtMap.Find(this->Bullet)->DispersedTrajectory)
		this->OpenFire();
}

// Some early checks here, returns whether or not to detonate the bullet.
// You can change the bullet's true velocity or set its location here. If you modify them here, it will affect the incoming parameters in OnAIVelocity.
bool SampleTrajectory::OnAI()
{
	const auto pBullet = this->Bullet;
	const auto pFirer = pBullet->Owner;
	const auto pOwner = pFirer ? pFirer->Owner : BulletExt::ExtMap.Find(pBullet)->FirerHouse;

	if (this->OnAIPreCheck(pOwner))
		return true;

	this->OnAIVelocityCheck(pOwner);

	if (this->PhobosTrajectory::OnAI())
		return true;

	this->OnAILastCheck(pOwner);

	return false;
}

// If the projectile needs to update its speed during the journey, then I suggest you complete the update here.
bool SampleTrajectory::OnAIPreCheck(HouseClass* pOwner)
{
	const auto pBullet = this->Bullet;
	const auto distance = pBullet->TargetCoords.DistanceFrom(pBullet->Location);
	const auto velocity = pBullet->Velocity.Magnitude();

	if (distance < velocity)
		pBullet->Velocity *= (distance / velocity);

	return this->PhobosTrajectory::OnAIPreCheck(pOwner);
}

// What needs to be done before launching the weapon after calculating the new speed.
void SampleTrajectory::OnAIVelocityCheck(HouseClass* pOwner)
{
	this->PhobosTrajectory::OnAIVelocityCheck(pOwner);
}

// What else should be done after the weapon is launched, to prepare for the next frame.
void SampleTrajectory::OnAILastCheck(HouseClass* pOwner)
{
	this->PhobosTrajectory::OnAILastCheck(pOwner);
}

// At this time, the bullet has hit the target and is ready to detonate.
// You can make it change before detonating.
void SampleTrajectory::OnAIPreDetonate()
{
	const auto pBullet = this->Bullet;
	auto pTarget = abstract_cast<ObjectClass*>(pBullet->Target);
	auto pCoords = pTarget ? pTarget->GetCoords() : pBullet->TargetCoords;

	if (pCoords.DistanceFrom(pBullet->Location) <= this->TargetSnapDistance)
	{
		BulletExt::ExtMap.Find(pBullet)->SnappedToTarget = true;
		pBullet->SetLocation(pCoords);
	}

	this->PhobosTrajectory::OnAIPreDetonate();
}

// Where you can update the bullet's speed and position. But I would recommend that you complete the calculation at OnAI().
// pSpeed: From the basic `Velocity` of the bullet plus gravity. It is only used in the calculation of this frame and will not be retained to the next frame.
// pPosition: From the current `Location` of the bullet, then the bullet will be set location to (*pSpeed + *pPosition). So don't use SetLocation here.
// You can also do additional processing here so that the position of the bullet will not change with its true velocity.
void SampleTrajectory::OnAIVelocity(BulletVelocity* pSpeed, BulletVelocity* pPosition)
{
	this->PhobosTrajectory::OnAIVelocity(pSpeed, pPosition);
}

// Where additional checks based on bullet reaching its target coordinate can be done.
// Vanilla code will do additional checks regarding buildings on target coordinate and Vertical projectiles and will detonate the projectile if they pass.
// Return value determines what is done regards to the game checks: they can be skipped, executed as normal or treated as if the condition is already satisfied.
TrajectoryCheckReturnType SampleTrajectory::OnAITargetCoordCheck()
{
	return TrajectoryCheckReturnType::ExecuteGameCheck; // Execute game checks.
}

// Where additional checks based on a TechnoClass instance in same cell as the bullet can be done.
// Vanilla code will do additional trajectory alterations here if there is an enemy techno in the cell.
// Return value determines what is done regards to the game checks: they can be skipped, executed as normal or treated as if the condition is already satisfied.
// pTechno: TechnoClass instance in same cell as the bullet. Note that you should first check whether it is a nullptr.
TrajectoryCheckReturnType SampleTrajectory::OnAITechnoCheck(TechnoClass* pTechno)
{
	return TrajectoryCheckReturnType::ExecuteGameCheck; // Execute game checks.
}

// Do some math here to set the initial speed or location of your bullet.
// Be careful not to let the bullet speed too fast without other processing.
bool SampleTrajectory::OpenFire()
{
	const auto pBullet = this->Bullet;
	pBullet->Velocity.X = static_cast<double>(pBullet->TargetCoords.X - pBullet->SourceCoords.X);
	pBullet->Velocity.Y = static_cast<double>(pBullet->TargetCoords.Y - pBullet->SourceCoords.Y);
	pBullet->Velocity.Z = static_cast<double>(pBullet->TargetCoords.Z - pBullet->SourceCoords.Z);
	pBullet->Velocity *= this->Type->Speed / pBullet->Velocity.Magnitude();
}

// Does the projectile detonate when it lands below the ground
bool SampleTrajectory::GetCanHitGround() const
{
	return true;
}

// If need to research a target, where is the search center
CoordStruct SampleTrajectory::GetRetargetCenter() const
{
	return this->Bullet->TargetCoords;
}

// How to calculate when inputting velocity values after updating the velocity vector each time
bool SampleTrajectory::CalculateBulletVelocity()
{
	return this->PhobosTrajectory::CalculateBulletVelocity();
};

// How to do when should change to a new target
void SampleTrajectory::SetBulletNewTarget(AbstractClass* const pTarget)
{
	this->PhobosTrajectory::SetBulletNewTarget(pTarget);
}
