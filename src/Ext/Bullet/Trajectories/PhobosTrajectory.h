#pragma once

#include <BulletClass.h>

#include <Ext/WeaponType/Body.h>
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
		, TargetSnapDistance { 0.5 }

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

		, RotateCoord { 0 }
		, OffsetCoord { { 0, 0, 0 } }
		, AxisOfRotation { { 0, 0, 1 } }
		, UseDisperseBurst { false }
		, LeadTimeCalculate { false }
		, EarlyDetonation { false }
		, DetonationHeight { -1 }
		, DetonationDistance { Leptons(102) }

		, VirtualSourceCoord { { 0, 0, 0 } }
		, VirtualTargetCoord { { 0, 0, 0 } }
		, AllowFirerTurning { true }
	{ }

	virtual ~PhobosTrajectoryType() noexcept = default;
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	virtual bool Save(PhobosStreamWriter& Stm) const;
	virtual TrajectoryFlag Flag() const { return TrajectoryFlag::Invalid; }
	virtual void Read(CCINIClass* const pINI, const char* pSection);
	[[nodiscard]] virtual std::unique_ptr<PhobosTrajectory> CreateInstance() const = 0;

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	Valueable<double> Speed; // The speed that a projectile should reach
	Valueable<int> Duration; // The existence time of projectile
	Valueable<int> TolerantTime; // The tolerance time for the projectile to lose its target, after which it will explode
	Valueable<int> BulletROT; // The rotational speed of the projectile image that does not affect the direction of movement
	Valueable<bool> BulletSpin; // Image continue to rotate
	Valueable<bool> BulletStable; // Image no longer rotate after launch
	Valueable<bool> BulletOnPlane; // Image only rotates on the horizontal plane
	Valueable<bool> MirrorCoord; // Mirror offset
	Valueable<double> RetargetRadius; // Searching for a new target after losing it
	Valueable<bool> Synchronize; // Synchronize the target of its launcher
	Nullable<bool> PeacefulVanish; // Disappear directly when about to detonate
	Valueable<bool> ApplyRangeModifiers; // Apply range bonus
	Valueable<bool> UseDisperseCoord; // Use the recorded launch location
	Valueable<bool> RecordSourceCoord; // Record the launch location
	Valueable<double> TargetSnapDistance; // The distance that can be adsorbed onto the target

	Valueable<bool> PassDetonate;
	Valueable<WarheadTypeClass*> PassDetonateWarhead;
	Valueable<int> PassDetonateDamage;
	Valueable<int> PassDetonateDelay;
	Valueable<int> PassDetonateInitialDelay;
	Valueable<bool> PassDetonateLocal;
	Valueable<int> ProximityImpact;
	Valueable<WarheadTypeClass*> ProximityWarhead;
	Valueable<int> ProximityDamage;
	Valueable<Leptons> ProximityRadius;
	Valueable<bool> ProximityDirect;
	Valueable<bool> ProximityMedial;
	Valueable<bool> ProximityAllies;
	Valueable<bool> ProximityFlight;
	Valueable<bool> ThroughVehicles;
	Valueable<bool> ThroughBuilding;
	Valueable<double> DamageEdgeAttenuation;
	Valueable<double> DamageCountAttenuation;

	ValueableVector<WeaponTypeClass*> DisperseWeapons;
	ValueableVector<int> DisperseBursts;
	ValueableVector<int> DisperseCounts;
	ValueableVector<int> DisperseDelays;
	Valueable<int> DisperseCycle;
	Valueable<int> DisperseInitialDelay;
	Valueable<Leptons> DisperseEffectiveRange;
	Valueable<bool> DisperseSeparate;
	Valueable<bool> DisperseRetarget;
	Valueable<bool> DisperseLocation;
	Valueable<bool> DisperseTendency;
	Valueable<bool> DisperseHolistic;
	Valueable<bool> DisperseMarginal;
	Valueable<bool> DisperseDoRepeat;
	Valueable<bool> DisperseSuicide;
	Nullable<bool> DisperseFromFirer;
	Valueable<bool> DisperseFaceCheck;
	Valueable<bool> DisperseForceFire;
	Valueable<PartialVector3D<int>> DisperseCoord;

	Valueable<double> RotateCoord;
	Valueable<PartialVector3D<int>> OffsetCoord;
	Valueable<PartialVector3D<int>> AxisOfRotation;
	Valueable<bool> UseDisperseBurst;
	Valueable<bool> LeadTimeCalculate;
	Valueable<bool> EarlyDetonation;
	Valueable<int> DetonationHeight;
	Valueable<Leptons> DetonationDistance;

	Valueable<PartialVector3D<int>> VirtualSourceCoord;
	Valueable<PartialVector3D<int>> VirtualTargetCoord;
	Valueable<bool> AllowFirerTurning;
};

class PhobosTrajectory
{
public:
	PhobosTrajectory(PhobosTrajectoryType const* trajType) :
		DurationTimer {}
		, TolerantTimer {}
		, FirepowerMult { 1.0 }
		, AttenuationRange { 0 }
		, FLHCoord {}
		, BuildingCoord {}

		, PassDetonateDamage { trajType->PassDetonateDamage }
		, PassDetonateTimer {}
		, ProximityImpact { trajType->ProximityImpact }
		, ProximityDamage { trajType->ProximityDamage }
		, ExtraCheck { nullptr }
		, TheCasualty {}

		, DisperseIndex { 0 }
		, DisperseCount { 0 }
		, DisperseCycle { trajType->DisperseCycle }
		, DisperseTimer {}
		, TargetInTheAir { false }
		, TargetIsTechno { false }

