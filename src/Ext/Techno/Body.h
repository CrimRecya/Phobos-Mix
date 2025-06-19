#pragma once
#include <InfantryClass.h>
#include <AnimClass.h>

#include <Helpers/Macro.h>
#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/Macro.h>
#include <Ext/Bullet/Trajectories/PhobosTrajectory.h>
#include <New/Entity/ShieldClass.h>
#include <New/Entity/LaserTrailClass.h>
#include <New/Entity/AttachEffectClass.h>
#include <New/Entity/AttachmentClass.h>
#include <New/Entity/SquadManagerClass.h>

class BulletClass;

class TechnoExt
{
public:
	using base_type = TechnoClass;

	static constexpr DWORD Canary = 0x55555555;
	static constexpr size_t ExtPointerOffset = 0x34C;

	class ExtData final : public Extension<TechnoClass>
	{
	public:
		TechnoTypeExt::ExtData* TypeExtData;
		std::unique_ptr<ShieldClass> Shield;
		std::vector<std::unique_ptr<LaserTrailClass>> LaserTrails;
		std::vector<std::unique_ptr<AttachEffectClass>> AttachedEffects;
		AttachEffectTechnoProperties AE;
		TechnoTypeClass* PreviousType; // Type change registered in TechnoClass::AI on current frame and used in FootClass::AI on same frame and reset after.
		std::vector<EBolt*> ElectricBolts; // EBolts are not serialized so do not serialize this either.
		int AnimRefCount; // Used to keep track of how many times this techno is referenced in anims f.ex Invoker, ParentBuilding etc., for pointer invalidation.
		bool SubterraneanHarvFreshFromFactory;
		AbstractClass* SubterraneanHarvRallyDest;
		bool ReceiveDamage;
		bool LastKillWasTeamTarget;
		CDTimerClass PassengerDeletionTimer;
		ShieldTypeClass* CurrentShieldType;
		int LastWarpDistance;
		CDTimerClass ChargeTurretTimer; // Used for charge turrets instead of RearmTimer if weapon has ChargeTurret.Delays set.
		CDTimerClass AutoDeathTimer;
		AnimTypeClass* MindControlRingAnimType;
		int DamageNumberOffset;
		int Strafe_BombsDroppedThisRound;
		int CurrentAircraftWeaponIndex;
		bool IsInTunnel;
		bool IsBurrowed;
		bool HasBeenPlacedOnMap; // Set to true on first Unlimbo() call.
		CDTimerClass DeployFireTimer;
		bool SkipTargetChangeResetSequence;
		bool ForceFullRearmDelay;
		bool LastRearmWasFullDelay;
		bool CanCloakDuringRearm; // Current rearm timer was started by DecloakToFire=no weapon.
		int WHAnimRemainingCreationInterval;
		bool UnitIdleIsSelected;
		CDTimerClass UnitIdleActionTimer;
		CDTimerClass UnitIdleActionGapTimer;
		CDTimerClass UnitAutoDeployTimer;
		WeaponTypeClass* LastWeaponType;
		CoordStruct LastWeaponFLH;
		std::shared_ptr<PhobosMap<DWORD, PhobosTrajectory::GroupData>> TrajectoryGroup;
		std::vector<RecoilData> ExtraTurretRecoil;
		std::vector<RecoilData> ExtraBarrelRecoil;
		int ScatteringStopFrame;
		int MyTargetingFrame;
		int AttackMoveFollowerTempCount;
		CellClass* AutoTargetedWallCell;
		bool HasCachedClickMission;
		Mission CachedMission;
		AbstractClass* CachedCell;
		AbstractClass* CachedTarget;
		bool HasCachedClickEvent;
		EventType CachedEventType;
		CellClass* FiringObstacleCell; // Set on firing if there is an obstacle cell between target and techno, used for updating WaveClass target etc.
		bool IsDetachingForCloak; // Used for checking animation detaching, set to true before calling Detach_All() on techno when this anim is attached to and to false after when cloaking only.
		int BeControlledThreatFrame;
		int LastHurtFrame;
		DWORD LastTargetID;
		int AccumulatedGattlingValue;
		bool ShouldUpdateGattlingValue;
		int AttachedEffectInvokerCount;

