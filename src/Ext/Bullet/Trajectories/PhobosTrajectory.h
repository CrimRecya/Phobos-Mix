#pragma once

#include <Utilities/TemplateDef.h>
#include <Utilities/Savegame.h>

#include <BulletClass.h>

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
		, RetargetRadius { 0 }
		, Synchronize { false }
		, PeacefulVanish {}
		, ApplyRangeModifiers { false }
		, TargetSnapDistance { 0.5 }
		, UseDisperseCoord { false }
		, RecordSourceCoord { false }

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
		, EdgeAttenuation { 1.0 }
		, CountAttenuation { 1.0 }

		, DisperseWeapons {}
		, DisperseBursts {}
		, DisperseCounts { 0 }
		, DisperseDelays { 1 }
		, DisperseCycle { -1 }
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
		, DisperseFaceCheck { true }
		, DisperseCoord { { 0, 0, 0 } }
	{ }

	virtual ~PhobosTrajectoryType() noexcept = default;
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	virtual bool Save(PhobosStreamWriter& Stm) const;
	virtual TrajectoryFlag Flag() const { return TrajectoryFlag::Invalid; };
	virtual void Read(CCINIClass* const pINI, const char* pSection);
	[[nodiscard]] virtual std::unique_ptr<PhobosTrajectory> CreateInstance() const = 0;

	static std::vector<CellClass*> GetCellsInProximityRadius(const BulletClass* const pBullet, const Leptons trajectoryProximityRange);
	static std::vector<CellStruct> GetCellsInRectangle(const CellStruct bottomStaCell, const CellStruct leftMidCell, const CellStruct rightMidCell, const CellStruct topEndCell);

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	Valueable<double> Speed;
	Valueable<int> Duration;
	Valueable<int> TolerantTime;
	Valueable<int> BulletROT;
	Valueable<bool> BulletSpin;
	Valueable<bool> BulletStable;
	Valueable<bool> BulletOnPlane;
	Valueable<double> RetargetRadius;
	Valueable<bool> Synchronize;
	Nullable<bool> PeacefulVanish;
	Valueable<bool> ApplyRangeModifiers;
	Valueable<double> TargetSnapDistance;
	Valueable<bool> UseDisperseCoord;
	Valueable<bool> RecordSourceCoord;

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
	Valueable<double> EdgeAttenuation;
	Valueable<double> CountAttenuation;

	ValueableVector<WeaponTypeClass*> DisperseWeapons;
	ValueableVector<int> DisperseBursts;
	Valueable<int> DisperseCounts;
	Valueable<int> DisperseDelays;
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
	Valueable<CoordStruct> DisperseCoord;
};

class PhobosTrajectory
{
public:
	PhobosTrajectory(PhobosTrajectoryType const* trajType) :
		Duration {}
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
	{ }

	virtual ~PhobosTrajectory() noexcept = default;
	virtual bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	virtual bool Save(PhobosStreamWriter& Stm) const;
	virtual TrajectoryFlag Flag() const { return TrajectoryFlag::Invalid; };
	virtual void OnUnlimbo(BulletClass* pBullet, CoordStruct* pCoord, BulletVelocity* pVelocity) = 0;
	virtual bool OnAI(BulletClass* pBullet) = 0;
	virtual void OnAIPreDetonate(BulletClass* pBullet) = 0;
	virtual void OnAIVelocity(BulletClass* pBullet, BulletVelocity* pSpeed, BulletVelocity* pPosition) = 0;
	virtual TrajectoryCheckReturnType OnAITargetCoordCheck(BulletClass* pBullet) = 0;
	virtual TrajectoryCheckReturnType OnAITechnoCheck(BulletClass* pBullet, TechnoClass* pTechno) = 0;

private:
	template <typename T>
	void Serialize(T& Stm);

public:
	CDTimerClass Duration;
	CDTimerClass TolerantTimer;
	double FirepowerMult;
	int AttenuationRange;
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
