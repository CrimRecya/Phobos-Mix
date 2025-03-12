#pragma once

#include <BulletClass.h>

#include <Ext/WeaponType/Body.h>
#include <Utilities/EnumFunctions.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/Savegame.h>

enum class TrajectoryFlag : int
{
	Invalid = -1,
	Straight = 0,
	Bombard = 1,
	Missile = 2,
	Engrave = 3,
	Parabola = 4,
	Tracing = 5
};

enum class TrajectoryCheckReturnType : int
{
	ExecuteGameCheck = 0,
	SkipGameCheck = 1,
	SatisfyGameCheck = 2,
	Detonate = 3
};

class PhobosTrajectory;
class PhobosTrajectoryType
{
public:
	PhobosTrajectoryType() :
		Speed { 100.0 }
		, Duration { 0 }
		, TolerantTime { -1 }
		, BulletROT { -1 }
		, BulletSpin { false }
		, BulletStable { false }
		, BulletOnPlane { true }
		, MirrorCoord { true }
		, RetargetRadius { 0 }
		, Synchronize { false }
		, PeacefulVanish {}
		, ApplyRangeModifiers { false }
		, UseDisperseCoord { false }
		, RecordSourceCoord { false }
		, UseDisperseBurst { false }

		, PassDetonate { false }
		, PassDetonateWarhead {}
		, PassDetonateDamage { 0 }
		, PassDetonateDelay { 1 }
		, PassDetonateInitialDelay { 0 }
		, PassDetonateLocal { false }
		, ProximityImpact { 0 }
		, ProximityWarhead {}
		, ProximityDamage { 0 }
		, ProximityRadius { Leptons(179) }
		, ProximityDirect { false }
		, ProximityMedial { false }
		, ProximityAllies { false }
		, ProximityFlight { false }
		, ThroughVehicles { true }
		, ThroughBuilding { true }
		, DamageEdgeAttenuation { 1.0 }
		, DamageCountAttenuation { 1.0 }

		, DisperseWeapons {}
		, DisperseBursts {}
		, DisperseCounts {}
		, DisperseDelays {}
		, DisperseCycle { 0 }
		, DisperseInitialDelay { 0 }
		, DisperseEffectiveRange { Leptons(0) }
		, DisperseSeparate { false }
		, DisperseRetarget { false }
		, DisperseLocation { false }
		, DisperseTendency { false }
		, DisperseHolistic { false }
		, DisperseMarginal { false }
		, DisperseDoRepeat { false }
		, DisperseSuicide { true }
		, DisperseFromFirer {}
		, DisperseFaceCheck { false }
		, DisperseForceFire { true }
		, DisperseCoord { { 0, 0, 0 } }
	{ }

	Valueable<double> Speed; // The speed that a projectile should reach
	Valueable<int> Duration; // The existence time of projectile
	Valueable<int> TolerantTime; // The tolerance time for the projectile to lose its target, after which it will explode
	Valueable<int> BulletROT; // The rotational speed of the projectile image that does not affect the direction of movement
	Valueable<bool> BulletSpin; // Image continue to rotate
	Valueable<bool> BulletStable; // Image no longer rotate after launch
	Valueable<bool> BulletOnPlane; // Image only rotates on the horizontal plane
	Valueable<bool> MirrorCoord; // Should mirror offset
	Valueable<double> RetargetRadius; // Searching for a new target after losing it
	Valueable<bool> Synchronize; // Synchronize the target of its launcher
	Nullable<bool> PeacefulVanish; // Disappear directly when about to detonate
	Valueable<bool> ApplyRangeModifiers; // Apply range bonus
	Valueable<bool> UseDisperseCoord; // Use the recorded launch location
	Valueable<bool> RecordSourceCoord; // Record the launch location
	Valueable<bool> UseDisperseBurst; // Use disperse weapon burst count