		// Used for Passengers.SyncOwner.RevertOnExit instead of TechnoClass::InitialOwner / OriginallyOwnedByHouse,
		// as neither is guaranteed to point to the house the TechnoClass had prior to entering transport and cannot be safely overridden.
		HouseClass* OriginalPassengerOwner;
		bool HasRemainingWarpInDelay;          // Converted from object with Teleport Locomotor to one with a different Locomotor while still phasing in OR set if ChronoSphereDelay > 0.
		int LastWarpInDelay;                   // Last-warp in delay for this unit, used by HasCarryoverWarpInDelay.
		bool IsBeingChronoSphered;             // Set to true on units currently being ChronoSphered, does not apply to Ares-ChronoSphere'd buildings or Chrono reinforcements.
		bool KeepTargetOnMove;
		CellStruct LastSensorsMapCoords;
		CDTimerClass TiberiumEater_Timer;

		bool AggressiveStance;                  // Aggressive stance that will auto target buildings
		bool CeaseFireStance;

		bool IsWreckage;

		BuildingClass* BuildingOccupying;

		AirstrikeClass* AirstrikeTargetingMe;

		struct LaserTrackingData
		{
			LaserDrawClass* Laser;
			int WeaponIdx;
			int BurstIdx;
			WeaponTypeClass* CreatorWeapon;
		};

		std::vector<LaserTrackingData> MyTrackingLasers;
		AbstractClass* MyTrackingLasersTarget;
		std::vector<LaserDrawClass*> TrackingLasersTargetingMe;

		SquadManagerClass* SquadManager;

		AttachmentClass* ParentAttachment;
		std::vector<std::unique_ptr<AttachmentClass>> ChildAttachments;
		CellClass* ThisOccupationCell;
		CellClass* LastOccupationCell;
		// Ares
		std::optional<bool> AltOccupation; // if the unit marks cell occupation flags, this is set to whether it uses the "high" occupation members

		CDTimerClass FiringAnimationTimer;

		// cache tint values
		int TintColorOwner;
		int TintColorAllies;
		int TintColorEnemies;
		int TintIntensityOwner;
		int TintIntensityAllies;
		int TintIntensityEnemies;

