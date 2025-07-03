#pragma once

#pragma region Toggle

//#define ENABLE_ASTAR_REIMPL

#pragma endregion

#ifdef ENABLE_ASTAR_REIMPL

#include <Utilities/Container.h>
#include <Utilities/TemplateDef.h>
#include <Utilities/Macro.h>

#include <PriorityQueueClass.h>

#define ENABLE_ASTAR_OPTIMIZE true

class AStarClass
{
public:
	struct PathType
	{
		CellStruct Start;
		int AccumulatedDistance;
		int AccumulatedCount;
		int* Facings;
		int unknown_int_10;
		int* Levels;
		CellStruct unknown_cellstruct_18;
		int unknown_int_1C;
	};

	struct WorkPathHeapNode
	{
		CellClass** Items;
		int Level;
		WorkPathHeapNode* Prev;
	};

	struct WorkPathHeapData
	{
		WorkPathHeapNode WorkPathHeapData[131072];
		int Count;
	};

	struct PassabilityIndexData
	{
		short Indexes[500];
	};

	struct WorkPathQueueNode
	{
		WorkPathHeapNode* WorkPathHeapNodes;
		float MovementDistance;
		float AccumulatedDistance;
		int AccumulatedCount;
	};

	struct HierarchicalQueueNode
	{
		int NodeIndex;
		int FinderIndex;
		float Coefficient;
		int Count;
	};

	DEFINE_REFERENCE(AStarClass, Instance, 0x87E8B8u)
	DEFINE_REFERENCE(AStarClass::PathType, PathData, 0x89A2D8u)

	AStarClass() JMP_THIS(0x42A6D0);
	~AStarClass() JMP_THIS(0x42A900);

	void CleanUp() JMP_THIS(0x42A5B0);
	void ClearPassability() JMP_THIS(0x42C1C0);
	void ReinitCostArrays(RectangleStruct* a2) JMP_THIS(0x42AC00);

	void RecordCellIndex(FootClass* pFoot) JMP_THIS(0x42CCD0);
	void RegisterCellIndex(int a2, int index) JMP_THIS(0x42CF80);

	static CellStruct* __fastcall NextPathCell(CellStruct* a1, CellStruct* a2, int a3) JMP_STD(0x42D490);


	// main external interface
	PathType* DoPathfinding
	(
		CellStruct* pSource,
		CellStruct* pDestination,
		FootClass* pFoot,
		int* a5,
		int a6,
		MovementZone movementZone,
		int mode
	)
	JMP_THIS(0x42C900);

	// lightweight external interface
	int AttemptPath
	(
		CellStruct* pSource,
		CellStruct* pDestination,
		FootClass* pFoot,
		bool checkSourceBridge,
		bool checkDestinationBridge,
		MovementZone movementZone
	)
	JMP_THIS(0x42D170);


	// main function
	PathType* FindPathRegular
	(
		CellStruct* pSource,
		CellStruct* pDestination,
		FootClass* pFoot,
		int* a5,
		int a6,
		bool a7
	)
	JMP_THIS(0x429A90);

	// hierarchical quick find
	bool FindHierarchical
	(
		CellStruct* pSource,
		CellStruct* pDestination,
		MovementZone movementZone,
		FootClass* pFoot
	)
	JMP_THIS(0x42C290);


	// auxiliary function in FindPathRegular
	WorkPathQueueNode* CreateNode
	(
		WorkPathQueueNode* a2,
		CellClass** a3,
		CellStruct* a4,
		float a5
	)
	JMP_THIS(0x42A460);

	// auxiliary function in FindPathRegular
	void PostProcessCells(FootClass* pFoot) JMP_THIS(0x42ACF0);
	// auxiliary function in PostProcessCells
	FootClass* GetOccupier(CellStruct* pCheckCell, int level) JMP_THIS(0x42B080);

	// auxiliary function in FindPathRegular
	double GetMovementCost
	(
		CellClass** a2,
		CellClass** a3,
		bool a4,
		int a5,
		FootClass* pFoot
	)
	JMP_THIS(0x429830);

	// auxiliary function in FindPathRegular
	PathType* BuildFinalPath(WorkPathQueueNode* a2, int* a3) JMP_THIS(0x42AA90);

	// auxiliary function in FindPathRegular
	void ProcessFinalPath(PathType* pPath, FootClass* pFoot) JMP_THIS(0x42B210);
	// auxiliary function in ProcessFinalPath
	int FixupFinalPath
	(
		FootClass* pFoot,
		int* facings,
		int* levels,
		int a5,
		int a6,
		CellStruct* pCurCell
	)
	JMP_THIS(0x42B420);