	Valueable<bool> PassDetonate; // Detonate the warhead while moving
	Nullable<WarheadTypeClass*> PassDetonateWarhead; // The pass warhead used
	Nullable<int> PassDetonateDamage; // The damage caused by the pass warhead
	Valueable<int> PassDetonateDelay; // Detonation interval
	Valueable<int> PassDetonateInitialDelay; // Detonation initial delay
	Valueable<bool> PassDetonateLocal; // Detonate at ground level
	Valueable<int> ProximityImpact; // How many times can proximity warhead be triggered
	Nullable<WarheadTypeClass*> ProximityWarhead; // The proximity warhead used
	Nullable<int> ProximityDamage; // The damage caused by the proximity warhead
	Valueable<Leptons> ProximityRadius; // How large is the scope of impact
	Valueable<bool> ProximityDirect; // Not detonating the warhead, but directly causing it to receive the damage
	Valueable<bool> ProximityMedial; // When judged as passing through, detonate at bullet position
	Valueable<bool> ProximityAllies; // Does the friendly army accept the judgment
	Valueable<bool> ProximityFlight; // Does the air forces accept the judgment
	Valueable<bool> ThroughVehicles; // Vehicles judged as normal
	Valueable<bool> ThroughBuilding; // Building judged as normal
	Valueable<double> DamageEdgeAttenuation; // The ratio of distance to damage
	Valueable<double> DamageCountAttenuation; // The ratio of count to damage

	ValueableVector<WeaponTypeClass*> DisperseWeapons; // Weapons fired towards the surroundings
	ValueableVector<int> DisperseBursts; // How many times does each weapon burst
	ValueableVector<int> DisperseCounts; // How many times does each group fire
	ValueableVector<int> DisperseDelays; // Cooling time after weapon launch
	Valueable<int> DisperseCycle; // How many rounds of weapons can be fired
	Valueable<int> DisperseInitialDelay; // How long will it take to start firing weapons
	Valueable<Leptons> DisperseEffectiveRange; // How close should it get before start firing weapons
	Valueable<bool> DisperseSeparate; // Launch by weapon or by group
	Valueable<bool> DisperseRetarget; // Automatically research for targets
	Valueable<bool> DisperseLocation; // Where to search for the enemy from
	Valueable<bool> DisperseTendency; // The every first weapon will attack the original target
	Valueable<bool> DisperseHolistic; // Can select targets from different locations
	Valueable<bool> DisperseMarginal; // Can attack trees, stones, bullets, etc
	Valueable<bool> DisperseDoRepeat; // Can repeatedly attack a same target
	Valueable<bool> DisperseSuicide; // Self destruct after all weapons are launched
	Nullable<bool> DisperseFromFirer; // Fire from the firer's position
	Valueable<bool> DisperseFaceCheck; // Check the orientation before launching the weapon
	Valueable<bool> DisperseForceFire; // Ignore the no target state before launching the weapon
	Valueable<PartialVector3D<int>> DisperseCoord; // The firing position when fired from the bullet

	virtual ~PhobosTrajectoryType() noexcept = default;
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	virtual bool Save(PhobosStreamWriter& Stm) const;
	virtual TrajectoryFlag Flag() const { return TrajectoryFlag::Invalid; }
	virtual void Read(CCINIClass* const pINI, const char* pSection);
	[[nodiscard]] virtual std::unique_ptr<PhobosTrajectory> CreateInstance(BulletClass* pBullet) const = 0;

private:
	template <typename T>
	void Serialize(T& Stm);
};

class PhobosTrajectory
{
public:
	PhobosTrajectory() { }
	PhobosTrajectory(PhobosTrajectoryType const* trajType, BulletClass* pBullet) :
		Bullet { pBullet }
		, MovingVelocity { BulletVelocity::Empty }
		, MovingSpeed { 0 }
		, DurationTimer {}
		, TolerantTimer {}
		, FirepowerMult { 1.0 }
		, AttenuationRange { 0 }
		, RemainingDistance { 1 }
		, TargetInTheAir { false }
		, TargetIsTechno { false }
		, NotMainWeapon { false }
		, ShouldDetonate { false }
		, FLHCoord { CoordStruct::Empty }
		, BuildingCoord { CoordStruct::Empty }
		, CurrentBurst { 0 }
		, CountOfBurst { 0 }