		ExtData(TechnoClass* OwnerObject) : Extension<TechnoClass>(OwnerObject)
			, TypeExtData { nullptr }
			, Shield {}
			, LaserTrails {}
			, AttachedEffects {}
			, AE {}
			, PreviousType { nullptr }
			, ElectricBolts {}
			, AnimRefCount { 0 }
			, SubterraneanHarvFreshFromFactory { false }
			, SubterraneanHarvRallyDest { nullptr }
			, ReceiveDamage { false }
			, LastKillWasTeamTarget { false }
			, PassengerDeletionTimer {}
			, CurrentShieldType { nullptr }
			, LastWarpDistance {}
			, ChargeTurretTimer {}
			, AutoDeathTimer {}
			, MindControlRingAnimType { nullptr }
			, DamageNumberOffset { INT32_MIN }
			, Strafe_BombsDroppedThisRound { 0 }
			, CurrentAircraftWeaponIndex {}
			, IsInTunnel { false }
			, IsBurrowed { false }
			, HasBeenPlacedOnMap { false }
			, DeployFireTimer {}
			, SkipTargetChangeResetSequence { false }
			, ForceFullRearmDelay { false }
			, LastRearmWasFullDelay { false }
			, CanCloakDuringRearm { false }
			, WHAnimRemainingCreationInterval { 0 }
			, UnitIdleIsSelected { false }
			, UnitIdleActionTimer {}
			, UnitIdleActionGapTimer {}
			, UnitAutoDeployTimer {}
			, LastWeaponType {}
			, LastWeaponFLH {}
			, TrajectoryGroup {}
			, ExtraTurretRecoil {}
			, ExtraBarrelRecoil {}
			, ScatteringStopFrame { 0 }
			, MyTargetingFrame { ScenarioClass::Instance->Random.RandomRanged(0,15) }
			, AttackMoveFollowerTempCount { 0 }
			, AutoTargetedWallCell{ nullptr }
			, HasCachedClickMission { false }
			, CachedMission { Mission::None }
			, CachedCell { nullptr }
			, CachedTarget { nullptr }
			, HasCachedClickEvent { false }
			, CachedEventType { EventType::LAST_EVENT }
			, FiringObstacleCell {}
			, IsDetachingForCloak { false }
			, BeControlledThreatFrame { 0 }
			, LastHurtFrame { 0 }
			, LastTargetID { 0xFFFFFFFF }
			, AccumulatedGattlingValue { 0 }
			, ShouldUpdateGattlingValue { false }
			, OriginalPassengerOwner {}
			, HasRemainingWarpInDelay { false }
			, LastWarpInDelay { 0 }
			, IsBeingChronoSphered { false }
			, AggressiveStance { false }
			, CeaseFireStance { false }
			, KeepTargetOnMove { false }
			, LastSensorsMapCoords { CellStruct::Empty }
			, IsWreckage { false }
			, BuildingOccupying { }
			, TiberiumEater_Timer {}
			, AirstrikeTargetingMe { nullptr }
			, MyTrackingLasers { }
			, MyTrackingLasersTarget { }
			, TrackingLasersTargetingMe { }
			, SquadManager { nullptr }
			, ParentAttachment { nullptr }
			, ChildAttachments {}
			, ThisOccupationCell { nullptr }
			, LastOccupationCell { nullptr }
			, AltOccupation {}
			, FiringAnimationTimer {}
			, AttachedEffectInvokerCount { 0 }
			, TintColorOwner { 0 }
			, TintColorAllies { 0 }
			, TintColorEnemies { 0 }
			, TintIntensityOwner { 0 }
			, TintIntensityAllies { 0 }
			, TintIntensityEnemies { 0 }
		{ }

		void OnEarlyUpdate();

		void ApplyInterceptor();
		bool CheckDeathConditions(bool isInLimbo = false);
		void DepletedAmmoActions();
		void EatPassengers();
		void UpdateTiberiumEater();
		void UpdateShield();
		void UpdateOnTunnelEnter();
		void UpdateOnTunnelExit();
		void ApplySpawnLimitRange();
		void UpdateTypeData(TechnoTypeClass* pCurrentType);
		void UpdateTypeData_Foot();
		void UpdateLaserTrails();
		void UpdateAttachEffects();
		void UpdateGattlingRateDownReset();
		void UpdateKeepTargetOnMove();
		void UpdateWarpInDelay();
		void UpdateCumulativeAttachEffects(AttachEffectTypeClass* pAttachEffectType, AttachEffectClass* pRemoved = nullptr);
		void RecalculateStatMultipliers();
		void UpdateTemporal();
		void UpdateMindControlAnim();
		void UpdateRecountBurst();
		void UpdateRearmInEMPState();
		void UpdateRearmInTemporal();
		void UpdateTrackingLasers();
		void InitializeLaserTrails();
		void InitializeAttachEffects();
		void InitializeAttachments();
		void UpdateSelfOwnedAttachEffects();
		void RecordRecoilData();
		void UpdateRecoilData();
		void InitializeRecoilData();
		bool HasAttachedEffects(std::vector<AttachEffectTypeClass*> attachEffectTypes, bool requireAll, bool ignoreSameSource, TechnoClass* pInvoker, AbstractClass* pSource, std::vector<int> const* minCounts, std::vector<int> const* maxCounts) const;
		int GetAttachedEffectCumulativeCount(AttachEffectTypeClass* pAttachEffectType, bool ignoreSameSource = false, TechnoClass* pInvoker = nullptr, AbstractClass* pSource = nullptr) const;
		void InitializeDisplayInfo();
		void StopIdleAction();
		void ApplyIdleAction();
		void ManualIdleAction();
		void CheckIdleAction();
		void UpdateIdleDir();
		void SetTurretDir(DirStruct desiredDir, bool limited = false);
		void StopRotateWithNewROT(int ROT = -1);
		void UpdateCachedClick();
		void ApplyMindControlRangeLimit();
		int ApplyForceWeaponInRange(AbstractClass* pTarget);
		void UpdateTintValues();