	// auxiliary function in FindPathRegular
	void OptimizeFinalPath(PathType* pPath, FootClass* pFoot) JMP_THIS(0x42B7F0);
	// auxiliary function in OptimizeFinalPath
	void AdjacentCell
	(
		int* a2,
		int a3,
		int a4,
		int* a5,
		CellStruct* pAdjCell
	)
	JMP_THIS(0x42BCA0);
	// auxiliary function in OptimizeFinalPath
	void PlotStraightLine
	(
		int* pDirs,
		int maxLength,
		CellStruct* pCurrent,
		CellStruct* pVector,
		FootClass* pFoot,
		int curLevel,
		bool allowThreats
	)
	JMP_THIS(0x42BE20);


	// All temp variable name
	__int8 unknown_byte_0;
	bool FindBridgeOwner;
	__int8 unknown_byte_2;
	bool CanFind;
	float FinderCoefficient;
	bool IsAlt;
	WorkPathHeapData* WorkPathHeapData; // -> [131072]
	WorkPathQueueNode** WorkPathPriorityQueueBuffer; // -> [65536]
	PriorityQueueClass<WorkPathQueueNode>* WorkPathPriorityQueueDatas; // Count = 65537
	int* AltCounts;
	int* Counts;
	float* Distances;
	float* AltDistances;
	int FindCount;
	SpeedType FinderSpeedType;
	int SourceCellLevel;
	int DestinationCellLevel;
	bool Finding;
	int FindMode;
	int* BothWayPassabilityCounts[3];
	int* OneWayPassabilityCounts[3];
	float* OneWayPassabilityCoefficients[3];
	HierarchicalQueueNode** HierarchicalPriorityQueueBuffer; // -> [10000]
	PriorityQueueClass<HierarchicalQueueNode>* HierarchicalPriorityQueueDatas; // Count = 10001
	int CellStructCount;
	CellStruct CellStructBuffer;
	DynamicVectorClass<unsigned int> SubzonesIndexes[3];
	PassabilityIndexData PassabilityIndexes[3];
	int PassabilityCounts[3];
};

class AStarClassFake final : public AStarClass
{
public:

#ifdef DISABLED_ASTAR_REIMPL

	PathType* DoPathfinding_AsVanilla
	(
		CellStruct* pSource,
		CellStruct* pDestination,
		FootClass* pFoot,
		int* a5,
		int a6,
		MovementZone movementZone,
		int mode
	);

	int AttemptPath_AsVanilla
	(
		CellStruct* pSource,
		CellStruct* pDestination,
		FootClass* pFoot,
		bool checkSourceBridge,
		bool checkDestinationBridge,
		MovementZone movementZone
	);

	PathType* FindPathRegular_AsVanilla
	(
		CellStruct* pSource,
		CellStruct* pDestination,
		FootClass* pFoot,
		int* a5,
		int a6,
		bool a7
	);

	bool FindHierarchical_AsVanilla
	(
		CellStruct* pSource,
		CellStruct* pDestination,
		MovementZone movementZone,
		FootClass* pFoot
	);

	WorkPathQueueNode* CreateNode_AsVanilla
	(
		WorkPathQueueNode* a2,
		CellClass** a3,
		CellStruct* a4,
		float a5
	);

	void PostProcessCells_AsVanilla(FootClass* pFoot);

	double GetMovementCost
	(
		CellClass** a2,
		CellClass** a3,
		bool a4,
		int a5,
		FootClass* pFoot
	);

	PathType* BuildFinalPath_AsVanilla(WorkPathQueueNode* a2, int* a3);

	void ProcessFinalPath_AsVanilla(PathType* pPath, FootClass* pFoot);

	int FixupFinalPath_AsVanilla
	(
		FootClass* pFoot,
		int* facings,
		int* levels,
		int a5,
		int a6,
		CellStruct* pCurCell
	);

	void OptimizeFinalPath_AsVanilla(PathType* pPath, FootClass* pFoot);

	void AdjacentCell_AsVanilla
	(
		int* a2,
		int a3,
		int a4,
		int* a5,
		CellStruct* pAdjCell
	);

#endif

	static void PlotStraightLine_Reimplement
	(
		int* const pDirs,
		const int maxLength,
		const CellStruct* const pCurCell,
		const CellStruct* const pVecCell,
		const FootClass* const pFoot,
		int curLevel,
		const bool allowThreats
	)
	{
		if (ENABLE_ASTAR_OPTIMIZE)
			AStarClassFake::PlotStraightLine_Optimized(pDirs, maxLength, pCurCell, pVecCell, pFoot, curLevel, allowThreats);
		else
			AStarClassFake::PlotStraightLine_AsVanilla(pDirs, maxLength, pCurCell, pVecCell, pFoot, curLevel, allowThreats);
	}

	static bool PlotStraightLine_AsVanilla
	(
		int* const pDirs,
		const int maxLength,
		const CellStruct* const pCurCell,
		const CellStruct* const pVecCell,
		const FootClass* const pFoot,
		int& curLevel,
		const bool allowThreats
	);

	static void PlotStraightLine_Optimized
	(
		int* const pDirs,
		const int maxLength,
		const CellStruct* const pCurCell,
		const CellStruct* const pVecCell,
		const FootClass* const pFoot,
		int curLevel,
		const bool allowThreats
	);
};

#endif
