#pragma once
#include <vector>
#include <string>
#include <type_traits>
#include <functional>
#include "IH.Initial.h"
#include "ExtJson.h"
#include <YRPP.h>


#ifndef PARRAY_DEFINITION
#define PARRAY_DEFINITION
template<typename T>
struct PArray
{
	size_t N;
	const T* Data;

	PArray() :N(0), Data(nullptr) {}
	PArray(const std::vector<T>& p) :N(p.size()), Data(p.data()) {}
	template<size_t _N>
	PArray(const T p[_N]) : N(_N), Data(p) {}
};
#endif

using LibFuncHandle = void*;
using CSFHandle = void*;
struct LibInputFnTable;
struct CSFClass_ITable;

template<typename T>
T* AsType(LibFuncHandle Handle)
{
	return (T*)Handle;
}

template<typename T>
T AsTypePtr(LibFuncHandle Handle)
{
	return (T)Handle;
}

struct GeneratorParam
{
	int ExecutorVersion;
	void* Ptr;
	AbstractType Type;
	int Reserved;
};

struct RoutineParam
{
	char Unknown[16];
};
const RoutineParam NullRoutineParam{};

enum class FuncType
{
	Default = 0,//idk type
	Condition = 1,//bool (__cdecl *)(JsonObject Context)
	Action = 2,//void (__cdecl *)(JsonObject Context)
	Callback = 3,//void (__cdecl *)(JsonObject Context)
	Procedure = 4,//void (__cdecl *)(void)
	Comm = 5,//void* (__cdecl *)(void*)
	ConditionAlt = 6,//bool (__cdecl *)(GeneratorParam Param)
	ActionAlt = 7,//void (__cdecl *)(GeneratorParam Param)
	CommAlt = 8//RoutineParam (__cdecl*)(RoutineParam)
};
using FuncType_Condition = bool(__cdecl*)(JsonObject Context);
using FuncType_Action = void(__cdecl*)(JsonObject Context);
using FuncType_Callback = void(__cdecl*)(JsonObject Context);
using FuncType_Procedure = void(__cdecl*)(void);
using FuncType_Comm = void*(__cdecl*)(void*);
using FuncType_ConditionAlt = bool(__cdecl*)(GeneratorParam Param);
using FuncType_ActionAlt = void(__cdecl*)(GeneratorParam Param);
using FuncType_CommAlt = RoutineParam(__cdecl*)(RoutineParam);



//For Executor&Routine
enum class CreateMode
{
	ProcReplace = 0,  //替换Proc 替换Context/Param 会触发Destructor、Uninit等
	ProcMerge = 1,    //替换Proc 合并Context 更新重复项 对Routine同Replace 不会触发Destructor、Uninit等
	ProcReserve = 2,  //替换Proc 保留Context/Param 不会触发Destructor、Uninit等
	CtxReplace = 4,   //保留Proc 替换Context/Param 会触发Destructor、Uninit等
	CtxMerge = 5,     //保留Proc 合并Context 更新重复项 对Routine同Replace 不会触发Destructor、Uninit等
	CtxReserve = 6,   //保留Proc 保留Context/Param 不会触发Destructor、Uninit等
};

struct FuncInfo
{
	static const int GClassVersion{ 1 };
	int ClassVersion{ GClassVersion };

	LibFuncHandle Func;
	FuncType Type;
	FuncInfo(LibFuncHandle F, FuncType T) :ClassVersion(GClassVersion), Func(F), Type(T) {}
	FuncInfo() = default;
	template<typename T>
	T* SeeAsType()
	{
		return (T*)Func;
	}
};


struct GeneralExecutor;
using SwizzleExecutor_t = void(__cdecl*)(GeneralExecutor* Exec, const GeneratorParam* Param, JsonObject CurrentContext);

struct ExecutorBase
{
	static const int GClassVersion{ 1 };
	int ClassVersion{ GClassVersion };
	