		, PassDetonateDamage { 0 }
		, PassDetonateTimer {}
		, ProximityImpact { trajType->ProximityImpact }
		, ProximityDamage { 0 }
		, ExtraCheck { nullptr }
		, TheCasualty {}

		, DisperseIndex { 0 }
		, DisperseCount { 0 }
		, DisperseCycle { trajType->DisperseCycle }
		, DisperseTimer {}
	{ }

	BulletClass* Bullet; // Bullet attached to
	BulletVelocity MovingVelocity; // The vector used for calculating speed
	double MovingSpeed; // The current speed value
	CDTimerClass DurationTimer; // Bullet existence timer
	CDTimerClass TolerantTimer; // Target tolerance timer
	double FirepowerMult; // Inherited firepower bonus
	int AttenuationRange; // Maximum range
	int RemainingDistance; // Remaining distance from the self explosion location
	bool TargetInTheAir; // Is the original target the Air Force
	bool TargetIsTechno; // Is the original target a techno type
	bool NotMainWeapon; // Does it ignore the launcher
	bool ShouldDetonate; // Should detonate in next frame
	CoordStruct FLHCoord; // Launch FLH
	CoordStruct BuildingCoord; // Offset on the building of launch FLH
	int CurrentBurst; // Current burst index
	int CountOfBurst; // Upper limit of burst counts

	int PassDetonateDamage; // Current damage caused by the pass warhead
	CDTimerClass PassDetonateTimer; // Detonation interval timer
	int ProximityImpact; // How many times can proximity warhead be triggered
	int ProximityDamage; // Current damage caused by the proximity warhead
	TechnoClass* ExtraCheck; // The obstacle, no taken out for use in next frame
	std::map<int, int> TheCasualty; // <UniqueID, Frames>, only for recording existence to check whether have damaged

	int DisperseIndex; // Launch weapon group Index
	int DisperseCount; // Launch weapon group remaining times
	int DisperseCycle; // Launch weapon times remaining rounds
	CDTimerClass DisperseTimer; // Cooling timer for launching weapons

	virtual ~PhobosTrajectory() noexcept = default;
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	virtual bool Save(PhobosStreamWriter& Stm) const;
	virtual TrajectoryFlag Flag() const { return TrajectoryFlag::Invalid; }
	virtual void OnUnlimbo();
	virtual bool OnAI();
	virtual bool OnAIDetonateCheck();
	virtual void OnAIVelocityCheck();
	virtual void OnAINextFrameCheck();
	virtual void OnAIPreDetonate();
	virtual void OnAIVelocity(BulletVelocity* pSpeed, BulletVelocity* pPosition) { *pSpeed = this->MovingVelocity; }
	virtual TrajectoryCheckReturnType OnAITargetCoordCheck() { return TrajectoryCheckReturnType::SkipGameCheck; }
	virtual TrajectoryCheckReturnType OnAITechnoCheck(TechnoClass* pTechno) { return TrajectoryCheckReturnType::SkipGameCheck; }
	virtual const PhobosTrajectoryType* GetType() const = 0;
	virtual void OpenFire();
	virtual bool GetCanHitGround() const { return true; }
	virtual CoordStruct GetRetargetCenter() const { return this->Bullet->TargetCoords; }
	virtual void SetBulletNewTarget(AbstractClass* const pTarget);
	virtual bool CalculateBulletVelocity(const double speed);
	virtual void MultiplyBulletVelocity(const double ratio, const bool shouldDetonate);

