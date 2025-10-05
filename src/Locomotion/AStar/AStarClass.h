#pragma once

#pragma region Toggle

#define ENABLE_ASTAR_REIMPL

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

	// 结构体定义

	struct PathFinderData
	{
		// 路径起点单元格
		CellStruct StartCell;

		// 路径总距离
		int TotalDistance;

		// 路径节点数量
		int PathLength;

		// 移动方向数组
		int* Directions;

		// 不明
		int unknown_int_10;

		// 单元格高度数组
		int* Levels;

		// 不明
		CellStruct unknown_cellstruct_18;

		// 不明
		int unknown_int_1C;
	};

	struct PathNode
	{
		// 指向单元格组的指针
		const CellClass* const* CellItems;

		// 单元格高度
		int Level;

		// 前一个路径节点
		PathNode* PreviousNode;
	};

	struct PathNodeBuffer
	{
		// 路径节点缓冲区
		PathNode Nodes[131072];

		// 当前节点数量
		int Count;
	};

	struct PassabilityData
	{
		// 可通行性索引
		unsigned short Indices[500];
	};

	struct PathQueueNode
	{
		// 路径节点数据
		PathNode* NodeData;

		// 路径成本
		float PathCost;

		// 总成本(路径成本+启发式成本)
		float TotalCost;

		// 节点计数
		int NodeCount;

		// 用于堆排序的比较函数
		bool operator<(const PathQueueNode& another) const
		{
			return TotalCost < another.TotalCost;
		}
	};

	struct PathQueueNodeBuffer
	{
		// 节点缓冲区
		PathQueueNode Nodes[131072]; // [65536] => [131072]

		// 当前节点数量
		int Count;
	};

	struct HierarchicalNode
	{
		// 上个节点的缓存索引
		int PreviousNodeIndex;

		// 该节点的子区域索引
		int SubzoneIndex;

		// 节点成本
		float Cost;

		// 计数
		int Count;

		// 用于堆排序的比较函数
		bool operator<(const HierarchicalNode& another) const
		{
			if (Cost < another.Cost)
				return true;

			if (Cost > another.Cost)
				return false;

			return Count > another.Count;
		}
	};

	struct HierarchicalNodeBuffer
	{
		// 节点缓冲区
		HierarchicalNode Nodes[10000];
	};

	// 实体引用

	DEFINE_REFERENCE(AStarClass, Instance, 0x87E8B8u)
	DEFINE_REFERENCE(AStarClass::PathFinderData, PathData, 0x89A2D8u)
	DEFINE_REFERENCE(const int, MapSides, 0x89C2DCu)
	DEFINE_REFERENCE(const CellClass*, InvalidCell, 0x89C2E0u)

	// 数组引用

	DEFINE_ARRAY_REFERENCE(const int, [8], BridgeDir0OffsetIndexes, 0x7E3710)
	DEFINE_ARRAY_REFERENCE(const int, [8], BridgeDir1OffsetIndexes, 0x7E3730)
	DEFINE_ARRAY_REFERENCE(const int, [9], BridgeDirOffsets, 0x7E3750)
	DEFINE_ARRAY_REFERENCE(const int, [8], DirCellOffsets, 0x7E3774)
	DEFINE_ARRAY_REFERENCE(const float, [8], PassabilityCoefficients, 0x7E3794)
	DEFINE_ARRAY_REFERENCE(const float, [8], MoveCosts, 0x81870C)
	DEFINE_ARRAY_REFERENCE(const float, [8], DirPathCosts, 0x81872C)
	DEFINE_ARRAY_REFERENCE(const int, [8], DirSides, 0x89A304)

	// 构造/析构函数

	AStarClass()
		JMP_THIS(0x42A6D0);

	~AStarClass()
		JMP_THIS(0x42A900);

	// 初始化函数

	void CleanUp()
		JMP_THIS(0x42A5B0);

	void ClearPassability()
		JMP_THIS(0x42C1C0);

	void ReinitCostArrays(
		const RectangleStruct* const pMapRect
	) JMP_THIS(0x42AC00);

	// 记录函数

	void RecordCellIndex(
		const FootClass* const pFoot
	) JMP_THIS(0x42CCD0);

	void RegisterCellIndex(
		const int index,
		const int dataIndex
	) JMP_THIS(0x42CF80);

	// 静态函数

	// static CellStruct* __fastcall NextPathCell(CellStruct* pBuffer, CellStruct* pCurrent, int dir) JMP_STD(0x42D490);
	static CellStruct NextPathCell(
		const CellStruct cell,
		const int dir
	);

	// 接口函数

	// 主要路径查找接口
	PathFinderData* FindPath(
		const CellStruct* const pStart,
		const CellStruct* const pEnd,
		const FootClass* const pFoot,
		int* const pDirs,
		int maxSteps,
		MovementZone movementZone,
		const int mode
	) JMP_THIS(0x42C900);

	// 轻量级路径检查接口
	int AttemptPath(
		const CellStruct* const pStart,
		const CellStruct* const pEnd,
		const FootClass* const pFoot,
		const bool checkStartBridge,
		const bool checkEndBridge,
		MovementZone movementZone
	) JMP_THIS(0x42D170);

	// 功能函数

	// 常规路径查找
	PathFinderData* FindRegularPath(
		const CellStruct* const pStart,
		const CellStruct* const pEnd,
		const FootClass* const pFoot,
		int* const pDirs,
		int maxSteps,
		const bool useHierarchical
	); // JMP_THIS(0x429A90)

	// 分层快速查找
	bool FindHierarchicalPath(
		const CellStruct* const pStart,
		const CellStruct* const pEnd,
		const MovementZone movementZone,
		const FootClass* const pFoot
	); // JMP_THIS(0x42C290)

	// 新增可跨区域移动类型分层快速查找
	bool FindHierarchicalPath_Comprehensive(
		const CellStruct* const pStart,
		const CellStruct* const pEnd,
		const MovementZone movementZone,
		const FootClass* const pFoot
	);

	// 辅助函数

	// 创建路径节点 ∈ FindRegularPath
	PathQueueNode* CreatePathNode(
		const PathQueueNode* const pPrevNode,
		const CellClass* const* const pCellPtr,
		const CellStruct* const pCoords,
		const float cost
	) JMP_THIS(0x42A460);

	// 后处理单元格 ∈ FindRegularPath
	void PostProcessCells(
		const FootClass* const pFoot
	) JMP_THIS(0x42ACF0);

	// 获取单元格占用者 ∈ PostProcessCells
	FootClass* GetOccupier(
		const CellStruct* const pCell,
		const int level
	) const JMP_THIS(0x42B080);

	// 计算移动成本 ∈ FindRegularPath
	double CalculateMoveCost(
		const CellClass* const* const pFromCellPtr,
		const CellClass* const* const pToCellPtr,
		const bool isAlternate,
		const Move moveType,
		const FootClass* const pFoot
	) const; // JMP_THIS(0x429830)

	// 构建最终路径 ∈ FindRegularPath
	PathFinderData* BuildFinalPath(
		PathQueueNode* const pEndNode,
		int* const pDirs
	) const JMP_THIS(0x42AA90);

	// 处理最终路径 ∈ FindRegularPath
	void ProcessFinalPath(
		PathFinderData* const pPath,
		const FootClass* const pFoot
	) const; // JMP_THIS(0x42B210)

	// 修正最终路径 ∈ ProcessFinalPath
	int AdjustFinalPath(
		const FootClass* const pFoot,
		int* const pDirs,
		const int* const pLevels,
		const int steps,
		int offset,
		CellStruct* const pCurrent
	) const; // JMP_THIS(0x42B420)

	// 优化最终路径 ∈ FindRegularPath
	void OptimizeFinalPath(
		PathFinderData* const pPath,
		const FootClass* const pFoot
	) const; // JMP_THIS(0x42B7F0)

	// 获取相邻单元格 ∈ OptimizeFinalPath
	void GetFinalStepCell(
		const int* const pDirs,
		const int segmentEndIdx,
		const int segmentStartIdx,
		int* const pOutIdx,
		CellStruct* const pAdjacent
	) const; // JMP_THIS(0x42BCA0)

	// 绘制直线路径 ∈ OptimizeFinalPath
	bool PlotStraightPath(
		int* const pDirs,
		const int maxLength,
		const CellStruct* const pCurrent,
		const CellStruct* const pVector,
		const FootClass* const pFoot,
		const int curLevel,
		const bool allowThreats
	) const; // JMP_THIS(0x42BE20)

	// 成员变量（分层：大区块、子区域、单元格）

	// 不明
	char unknown_byte_0; // false

	// 是否检查桥梁方向
	bool FindBridgeDir; // false

	// 不明
	char unknown_byte_2; // false

	// 是否能找到路径
	bool CanFindPath; // true

	// 路径成本因子
	float PathCostFactor; // 1.0

	// 是否是桥梁路径
	bool IsAlt; // true

	// 路径节点缓冲区
	PathNodeBuffer* PathNodeBuffer;

	// 路径队列缓冲区
	PathQueueNodeBuffer* PathQueueBuffer;

	// 路径优先级队列
	PriorityQueueClass<PathQueueNode>* PathQueue; // Count = 65537 => 131073

	// 访问标志数组
	int* VisitCounts;

	// 桥梁路径访问标志数组
	int* AltVisitCounts;

	// 桥梁路径距离数组
	float* AltDistances;

	// 距离数组
	float* Distances;

	// 访问标记（每进行一次寻路，递增一次）
	int SearchID;

	// 单位速度类型
	SpeedType FinderSpeedType;

	// 起始高度
	int StartLevel;

	// 目标高度
	int EndLevel;

	// 是否正在搜索
	bool IsSearching; // true

	// 搜索模式
	int FindMode; // 0

	// 分层已访问节点
	int* LevelVisitedMarkers[3];

	// 分层待探索队列
	int* OpenSetMarkers[3];

	// 分层实际代价队列
	float* GCostArray[3];

	// 分层节点缓冲区
	HierarchicalNodeBuffer* HierarchyBuffer;

	// 分层优先级队列
	PriorityQueueClass<HierarchicalNode>* HierarchyQueue; // Count = 10001

	// 单元格计数
	int PathLength;

	// 单元格缓冲区
	CellStruct CellStructBuffer;

	// 区域索引数组
	DynamicVectorClass<unsigned int> ZoneIndices[3];

	// 可通行性数据
	PassabilityData PassabilityData[3];

	// 可通行性计数
	int PassabilityCounts[3];
};

#endif