		UnitTypeClass* GetUnitTypeExtra() const;

		virtual ~ExtData() override;
		virtual void InvalidatePointer(void* ptr, bool bRemoved) override;
		virtual void LoadFromStream(PhobosStreamReader& Stm) override;
		virtual void SaveToStream(PhobosStreamWriter& Stm) override;

		void InitAggressiveStance();
		bool GetAggressiveStance() const;
		void ToggleAggressiveStance();
		bool CanToggleAggressiveStance();

		void InitCeaseFireStance();
		bool GetCeaseFireStance() const;
		void ToggleCeaseFireStance();
		bool CanToggleCeaseFireStance();

	private:
		template <typename T>
		void Serialize(T& Stm);
	};

	class ExtContainer final : public Container<TechnoExt>
	{
	public:
		ExtContainer();
		~ExtContainer();

		virtual bool InvalidateExtDataIgnorable(void* const ptr) const override
		{
			auto const abs = static_cast<AbstractClass*>(ptr)->WhatAmI();

			switch (abs)
			{
			case AbstractType::Airstrike:
			case AbstractType::Aircraft:
			case AbstractType::Building:
			case AbstractType::Infantry:
			case AbstractType::Unit:
			case AbstractType::Terrain:
			case AbstractType::Bullet:
				return false;
			default:
				return true;
			}
		}
	};

	struct DrawFrameStruct
	{
		int TopLength;
		int TopFrame;
		SHPStruct* TopPipSHP;
		int MidLength;
		int MidFrame;
		SHPStruct* MidPipSHP;
		int MaxLength;
		int BrdFrame;
		Point2D* Location;
		RectangleStruct* Bounds;
	};

	static ExtContainer ExtMap;

	static UnitClass* Deployer;

	static bool LoadGlobals(PhobosStreamReader& Stm);
	static bool SaveGlobals(PhobosStreamWriter& Stm);

	static bool IsActive(TechnoClass* pThis);
	static bool IsActiveIgnoreEMP(TechnoClass* pThis);

	static bool IsHarvesting(TechnoClass* pThis);
	static bool HasAvailableDock(TechnoClass* pThis);
	static bool HasRadioLinkWithDock(TechnoClass* pThis);

	static Matrix3D GetTransform(TechnoClass* pThis, VoxelIndexKey* pKey = nullptr, bool isShadow = false);
	static Matrix3D GetFLHMatrix(TechnoClass* pThis, const CoordStruct& flh, bool isOnTurret, double factor = 1.0, bool isShadow = false, int turIdx = -1);
	static Matrix3D TransformFLHForTurret(TechnoClass* pThis, Matrix3D mtx, bool isOnTurret, double factor = 1.0, int turIdx = -1);
	static CoordStruct GetFLHAbsoluteCoords(TechnoClass* pThis, const CoordStruct& flh, bool isOnTurret = false, int turIdx = -1);

	static CoordStruct GetBurstFLH(TechnoClass* pThis, int weaponIndex, bool& FLHFound);
	static CoordStruct GetSimpleFLH(InfantryClass* pThis, int weaponIndex, bool& FLHFound);