	SwizzleExecutor_t Swizzle;
	JsonObject Context;
	const char* Type;
	const char* ExecType;
	FuncInfo* Action;
	FuncInfo* Destructor;
	int Delay;
};
static_assert(sizeof(ExecutorBase) == 32);

struct GeneralExecutor
{
	enum class type
	{
		Trigger=0,
		Once=1,
		InfiniteLoop=2,
		WhileLoop=3,
		NLoop=4,
		InfiniteTrigger=5,
	}Type;
	
	ExecutorBase Base;
	FuncInfo* Condition;
	int Interval;

	int Reserved[5];
};
static_assert(sizeof(GeneralExecutor) == 64);

struct ContextIndex
{
	const char* Type;
	const char* ExecType;
	GeneratorParam Param;
};

using UTF8_CString = std::decay_t<decltype(u8"")>;
using UTF8_CharType = std::remove_cv_t<std::remove_pointer_t<UTF8_CString>>;

struct LibVersionInfo
{
	static const int GClassVersion{ 1 };
	int ClassVersion{ GClassVersion };

	const char* LibName;
	int Version;
	int LowestSupportedVersion;
	UTF8_CString Description;

	int Reserved[3];
};
static_assert(sizeof(LibVersionInfo) == 32);

enum class GenCallRetType
{
	False = 0,
	True = 1,
	Void = 2,
	Default = -1
};

using GetFunc_t = FuncInfo*(__cdecl*)(const char*, int);
using ContextFunc_t = bool(__cdecl*)(JsonObject Context);
using BindingGenerator_t = bool(__cdecl*)(JsonObject Binding,const GeneratorParam& Param);
using Binder_t = bool(__cdecl*)(JsonObject Binding, JsonObject Context);
using DirectBinder_t = bool(__cdecl*)(JsonObject Context, const GeneratorParam& Param);
using Routine_t = int(__cdecl*)(RoutineParam*);
using RoutineHandle = void*;

const int ExitThisRoutine = INT_MAX;
const int PauseThisRoutine = INT_MAX - 1;
const int DoNotCheckVersion = INT_MAX;

struct InitDependency
{
	const char* LibName;
	int Version;
	int LowestRequiredVersion;
	bool RequiredLoadSequence;
	int Reserved[4];
};
static_assert(sizeof(InitDependency) == 32);

struct InitResult
{
	static const int GClassVersion{ 1 };
	int ClassVersion{ GClassVersion };

	LibVersionInfo* Info;
	GetFunc_t GetFunc;
	PArray<InitDependency> Dependencies;
	FuncType_Procedure OrderedInit;

	int Reserved[2];
};
static_assert(sizeof(InitResult) == 32);

struct RoutineSet
{
	Routine_t Init;
	Routine_t Loop;
	Routine_t Uninit;
	Routine_t Exit;
};

struct CSFEntry
{
	const wchar_t* Value;
	const char* Extra;//无extra传空串
};


struct InitInput
{
	static const int GClassVersion{ 1 };
	const int ClassVersion{ GClassVersion };
	LibInputFnTable* FunctionTable;

	int* ECInitializeStage;
	int Reserved[5];
};
static_assert(sizeof(InitInput) == 32);



using InitFunc_t = InitResult * (__cdecl*)(InitInput*);

namespace Init
{
	bool Initialize();
	bool __cdecl RegisterEntry(LibFuncHandle Entry);
	PArray<IHInitialLoadService> __cdecl QueryServiceRequest(const char* Name);
	
	extern InitInput* LibInput;
	extern InitResult Result;
}

struct BasicLibData
{
	bool Available;
	InitInput* In;
	InitResult* Out;
	void* ReservedA;
	void* ReservedB;
	int Reserved[3];
};