	static inline double Get2DDistance(const CoordStruct& source, const CoordStruct& target)
	{
		return Point2D { source.X, source.Y }.DistanceFrom(Point2D { target.X, target.Y });
	}
	static inline double Get2DVelocity(const BulletVelocity& velocity)
	{
		return Vector2D<double>{ velocity.X, velocity.Y }.Magnitude();
	}
	static inline double Get2DOpRadian(const CoordStruct& source, const CoordStruct& target)
	{
		return Math::atan2(target.Y - source.Y , target.X - source.X);
	}
	static inline void SetNewDamage(int& damage, const double ratio)
	{
		if (damage)
		{
			if (const auto newDamage = static_cast<int>(damage * ratio))
				damage = newDamage;
			else
				damage = Math::sgn(damage);
		}
	}
	static inline bool CheckTechnoIsInvalid(const TechnoClass* const pTechno)
	{
		// The target is alive
		return (!pTechno->IsAlive || !pTechno->IsOnMap || pTechno->InLimbo || pTechno->IsSinking || pTechno->Health <= 0);
	}
	static inline bool CheckWeaponCanTarget(const WeaponTypeExt::ExtData* const pWeaponExt, TechnoClass* const pFirer, TechnoClass* const pTarget)
	{
		// No check for CanTargetHouses
		return !pWeaponExt || (EnumFunctions::IsTechnoEligible(pTarget, pWeaponExt->CanTarget) && pWeaponExt->HasRequiredAttachedEffects(pTarget, pFirer));
	}
	static inline bool CheckWeaponValidness(HouseClass* const pHouse, const TechnoClass* const pTechno, const CellClass* const pCell, const AffectedHouse flags)
	{
		if (pHouse == pTechno->Owner)
			return (flags & AffectedHouse::Owner) != AffectedHouse::None;
		else if (pHouse->IsAlliedWith(pTechno->Owner) || pTechno->IsDisguisedAs(pHouse))
			return (flags & AffectedHouse::Allies) != AffectedHouse::None;
		else if ((flags & AffectedHouse::Enemies) == AffectedHouse::None)
			return false;

		return pTechno->CloakState != CloakState::Cloaked || pCell->Sensors_InclHouse(pHouse->ArrayIndex);
	}
	static std::vector<CellStruct> GetCellsInRectangle(const CellStruct bottomStaCell, const CellStruct leftMidCell, const CellStruct rightMidCell, const CellStruct topEndCell);

	void ChangeBulletFacing();
	bool CheckTolerantAndSynchronize();
	std::vector<CellClass*> GetCellsInProximityRadius();
	bool BulletRetargetTechno();
	bool CheckThroughAndSubjectInCell(CellClass* pCell, HouseClass* pOwner);
	void CalculateNewDamage();
	void PassWithDetonateAt();
	void PrepareForDetonateAt();
	void ProximityDetonateAt(HouseClass* pOwner, TechnoClass* pTarget);
	int GetTheTrueDamage(int damage, bool self);
	double GetExtraDamageMultiplier();
	void GetTechnoFLHCoord();
	CoordStruct GetWeaponFireCoord(TechnoClass* pTechno);
	bool CheckFireFacing();
	bool PrepareDisperseWeapon();
	bool FireDisperseWeapon(TechnoClass* pFirer, const CoordStruct& sourceCoord, HouseClass* pOwner);
	void CreateDisperseBullets(TechnoClass* pTechno, const CoordStruct& sourceCoord, WeaponTypeClass* pWeapon, AbstractClass* pTarget, HouseClass* pOwner, int curBurst, int maxBurst);

private:
	template <typename T>
	void Serialize(T& Stm);
};

class LiveShellTrajectoryType : public PhobosTrajectoryType
{
public:
	LiveShellTrajectoryType() : PhobosTrajectoryType()
		, RotateCoord { 0 }
		, OffsetCoord { { 0, 0, 0 } }
		, AxisOfRotation { { 0, 0, 1 } }
		, LeadTimeCalculate { false }
		, SubjectToGround { false }
		, EarlyDetonation { false }
		, DetonationHeight { -1 }
		, DetonationDistance { Leptons(102) }
		, TargetSnapDistance { Leptons(128) }
	{ }

	Valueable<double> RotateCoord; // The maximum rotation angle of the initial velocity vector on the axis of rotation
	Valueable<PartialVector3D<int>> OffsetCoord; // Offset of target position, refers to the initial target position on Missile
	Valueable<PartialVector3D<int>> AxisOfRotation; // RotateCoord's rotation axis
	Valueable<bool> LeadTimeCalculate; // Predict the moving direction of the target
	bool SubjectToGround; // Auto set
	Valueable<bool> EarlyDetonation; // Calculating DetonationHeight in the rising phase rather than the falling phase
	Valueable<int> DetonationHeight; // At what height did it detonate in advance
	Valueable<Leptons> DetonationDistance; // Explode at a distance from the target, different on AAA and BBB
	Valueable<Leptons> TargetSnapDistance; // Snap to target when detonating with a distance less than this

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
//	virtual void Read(CCINIClass* const pINI, const char* pSection) override; // Read separately

private:
	template <typename T>
	void Serialize(T& Stm);
};