	template <bool checkParent = false>
	static void EnhancedScatterContent(CellClass* pCell, TechnoClass* pThis, const CoordStruct& coords, bool alt);
	static TechnoClass* FindOccupyTechno(CellClass* pCell, TechnoClass* pExclude);
	static void __fastcall CallEnhancedScatterContent(CellClass* pCell, TechnoClass* pThis, const CoordStruct& coords, bool alt);
	static void __fastcall CallEnhancedScatterContent(CellClass* pCell, FootClass* pFoot, bool alt);
	static void ScatterPathCellContent(FootClass* pThis, CellClass* pCell);
	static CellStruct GetScatterCell(FootClass* pThis, int face);
	static int GetTechnoCloseEnoughRange(TechnoClass* pThis);

	static bool AttachTo(TechnoClass* pThis, TechnoClass* pParent);
	static bool DetachFromParent(TechnoClass* pThis);

	static void DestroyAttachments(TechnoClass* pThis, TechnoClass* pSource);
	static void HandleDestructionAsChild(TechnoClass* pThis);
	static void UnlimboAttachments(TechnoClass* pThis);
	static void LimboAttachments(TechnoClass* pThis);
	static void TransferAttachments(TechnoClass* pThis, TechnoClass* pThat);

	static bool IsAttached(TechnoClass* pThis);
	static bool HasAttachmentLoco(FootClass* pThis); // FIXME shouldn't be here
	static bool DoesntOccupyCellAsChild(TechnoClass* pThis);
	static bool IsChildOf(TechnoClass* pThis, TechnoClass* pParent, bool deep = true);
	static bool AreRelatives(TechnoClass* pThis, TechnoClass* pThat);
	static TechnoClass* GetTopLevelParent(TechnoClass* pThis);