#define GENERATOR_PARAM(name) \
inline GeneratorParam GetGeneratorParam(name ## Class* pClass)\
	{ return GeneratorParam{ FuncInfo::GClassVersion,pClass,AbstractType:: name ,0 }; }

class CampaignClass;//Actually there is
class IsotileClass;
class IsotileTypeClass;
class SpecialClass;
class ActionClass;
class EventClass;//Actually there is
class TacticalMapClass;
class AITriggerClass;
class VeinholeMonsterClass;//Actually there is
class NavyTypeClass;

inline GeneratorParam GetGeneratorParam(void* pClass)
{ 
	return GeneratorParam{ FuncInfo::GClassVersion,pClass,AbstractType::None ,0 }; 
}
GENERATOR_PARAM(Unit)
GENERATOR_PARAM(Aircraft)
GENERATOR_PARAM(AircraftType)
GENERATOR_PARAM(Anim)
GENERATOR_PARAM(AnimType)
GENERATOR_PARAM(Building)
GENERATOR_PARAM(BuildingType)
GENERATOR_PARAM(Bullet)
GENERATOR_PARAM(BulletType)
GENERATOR_PARAM(Campaign)//
GENERATOR_PARAM(Cell)
GENERATOR_PARAM(Factory)
GENERATOR_PARAM(House)
GENERATOR_PARAM(HouseType)
GENERATOR_PARAM(Infantry)
GENERATOR_PARAM(InfantryType)
GENERATOR_PARAM(Isotile)//
GENERATOR_PARAM(IsotileType)//
GENERATOR_PARAM(BuildingLight)
GENERATOR_PARAM(Overlay)
GENERATOR_PARAM(OverlayType)
GENERATOR_PARAM(Particle)
GENERATOR_PARAM(ParticleType)
GENERATOR_PARAM(ParticleSystem)
GENERATOR_PARAM(ParticleSystemType)
GENERATOR_PARAM(Script)
GENERATOR_PARAM(ScriptType)
GENERATOR_PARAM(Side)
GENERATOR_PARAM(Smudge)
GENERATOR_PARAM(SmudgeType)
GENERATOR_PARAM(Special)//
GENERATOR_PARAM(SuperWeaponType)
GENERATOR_PARAM(TaskForce)
GENERATOR_PARAM(Team)
GENERATOR_PARAM(TeamType)
GENERATOR_PARAM(Terrain)
GENERATOR_PARAM(TerrainType)
GENERATOR_PARAM(Trigger)
GENERATOR_PARAM(TriggerType)
GENERATOR_PARAM(UnitType)
GENERATOR_PARAM(VoxelAnim)
GENERATOR_PARAM(VoxelAnimType)
GENERATOR_PARAM(Wave)
GENERATOR_PARAM(Tag)
GENERATOR_PARAM(TagType)
GENERATOR_PARAM(Tiberium)
GENERATOR_PARAM(Action)//
GENERATOR_PARAM(Event)//
GENERATOR_PARAM(WeaponType)
GENERATOR_PARAM(WarheadType)
GENERATOR_PARAM(Waypoint)
GENERATOR_PARAM(Abstract)
GENERATOR_PARAM(Tube)
GENERATOR_PARAM(LightSource)
GENERATOR_PARAM(EMPulse)
GENERATOR_PARAM(TacticalMap)//
GENERATOR_PARAM(Super)
GENERATOR_PARAM(AITrigger)//
GENERATOR_PARAM(AITriggerType)
GENERATOR_PARAM(Neuron)
GENERATOR_PARAM(FoggedObject)
GENERATOR_PARAM(AlphaShape)
GENERATOR_PARAM(VeinholeMonster)//
GENERATOR_PARAM(NavyType)//
GENERATOR_PARAM(SpawnManager)
GENERATOR_PARAM(CaptureManager)
GENERATOR_PARAM(Parasite)
GENERATOR_PARAM(Bomb)
GENERATOR_PARAM(RadSite)
GENERATOR_PARAM(Temporal)
GENERATOR_PARAM(Airstrike)
GENERATOR_PARAM(SlaveManager)
GENERATOR_PARAM(DiskLaser)