class LiveShellTrajectory : public PhobosTrajectory
{
public:
	LiveShellTrajectory() { }
	LiveShellTrajectory(LiveShellTrajectoryType const* trajType, BulletClass* pBullet)
		: PhobosTrajectory(trajType, pBullet)
		, LastTargetCoord { CoordStruct::Empty }
		, WaitOneFrame { 0 }
	{ }

	// TODO If we could calculate this before firing, perhaps it can solve the problem of one frame delay and not so correct turret orientation.
	CoordStruct LastTargetCoord; // The target is located in the previous frame, used to calculate the lead time
	int WaitOneFrame; // Attempts to launch when update

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual void OnUnlimbo() override;
	virtual bool OnAI() override;
	virtual void OnAIPreDetonate() override;
	virtual void FireTrajectory() { this->OpenFire(); } // New

	static BulletVelocity RotateAboutTheAxis(const BulletVelocity& theSpeed, BulletVelocity& theAxis, double theRadian);

	bool BulletPrepareCheck();
	CoordStruct GetOnlyStableOffsetCoords(double rotateRadian);
	CoordStruct GetInaccurateTargetCoords(const CoordStruct& baseCoord, double distance);
	void DisperseBurstSubstitution(double baseRadian);

private:
	template <typename T>
	void Serialize(T& Stm);
};

class VirtualTrajectoryType : public PhobosTrajectoryType
{
public:
	VirtualTrajectoryType() : PhobosTrajectoryType()
		, VirtualSourceCoord { { 0, 0, 0 } }
		, VirtualTargetCoord { { 0, 0, 0 } }
		, AllowFirerTurning { true }
		, IgnoresFirestorm { true }
	{ }

	Valueable<PartialVector3D<int>> VirtualSourceCoord; // Initial location of the projectile
	Valueable<PartialVector3D<int>> VirtualTargetCoord; // move to location of the projectile
	Valueable<bool> AllowFirerTurning; // Allow firer not facing projectiles
	bool IgnoresFirestorm; // Auto set

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
//	virtual void Read(CCINIClass* const pINI, const char* pSection) override; // Read separately

private:
	template <typename T>
	void Serialize(T& Stm);
};

class VirtualTrajectory : public PhobosTrajectory
{
public:
	VirtualTrajectory() { }
	VirtualTrajectory(VirtualTrajectoryType const* trajType, BulletClass* pBullet)
		: PhobosTrajectory(trajType, pBullet)
		, SurfaceFirerID { 0 }
	{ }

	DWORD SurfaceFirerID; // UniqueID of the "launcher"

	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange) override;
	virtual bool Save(PhobosStreamWriter& Stm) const override;
	virtual void OnUnlimbo() override;
	virtual bool OnAI() override;
	virtual bool OnAIDetonateCheck() override;

	bool InvalidFireCondition(TechnoClass* pTechno);

private:
	template <typename T>
	void Serialize(T& Stm);
};