		, OffsetCoord {}
		, LastTargetCoord {}
		, CurrentBurst { 0 }
		, CountOfBurst { 0 }
		, WaitOneFrame { 0 }
		, SubjectToGround { false }

		, VirtualSourceCoord {}
		, VirtualTargetCoord {}
		, TechnoInTransport { 0 }
		, NotMainWeapon { false }
	{ }

	virtual ~PhobosTrajectory() noexcept = default;
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	virtual bool Save(PhobosStreamWriter& Stm) const;
	virtual TrajectoryFlag Flag() const { return TrajectoryFlag::Invalid; }
	virtual void OnUnlimbo(BulletClass* pBullet, CoordStruct* pCoord, BulletVelocity* pVelocity);
	virtual bool OnAI(BulletClass* pBullet);
	virtual bool OnAIPreCheck(BulletClass* pBullet, HouseClass* pOwner);
	virtual void OnAIVelocityCheck(BulletClass* pBullet, HouseClass* pOwner);
	virtual void OnAILastCheck(BulletClass* pBullet, HouseClass* pOwner);
	virtual void OnAIPreDetonate(BulletClass* pBullet) = 0;
	virtual void OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition) = 0;
	virtual TrajectoryCheckReturnType OnAITargetCoordCheck(BulletClass* pBullet) = 0;
	virtual TrajectoryCheckReturnType OnAITechnoCheck(BulletClass* pBullet, TechnoClass* pTechno) = 0;
	virtual const PhobosTrajectoryType* GetType() const { return nullptr; }
	virtual bool GetCanHitGround() const { return true; }
	virtual CoordStruct GetRetargetCenter(const BulletClass* const pBullet) const { return pBullet->Location; }
	virtual void SetBulletNewTarget(BulletClass* const pBullet, AbstractClass* const pTarget) = 0;

	static inline double Get2DDistance(const CoordStruct& here, const CoordStruct& there);
	static inline double Get2DVelocity(const BulletVelocity& velocity);
	static inline void SetNewDamage(int& damage, double ratio);
	static inline bool CheckTechnoIsInvalid(TechnoClass* pTechno);
	static inline bool CheckWeaponCanTarget(WeaponTypeExt::ExtData* pWeaponExt, TechnoClass* pFirer, TechnoClass* pTarget);
	static inline bool CheckWeaponValidness(HouseClass* pHouse, TechnoClass* pTechno, CellClass* pCell, AffectedHouse flags);
	static std::vector<CellClass*> GetCellsInProximityRadius(const BulletClass* const pBullet, const Leptons trajectoryProximityRange);
	static std::vector<CellStruct> GetCellsInRectangle(const CellStruct bottomStaCell, const CellStruct leftMidCell, const CellStruct rightMidCell, const CellStruct topEndCell);
	static BulletVelocity RotateAboutTheAxis(const BulletVelocity& theSpeed, BulletVelocity& theAxis, double theRadian);
	static void DisperseBurstSubstitution(BulletClass* pBullet, const CoordStruct& axis, double rotateCoord, int curBurst, int maxBurst, bool mirror);

	bool BulletRetargetTechno(BulletClass* pBullet);
	bool CheckThroughAndSubjectInCell(BulletClass* pBullet, CellClass* pCell, HouseClass* pOwner);
	void CalculateNewDamage(BulletClass* pBullet);
	void PassWithDetonateAt(BulletClass* pBullet, HouseClass* pOwner);
	void PrepareForDetonateAt(BulletClass* pBullet, HouseClass* pOwner);
	int GetTheTrueDamage(int damage, BulletClass* pBullet, TechnoClass* pTechno, bool self);
	double GetExtraDamageMultiplier(BulletClass* pBullet, TechnoClass* pTechno);
	void GetTechnoFLHCoord(BulletClass* pBullet, TechnoClass* pTechno);
	CoordStruct GetWeaponFireCoord(BulletClass* pBullet, TechnoClass* pTechno);
	bool CheckFireFacing(BulletClass* pBullet);
	bool PrepareDisperseWeapon(BulletClass* pBullet);
	void FireDisperseWeapon(BulletClass* pBullet, TechnoClass* pFirer, const CoordStruct& sourceCoord, HouseClass* pOwner);
	void CreateDisperseBullets(TechnoClass* pTechno, const CoordStruct& sourceCoord, WeaponTypeClass* pWeapon, AbstractClass* pTarget, HouseClass* pOwner, int curBurst, int maxBurst);

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	BulletVelocity MovingVelocity;
	CDTimerClass DurationTimer;
	CDTimerClass TolerantTimer;
	double FirepowerMult;
	int AttenuationRange;
	int RemainingDistance;
	CoordStruct FLHCoord;
	CoordStruct BuildingCoord;

	int PassDetonateDamage;
	CDTimerClass PassDetonateTimer;
	int ProximityImpact;
	int ProximityDamage;
	TechnoClass* ExtraCheck; // No taken out for use in next frame
	std::map<int, int> TheCasualty; // Only for recording existence

	int DisperseIndex;
	int DisperseCount;
	int DisperseCycle;
	CDTimerClass DisperseTimer;
	bool TargetInTheAir;
	bool TargetIsTechno;

	CoordStruct OffsetCoord;
	CoordStruct LastTargetCoord;
	int CurrentBurst;
	int CountOfBurst;
	int WaitOneFrame;
	bool SubjectToGround;

	CoordStruct VirtualSourceCoord;
	CoordStruct VirtualTargetCoord;
	DWORD TechnoInTransport;
	bool NotMainWeapon;
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