	static void ChangeOwnerMissionFix(FootClass* pThis);
	static void KillSelf(TechnoClass* pThis, AutoDeathBehavior deathOption, AnimTypeClass* pVanishAnimation, bool isInLimbo = false);
	static void Kill(TechnoClass* pThis, ObjectClass* pAttacker, HouseClass* pAttackingHouse);
	static void Kill(TechnoClass* pThis, TechnoClass* pAttacker);
	static void ObjectKilledBy(TechnoClass* pThis, TechnoClass* pKiller);
	static void UpdateSharedAmmo(TechnoClass* pThis);
	static double GetCurrentSpeedMultiplier(FootClass* pThis);
	static void DrawSelfHealPips(TechnoClass* pThis, Point2D* pLocation, RectangleStruct* pBounds);
	static void DrawInsignia(TechnoClass* pThis, Point2D* pLocation, RectangleStruct* pBounds);
	static void ApplyGainedSelfHeal(TechnoClass* pThis);
	static void SyncInvulnerability(TechnoClass* pFrom, TechnoClass* pTo);
	static CoordStruct PassengerKickOutLocation(TechnoClass* pThis, FootClass* pPassenger, int maxAttempts);
	static bool AllowedTargetByZone(TechnoClass* pThis, TechnoClass* pTarget, TargetZoneScanType zoneScanType, WeaponTypeClass* pWeapon = nullptr, bool useZone = false, int zone = -1);
	static void UpdateAttachedAnimLayers(TechnoClass* pThis);
	static bool ConvertToType(TechnoClass* pThis, TechnoTypeClass* toType);
	static bool CanDeployIntoBuilding(UnitClass* pThis, bool noDeploysIntoDefaultValue = false);
	static bool IsTypeImmune(TechnoClass* pThis, TechnoClass* pSource);
	static int GetTintColor(TechnoClass* pThis, bool invulnerability, bool airstrike, bool berserk);
	static int GetCustomTintColor(TechnoClass* pThis);
	static int GetCustomTintIntensity(TechnoClass* pThis);
	static void ApplyCustomTintValues(TechnoClass* pThis, int& color, int& intensity);
	static void DrawFactoryProgress(BuildingClass* pThis, RectangleStruct* pBounds, Point2D basePosition);
	static void DrawSuperProgress(BuildingClass* pThis, RectangleStruct* pBounds, Point2D basePosition);
	static void DrawIronCurtainProgress(TechnoClass* pThis, RectangleStruct* pBounds, Point2D basePosition, bool isBuilding, bool isInfantry);
	static void DrawTemporalProgress(TechnoClass* pThis, RectangleStruct* pBounds, Point2D basePosition, bool isBuilding, bool isInfantry);
	static void DrawVanillaStyleFootBar(DrawFrameStruct* pDraw);
	static void DrawVanillaStyleBuildingBar(DrawFrameStruct* pDraw);
	static Point2D GetScreenLocation(TechnoClass* pThis);
	static Point2D GetFootSelectBracketPosition(TechnoClass* pThis, Anchor anchor);
	static Point2D GetBuildingSelectBracketPosition(TechnoClass* pThis, BuildingSelectBracketPosition bracketPosition);
	static void DrawSelectBox(TechnoClass* pThis, const Point2D* pLocation, const RectangleStruct* pBounds, bool drawBefore = false);
	static void ProcessDigitalDisplays(TechnoClass* pThis);
	static void GetValuesForDisplay(TechnoClass* pThis, DisplayInfoType infoType, int& value, int& maxValue, int infoIndex);
	static void GetDigitalDisplayFakeHealth(TechnoClass* pThis, int& value, int& maxValue);
	static void DrawExtraImage(TechnoClass* pThis, CellClass* pCell, DirStruct dir = DirStruct(0), int height = 0);
	static void DrawExtraImage(TechnoClass* pThis, const Point2D& location, const RectangleStruct& bounds, DirStruct dir = DirStruct(0), bool transparent = false, Sequence action = Sequence::Nothing, int tilt = -1);
	static void DrawExtraImage(UnitClass* pThis, Point2D* pLocation, RectangleStruct* pBounds, DirStruct dir = DirStruct(0), bool transparent = false, int tilt = -1);
	static void DrawExtraImage(InfantryClass* pThis, Point2D* pLocation, RectangleStruct* pBounds, DirStruct dir = DirStruct(0), bool transparent = false, Sequence action = Sequence::Nothing);

	// WeaponHelpers.cpp
	static int PickWeaponIndex(TechnoClass* pThis, TechnoClass* pTargetTechno, AbstractClass* pTarget, int weaponIndexOne, int weaponIndexTwo, bool allowFallback = true, bool allowAAFallback = true);
	static void FireWeaponAtSelf(TechnoClass* pThis, WeaponTypeClass* pWeaponType);
	static bool CanFireNoAmmoWeapon(TechnoClass* pThis, int weaponIndex);
	static WeaponTypeClass* GetDeployFireWeapon(TechnoClass* pThis, int& weaponIndex);
	static WeaponTypeClass* GetDeployFireWeapon(TechnoClass* pThis);
	static WeaponTypeClass* GetCurrentWeapon(TechnoClass* pThis, int& weaponIndex, bool getSecondary = false);
	static WeaponTypeClass* GetCurrentWeapon(TechnoClass* pThis, bool getSecondary = false);
	static int GetWeaponIndexAgainstWall(TechnoClass* pThis, OverlayTypeClass* pWallOverlayType);
	static void ApplyKillWeapon(TechnoClass* pThis, TechnoClass* pSource, WarheadTypeClass* pWH);
	static void ApplyRevengeWeapon(TechnoClass* pThis, TechnoClass* pSource, WarheadTypeClass* pWH);
	static bool MultiWeaponCanFire(TechnoClass* pThis, AbstractClass* pTarget, CellClass* pTargetCell, WeaponTypeClass* pWeapon);
};