/*
* This is a guidance to tell you how to add your trajectory here
* Firstly, we just image the game coordinate into a 2D-plain
*
* ZAxis
*   |			        	 TargetCoord
*   |
*   |
*   |
*   |
*   |  SourceCoord
*   O-------------------------------------XYPlain
*
* Then our problem just turns into:
* Find an equation whose curve just passes both two coord
* And the curve is just your trajectory
*
* Luckily, what we need to implement this is just calculate the velocity during the movement
* So a possible way is to find out the equation and find the derivative of it, which is just the velocity
* And the just code on it!
*
* There is a SampleTrajectory already, you can just copy it and do some edits.
* Following that, you can create a fun trajectory very easily. - secsome
*
*                                           ^*##^
*                *###$                     *##^*#*
*               ##^ $##                  ^##^   ^##
*              ##     ##$               ^##      ^##
*             ##       $#*  ^^^  ^^^^^^$#*         ##    ^$*###################*$^
*            *#^        ^###############$          ^#######*$^    ^#*****#^  ^$*####$^
*           ^#$          $##^  ##*  $##^            ^*$            #*****#       ^#####*^
*           ##                                                     $#####^       *#***####$^
*          ##                                            $###$       ^$^         ^#####* ^###^
*  *#**$  ^#^                                         *###$^                      ^$$$$     *##
*  $$$*#####                                          ^^                                      ##*
*        $#^       ^###*        *$        ####         $*####*^                                ^##
* ^$***$$#*        $####        ##       ^####^       ^**$$$*#^                                  ##^
* $#***$##^         ^$^      $######^      $$                                                     ##^
*       ##                    $^  ^^                                                               ##
*      $#^                                                                                          ##
*      ##                                                                                           $#^
*     ^#$                                                                                            ##
*     *#                                                                                             ^#$
*     ##                                                                                              ##
*     #*                                                                                              $#
*    ^#$                                                                                               ###################*^
*    $#                                                                                                $###^  ^#**#^   #*###*
*    $#                                                                                                $***    ****    **$*##
*    $#                                                                                                $###^   *#*#^   #####$
*    $#                                                                                                *#**##############*$
*    $#                                                                                                #*
*    ^#^                                                                                              ^#^
*     #$                                                                                              *#
*     ##                                                                                              #*
*     *#^                                                                                            *#
*      ##                                                                                            #*
*      $#^                                                                                          ##
*       ##                                                                                         *#^
*       ^##                                                                                       $#$
*        ^##                                                                                     $#*
*          ##                                                                                   $#*
*           ##*                                                                                *#*
*            ^##$                                                                             ##^
*              $##$                                                                         *##
*                $##*^                                                                   ^###^
*                  ^##  ^###**$$$$$$$$$$$   ^$$$$$$$$$$$$$$$$$$****$   *##############  ^##$
*                    #####$$*****#########^##*#########*#**###*****##$##$^$$$$^$$^^^^##*##
*                     ^$^                *##^                       ***               ^$
*
*/

// I removed most part but kept a few so that you know what you are eating
class TrajectoryTypePointer
{
	std::unique_ptr<PhobosTrajectoryType> _ptr {};
public:
	explicit TrajectoryTypePointer(TrajectoryFlag flag);
	explicit TrajectoryTypePointer() { }
	TrajectoryTypePointer(const TrajectoryTypePointer&) = delete;
	TrajectoryTypePointer& operator=(const TrajectoryTypePointer&) = delete;
	void LoadFromINI(CCINIClass* pINI, const char* pSection);
	bool Load(PhobosStreamReader& stm, bool registerForChange);
	bool Save(PhobosStreamWriter& stm) const;
	[[nodiscard]] PhobosTrajectoryType* operator->() const noexcept { return _ptr.get(); }
	[[nodiscard]] PhobosTrajectoryType* get() const noexcept { return _ptr.get(); }
	operator bool() const noexcept { return _ptr.get() != nullptr; }
};

class TrajectoryPointer
{
	std::unique_ptr<PhobosTrajectory> _ptr;
public:
	TrajectoryPointer(std::nullptr_t) : _ptr { nullptr } { }
	TrajectoryPointer(const TrajectoryPointer&) = delete;
	TrajectoryPointer& operator=(const TrajectoryPointer&) = delete;
	TrajectoryPointer& operator=(std::unique_ptr<PhobosTrajectory>&& ptr) noexcept { _ptr = std::move(ptr); return *this; }
	bool Load(PhobosStreamReader& stm, bool registerForChange);
	bool Save(PhobosStreamWriter& stm) const;
	[[nodiscard]] PhobosTrajectory* operator->() const noexcept { return _ptr.get(); }
	[[nodiscard]] PhobosTrajectory* get() const noexcept { return _ptr.get(); }
	operator bool() const noexcept { return _ptr.get() != nullptr; }
	operator std::unique_ptr<PhobosTrajectory>() noexcept { return std::move(_ptr); }
};
