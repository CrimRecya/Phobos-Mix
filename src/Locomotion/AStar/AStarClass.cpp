#include "AStarClass.h"

#ifdef ENABLE_ASTAR_REIMPL

#include <TubeClass.h>
#include <Ext/Techno/Body.h>

#pragma region NextPathCell

CellStruct AStarClass::NextPathCell(
	const CellStruct cell,
	const int dir
)
{
	if (dir != 8)
		return cell + Unsorted::AdjacentCell[dir];

	const int tubeIndex = MapClass::Instance.GetCellAt(cell)->TubeIndex;
	return (tubeIndex == -1) ? CellStruct::Empty : TubeClass::Array.Items[tubeIndex]->ExitCell;
}

#pragma endregion

AStarClass::PathFinderData* AStarClass::FindRegularPath(
	const CellStruct* const pStart,
	const CellStruct* const pEnd,
	const FootClass* const pFoot,
	int* const pDirs,
	int maxSteps,
	const bool useHierarchical
)
{
	int step = 0;

	// 获取终点和起点的地图单元格
	const auto pCellsFromEnd = &MapClass::Instance.Cells.Items[MapClass::GetCellIndex(*pEnd)];
	const auto pEndCell = *pCellsFromEnd;
	const auto pCellsFromStart = &MapClass::Instance.Cells.Items[MapClass::GetCellIndex(*pStart)];
	const auto pStartCell = *pCellsFromStart;

	if (!pStartCell || !pEndCell)
		return nullptr;

	// 确定起点和终点的高度
	const auto absType = pFoot->WhatAmI();
	this->EndLevel = (absType == AbstractType::Aircraft || !pEndCell->ContainsBridge()) ? pEndCell->Level : pEndCell->Level + 4;
	this->StartLevel = (absType == AbstractType::Aircraft || !pFoot->OnBridge) ? pStartCell->Level : pStartCell->Level + 4;
	const auto pType = pFoot->GetTechnoType();
	this->FinderSpeedType = pType->SpeedType;
	const bool isTrain = pType->IsTrain;

	if (isTrain && pStartCell->ContainsBridge() && std::abs(pFoot->Location.Z / Unsorted::LevelHeight - this->StartLevel) > 2)
		this->StartLevel += 4;

	// 初始化寻路数据
	const auto pCount = this->TwoWayPassCounts[0];
	this->PathLength = 0;
	this->CellStructBuffer = *pStart;

	// 创建起始路径节点
	auto pPathNode = this->CreatePathNode(0, pCellsFromStart, pEnd, 0.0f);

	do
	{
		// 起点和终点相同则直接退出
		if (pStart->X == pEnd->X && pStart->Y == pEnd->Y && this->StartLevel == this->EndLevel)
			return nullptr;

		if (this->FindMode)
			this->AStarClass::PostProcessCells(pFoot);

		const int astarStartIndex = pStartCell->MapCoords.X + AStarClass::MapSides * pStartCell->MapCoords.Y;

		// 标记起点为已访问，并设置初始距离
		if (this->StartLevel <= pStartCell->Level)
		{
			this->VisitCounts[astarStartIndex] = this->SearchID;
			this->Distances[astarStartIndex] = 0.0;
		}
		else
		{
			this->AltVisitCounts[astarStartIndex] = this->SearchID;
			this->AltDistances[astarStartIndex] = 0.0;
		}

		// 火车根据朝向预标记非前方单元格
		if (isTrain)
		{
			int dirOffset = 0;

			for (const auto dirCellOffset : AStarClass::DirCellOffsets)
			{
				const int absFace = std::abs(static_cast<int>(pFoot->PrimaryFacing.Current().GetFacing<8>()) - dirOffset);

				if (absFace > 2 && absFace < 6)
				{
					const auto pAdjCell = pCellsFromStart[dirCellOffset];
					const int astarAdjIndex = pAdjCell->MapCoords.X + AStarClass::MapSides * pAdjCell->MapCoords.Y;

					if (this->StartLevel <= pAdjCell->Level + 1)
					{
						this->VisitCounts[astarAdjIndex] = this->SearchID;
						this->Distances[astarAdjIndex] = 0.0;
					}
					else
					{
						this->AltVisitCounts[astarAdjIndex] = this->SearchID;
						this->AltDistances[astarAdjIndex] = 0.0;
					}
				}

				++dirOffset;
			}
		}

		const bool isPassiveUnit = absType == AbstractType::Unit && static_cast<const UnitClass*>(pFoot)->Type->Passive;

		// 最大步数默认值
		if (maxSteps < 0)
			maxSteps = 65527;

		if (pPathNode)
		{
			// 主寻路循环
			do
			{
				// 是否达到步数限制
				if (step >= maxSteps)
					break;

				const auto pCellsInChecking = pPathNode->NodeData->CellItems;

				// 是否已到达目标单元格
				if (pCellsInChecking == pCellsFromEnd && pPathNode->NodeData->Level == this->EndLevel)
					break;

				AStarClass::PathQueueNode* pBestCandidateNode = nullptr;
				int astarCheckCellIndex = (*pCellsInChecking)->MapCoords.X + AStarClass::MapSides * (*pCellsInChecking)->MapCoords.Y;

				// 遍历周围的方向和当前隧道
				for (int dir = 0; dir <= 8; ++dir)
				{
					const CellClass* const* pCellsPtr = nullptr;

					// 获取方向上连接的格子
					if (dir == 8)
					{
						const int tubeIndex = (*pCellsInChecking)->TubeIndex;

						if (tubeIndex == -1)
							pCellsPtr = &AStarClass::InvalidCell;
						else
							pCellsPtr = &MapClass::Instance.Cells.Items[MapClass::GetCellIndex(TubeClass::Array.Items[tubeIndex]->ExitCell)];
					}
					else
					{
						pCellsPtr = &pCellsInChecking[AStarClass::DirCellOffsets[dir]];
					}

					const auto pCell = *pCellsPtr;

					if (!pCell)
						continue;

					// 计算连接的格子的索引
					const auto& checkCell = pCell->MapCoords;
					const int astarCheckNextIndex = (dir == 8) ? (checkCell.X + AStarClass::MapSides * checkCell.Y) : (astarCheckCellIndex + AStarClass::DirSides[dir]);
					const bool notAlternate = !pCell->ContainsBridge() || std::abs(this->StartLevel - pCell->Level) <= 1;
					const int pathIndex = reinterpret_cast<int(__thiscall*)(MapClass*, const CellStruct*)>(0x56D3F0)(&MapClass::Instance, &checkCell);
					const unsigned int cellPassabilityIndex = static_cast<unsigned short>(MapClass::Instance.LevelAndPassabilityStruct2pointer_70[pathIndex].word_0[0]);

					// 检查是否已被更优路径访问，或被分层寻路跳过
					if (notAlternate
						? (pCount[cellPassabilityIndex] != this->SearchID && !pCell->BlockedNeighbours && useHierarchical // TODO pCount[cellPassabilityIndex] 不知为何和原版总是不一样
							|| this->VisitCounts[astarCheckNextIndex] == this->SearchID && this->Distances[astarCheckNextIndex] < (pPathNode->PathCost + 1.009))
						: (this->AltVisitCounts[astarCheckNextIndex] == this->SearchID && this->AltDistances[astarCheckNextIndex] < (pPathNode->PathCost + 1.009)))
					{
						continue;
					}

					// 检查格子占用情况
					auto moveType = pFoot->IsCellOccupied(const_cast<CellClass*>(pCell), static_cast<FacingType>(dir), this->StartLevel, const_cast<CellClass*>(*pCellsInChecking), this->IsAlt);

					if (isTrain && moveType < Move::No)
						moveType = Move::OK;

					// 计算格子移动成本
					const float cost = static_cast<float>((dir == 8)
						? Math::max(std::abs((*pCellsInChecking)->MapCoords.X - checkCell.X), std::abs((*pCellsInChecking)->MapCoords.Y - checkCell.Y))
						: this->CalculateMoveCost(pCellsInChecking, pCellsPtr, !notAlternate, moveType, pFoot) * this->PathCostFactor + AStarClass::DirPathCosts[dir]);

					if (moveType >= Move::No)
					{
						// 终点不可到达，跳出循环
						if (pCellsPtr == pCellsFromEnd && !isPassiveUnit && std::abs(this->StartLevel - this->EndLevel) <= 1)
							goto BREAK_STEP_LOOP;

						continue;
					}

					// 检查是否已访问过该单元格
					if (notAlternate ? (this->VisitCounts[astarCheckNextIndex] == this->SearchID) : (this->AltVisitCounts[astarCheckNextIndex] == this->SearchID))
						continue;

					// 创建新的路径节点
					const auto pNewPathNode = this->CreatePathNode(pPathNode, pCellsPtr, pEnd, cost);

					do
					{
						// 将新节点加入优先队列（堆）
						if (pBestCandidateNode)
						{
							// 堆插入
							const auto pPathQueue = this->PathQueue;
							const int count = pPathQueue->Count;
							int newCount = count + 1;

							if (newCount < pPathQueue->Capacity)
							{
								if (count == -1)
								{
									int harfNewCount = newCount >> 1;
									const float newCost = pNewPathNode->TotalCost;

									for ( ; newCount > 1; harfNewCount >>= 1)
									{
										const auto pQueueNodes = pPathQueue->Nodes;
										const auto pParentNode = pQueueNodes[harfNewCount];

										if (pParentNode->TotalCost <= newCost)
											break;

										pQueueNodes[newCount] = pParentNode;
										newCount = harfNewCount;
									}

									pPathQueue->Nodes[newCount] = pNewPathNode;
									pPathQueue->Count = newCount;

									// 更新队列边界
									if (pNewPathNode > pPathQueue->LMost)
										pPathQueue->LMost = pNewPathNode;

									if (pNewPathNode < pPathQueue->RMost)
										pPathQueue->RMost = pNewPathNode;

									break;
								}
								else
								{
									int harfNewCount = newCount >> 1;
									const float newCost = pBestCandidateNode->TotalCost;

									for ( ; newCount > 1; harfNewCount >>= 1)
									{
										const auto pQueueNodes = pPathQueue->Nodes;
										const auto pParentNode = pQueueNodes[harfNewCount];

										if (pParentNode->TotalCost <= newCost)
											break;

										pQueueNodes[newCount] = pParentNode;
										newCount = harfNewCount;
									}

									pPathQueue->Nodes[newCount] = pBestCandidateNode;
									pPathQueue->Count = newCount;

									// 更新队列边界
									if (pBestCandidateNode > pPathQueue->LMost)
										pPathQueue->LMost = pBestCandidateNode;

									if (pBestCandidateNode < pPathQueue->RMost)
										pPathQueue->RMost = pBestCandidateNode;
								}
							}
						}

						// 设置新的最佳节点
						pBestCandidateNode = pNewPathNode;
					}
					while (false);

					// 标记单元格为已访问
					if (notAlternate)
					{
						this->VisitCounts[astarCheckNextIndex] = this->SearchID;
						this->Distances[astarCheckNextIndex] = pNewPathNode->PathCost;
					}
					else
					{
						this->AltVisitCounts[astarCheckNextIndex] = this->SearchID;
						this->AltDistances[astarCheckNextIndex] = pNewPathNode->PathCost;
					}

					// 更新路径长度和缓冲区
					const int pathLength = this->PathLength;

					if (cellPassabilityIndex == static_cast<unsigned int>(this->PassabilityData[0].Indices[pathLength + 1]))
					{
						this->PathLength = pathLength + 1;
						this->CellStructBuffer = checkCell;
					}
				}

				// 从优先队列中获取下一个最佳节点
				const auto pPathQueue = this->PathQueue;

				if (!pBestCandidateNode)
				{
					if (const int count = pPathQueue->Count)
					{
						// 堆提取
						const auto pQueueNodes = pPathQueue->Nodes;
						const auto pExtractedNode = pQueueNodes[1];
						pQueueNodes[1] = pQueueNodes[count];
						pQueueNodes[count] = nullptr;
						const int newCount = count - 1;
						pPathQueue->Count = newCount;
						int heapIndex = 1;
						int childIndex = (newCount < 2 || pQueueNodes[1]->TotalCost <= pQueueNodes[2]->TotalCost) ? 1 : 2;

						do
						{
							// 堆下沉
							if (newCount < 3 || pQueueNodes[childIndex]->TotalCost <= pQueueNodes[3]->TotalCost)
							{
								if (childIndex == 1)
									break;
							}
							else
							{
								childIndex = 3;
							}

							do
							{
								std::swap(pQueueNodes[heapIndex], pQueueNodes[childIndex]);
								heapIndex = childIndex;
								const int leftChildIndex = 2 * childIndex;
								const int rightChildIndex = leftChildIndex + 1;

								if (leftChildIndex <= newCount && pQueueNodes[childIndex]->TotalCost > pQueueNodes[leftChildIndex]->TotalCost)
									childIndex = leftChildIndex;

								if (rightChildIndex <= newCount && pQueueNodes[childIndex]->TotalCost > pQueueNodes[rightChildIndex]->TotalCost)
									childIndex = rightChildIndex;
							}
							while (childIndex != heapIndex);
						}
						while (false);

						// 更新新的起点节点为提取的节点
						pPathNode = pExtractedNode;
					}
					else
					{
						// 无可提取节点，退出循环
						pPathNode = nullptr;
						++step;
						break;
					}
				}
				else
				{
					do
					{
						if (const int count = pPathQueue->Count)
						{
							const auto pQueueNodes = pPathQueue->Nodes;
							const auto pExtractedNode = pQueueNodes[1];

							// 比较最佳候选节点和队列顶部节点
							if (pExtractedNode->TotalCost <= pBestCandidateNode->TotalCost)
							{
								// 用候选节点替换顶部节点
								pQueueNodes[1] = pBestCandidateNode;
								int heapIndex = 1;
								int childIndex = (count < 2 || pQueueNodes[1]->TotalCost <= pQueueNodes[2]->TotalCost) ? 1 : 2;

								// 堆下沉
								do
								{
									if (count < 3 || pQueueNodes[childIndex]->TotalCost <= pQueueNodes[3]->TotalCost)
									{
										if (childIndex == 1)
											break;
									}
									else
									{
										childIndex = 3;
									}

									do
									{
										std::swap(pQueueNodes[heapIndex], pQueueNodes[childIndex]);
										heapIndex = childIndex;
										const int leftChildIndex = 2 * childIndex;
										const int rightChildIndex = leftChildIndex + 1;

										if (leftChildIndex <= count && pQueueNodes[childIndex]->TotalCost > pQueueNodes[leftChildIndex]->TotalCost)
											childIndex = leftChildIndex;

										if (rightChildIndex <= count && pQueueNodes[childIndex]->TotalCost > pQueueNodes[rightChildIndex]->TotalCost)
											childIndex = rightChildIndex;
									}
									while (childIndex != heapIndex);
								}
								while (false);

								// 更新新的起点节点为提取的节点
								pPathNode = pExtractedNode;
								break;
							}
						}

						// 更新新的起点节点为当前最佳节点
						pPathNode = pBestCandidateNode;
					}
					while (false);
				}

				// 更新当前高度
				if (pPathNode)
					this->StartLevel = pPathNode->NodeData->Level;

				++step;
			}
			while (pPathNode);

BREAK_STEP_LOOP:
			if (step != 10000 && pPathNode && step != maxSteps && pPathNode->NodeCount >= 2)
				break;
		}

		if (this->FindMode)
			this->PostProcessCells(pFoot);

		return nullptr;
	}
	while (false);

	// 构建最终路径
	const auto pData = this->BuildFinalPath(pPathNode, pDirs);

	// 处理和优化最终路径
	this->ProcessFinalPath(pData, pFoot);
	this->OptimizeFinalPath(pData, pFoot);

	if (this->FindMode)
		this->PostProcessCells(pFoot);

	return pData;
}

#pragma region CalculateMoveCost

double AStarClass::CalculateMoveCost(
	const CellClass* const* const pFromCellPtr,
	const CellClass* const* const pToCellPtr,
	const bool isAlternate,
	const Move moveType,
	const FootClass* const pFoot
) const
{
	const auto pToCell = *pToCellPtr;
	const auto pFromCell = *pFromCellPtr;
	float moveCost = AStarClass::MoveCosts[static_cast<int>(moveType)];

	if (moveType == Move::MovingBlock)
	{
		if (const int mode = this->FindMode)
		{
			moveCost = (mode == 2) ? 1000.0f : 4.0f;
		}
		else
		{
			auto pCellObj = isAlternate ? pToCell->AltObject : pToCell->FirstObject;

			for (int step = 0; pCellObj; )
			{
				if (const auto pCellFoot = abstract_cast<FootClass*, true>(pCellObj))
				{
					int dir = 0;

					if (pCellFoot->SpeedPercentage == 0.0)
					{
						dir = pCellFoot->PathDirections[0];

						if (dir == -1)
							break;
					}
					else
					{
						dir = pCellFoot->PrimaryFacing.Current().GetFacing<8>();
					}

					const auto pAdjCell = MapClass::Instance.GetCellAt(Unsorted::AdjacentCell[dir & 7] + pCellObj->GetMapCoords());
					pCellObj = (pAdjCell->ContainsBridge() && (pCellObj->OnBridge || (pCellObj->GetCell()->Level - pAdjCell->Level) > 2))
						? pAdjCell->AltObject
						: pAdjCell->FirstObject;

					if (++step < 10)
						continue;
				}

				moveCost = 4.0f;
				break;
			}
		}
	}

	const auto flags = pToCell->Flags;

	if (flags & CellFlags::Tube)
		moveCost *= 4.0f;

	if (!isAlternate || !this->FindBridgeDir)
		return moveCost;

	const auto deltaCell = pToCell->MapCoords - pFromCell->MapCoords;
	const int dirOffsetIndex = AStarClass::BridgeDirOffsets[4 + (3 * deltaCell.Y) + deltaCell.X];
	const auto& [pBridgeCheckCell, pBridgeReverseCheckCell] = (flags & CellFlags::BridgeDir)
		? std::make_pair(pToCellPtr[AStarClass::BridgeDir1OffsetIndexes[dirOffsetIndex]], pToCellPtr[AStarClass::BridgeDir1OffsetIndexes[(dirOffsetIndex - 4) & 7]])
		: std::make_pair(pToCellPtr[AStarClass::BridgeDir0OffsetIndexes[dirOffsetIndex]], pToCellPtr[AStarClass::BridgeDir0OffsetIndexes[(dirOffsetIndex - 4) & 7]]);

	if (!pBridgeCheckCell->ContainsBridge())
		return 10.0f * moveCost;

	return pBridgeReverseCheckCell->ContainsBridge() ? 2.0f * moveCost : moveCost;
}

#pragma region ProcessFinalPath

#pragma endregion

void AStarClass::ProcessFinalPath(
	PathFinderData* const pPath,
	const FootClass* const pFoot
) const
{
	const int pathLength = pPath->PathLength - 1;

	if (pathLength <= 0)
		return;

	const int* const pLevels = pPath->Levels;
	int* const pDirs = pPath->Directions;

	// 初始化变量
	int lastDir = -1;
	int diagonalDir = -1;
	int step = 0;
	int segmentLength = 0;
	bool adjustSegment = false;
	int adjustIndex = 0;
	int adjustOffset = 0;

	// 初始化当前和下一个单元格位置
	auto currentCell = pPath->StartCell;
	auto nextCell = currentCell;

	// 遍历整个路径
	do
	{
		if (adjustSegment)
		{
			// 需要调整
			if (pDirs[adjustOffset + adjustIndex] != lastDir)
			{
				// 方向改变，完成当前段的调整
				step += AStarClass::AdjustFinalPath(pFoot, &pDirs[step], &pLevels[step], segmentLength, adjustOffset, &currentCell);

				// 重置状态，开始新段
				segmentLength = 1;
				adjustSegment = false;

				nextCell = Unsorted::AdjacentCell[pDirs[step] & 7] + currentCell;
				diagonalDir = pDirs[step];
			}
			else
			{
				// 方向未改变，继续累积调整偏移量
				++adjustOffset;
			}
		}
		else
		{
			// 正常处理
			const int currentDir = pDirs[step + segmentLength];
			const int diffDir = (currentDir - diagonalDir) & 7;

			if (currentDir == diagonalDir)
			{
				// 方向相同，增加直线段长度
				++segmentLength;
			}
			else if ((diffDir != 2 && diffDir != 6) // 不是90度转角
				|| diagonalDir == -1 // 无效的对角线方向
				|| diagonalDir == 8 // 对角线方向是隧道
				|| currentDir == 8) // 当前方向是隧道
			{
				// 不满足调整条件，直接开始新段
				step += segmentLength;
				segmentLength = 1;

				// 更新对角线方向
				diagonalDir = (currentDir & 1) ? currentDir : -1;
				currentCell = nextCell;
			}
			else
			{
				// 满足调整条件，准备进入调整
				adjustSegment = true;
				lastDir = pDirs[step + segmentLength];
				adjustOffset = 1;
				adjustIndex = step + segmentLength;
			}

			nextCell = AStarClass::NextPathCell(nextCell, currentDir);
		}
	}
	while ((step + segmentLength) < pathLength && (adjustOffset + adjustIndex) < pathLength);

	// 处理最后一个未完成的调整段
	if (adjustSegment)
		AStarClass::AdjustFinalPath(pFoot, &pDirs[step], &pLevels[step], segmentLength, adjustOffset, &currentCell);
}

#pragma endregion

#pragma region AdjustFinalPath

int AStarClass::AdjustFinalPath(
	const FootClass* const pFoot,
	int* const pDirs,
	const int* const pLevels,
	const int steps,
	int offset,
	CellStruct* const pCurrent
) const
{
	// 获取当前段的起始和结束方向
	const int firstDir = pDirs[0];
	const int lastDir = pDirs[steps];

	int avgDir = (firstDir + lastDir) >> 1;
	{
		// 检查平均方向是否有效
		const int checkDir = avgDir + 1;

		if (checkDir != lastDir && checkDir != firstDir)
			avgDir = 0;
	}

	// 隧道
	if (firstDir == 8 || lastDir == 8)
	{
		const int count = steps + offset;

		if (count > 0)
		{
			// 沿隧道方向移动整个段
			int i = 0;
			auto cell = *pCurrent;

			do
			{
				cell = AStarClass::NextPathCell(cell, pDirs[i]);
			}
			while (++i < count);

			*pCurrent = cell;
		}

		return count;
	}

	// 准备计算中间位置
	auto midCell = *pCurrent;

	// 调整偏移量确保不超过总步数
	if (steps < offset)
	{
		offset = steps;
	}
	else if (offset < steps)
	{
		const int count = steps - offset;

		if (count > 0)
		{
			// 移动到中间位置
			int i = 0;
			auto cell = midCell;

			do
			{
				cell = AStarClass::NextPathCell(cell, pDirs[i]);
			}
			while (++i < count);

			// 记下中间位置
			midCell = cell;
		}
	}

	const double threat = reinterpret_cast<double(__thiscall*)(const FootClass*)>(0x4DC760)(pFoot); // GetThreatAvoidanceCoefficient
	const auto pOwner = pFoot->Owner;

	// 路径调整
	if (offset > 0)
	{
		const auto& dirOffset = Unsorted::AdjacentCell[avgDir & 7];

		// 剩余需要处理的偏移量
		int remainingOffset = 2 * offset;

		do
		{
			int currentLevel = pLevels[steps + offset - remainingOffset];

			// 尝试移动到的单元格
			auto nextCell = midCell + dirOffset;
			auto pCell = MapClass::Instance.GetCellAt(nextCell);

			int i = remainingOffset;
			bool blocked = false;

			do
			{
				if (i <= 0)
				{
					// 处理完所有偏移量，用平均方向填充整个路径段
					const int size = 2 * offset;
					const int count = steps - offset;

					if (size > 0)
						std::fill_n(pDirs + count, size, avgDir);

					if (count > 0)
					{
						int j = 0;
						auto cell = *pCurrent;

						do
						{
							cell = AStarClass::NextPathCell(cell, pDirs[j]);
						}
						while (++j < count);

						*pCurrent = cell;
					}

					return count;
				}

				// 检查路径可行性
				blocked = (pFoot->IsCellOccupied(pCell, static_cast<FacingType>(avgDir), currentLevel, nullptr, true) != Move::OK)
					|| (pCell->Flags & CellFlags::Tube)
					|| (MapClass::Instance.GetThreatPosed(midCell, pOwner) * threat > 1.0);

				--i;

				// 继续沿平均方向移动
				nextCell += dirOffset;
				pCell = MapClass::Instance.GetCellAt(nextCell);

				// 更新高度
				const int level = pCell->Level;
				const int upLevel = level + 4;
				currentLevel = (currentLevel == upLevel && pCell->ContainsBridge()) ? upLevel : level;
			}
			while (!blocked);

			// 调整位置继续尝试
			midCell += Unsorted::AdjacentCell[firstDir & 7];
			remainingOffset -= 2;
		}
		while (--offset > 0);
	}

	if (steps > 0)
	{
		// 如果没有进行优化调整，则按原路径移动
		int i = 0;
		auto cell = *pCurrent;

		do
		{
			cell = AStarClass::NextPathCell(cell, pDirs[i]);
		}
		while (++i < steps);

		*pCurrent = cell;
	}

	return steps;
}

#pragma endregion

#pragma region OptimizeFinalPath

void AStarClass::OptimizeFinalPath(
	AStarClass::PathFinderData* const pPath,
	const FootClass* const pFoot
) const
{
	int* const pDirs = pPath->Directions;
	const int pathLength = pPath->PathLength - 1;

	if (pathLength > 0)
	{
		auto MaxCellAxisDeviation = [](const CellStruct cell) { return Math::max(std::abs(cell.X), std::abs(cell.Y)); };
		const int* const pLevels = pPath->Levels;
		const int* pCurDir = pDirs;

		// 状态初始化
		auto currentPosition = pPath->StartCell;
		auto lastValidPosition = CellStruct::Empty;
		auto totalMovementOffset = CellStruct::Empty;
		auto segmentMovementOffset = CellStruct::Empty;
		auto maxMovementDeviation = Point2D::Empty;
		int maxSegmentDiagonal = 0;
		int processedStepCount = 0;
		int lastOptimizedIndex = 0;
		int lastValidOptimizedIndex = 0;

		// 遍历整个路径
		do
		{
			// 最多优化20步
			if (processedStepCount >= 20)
				break;

			const int currentDirection = *pCurDir;

			// 隧道
			if (currentDirection == 8)
			{
				// 移动到下一单元格
				currentPosition += Unsorted::AdjacentCell[0];
				++processedStepCount;
				++pCurDir;

				// 重置所有跟踪变量并继续
				lastOptimizedIndex = processedStepCount;
				lastValidOptimizedIndex = processedStepCount;
				maxMovementDeviation = Point2D::Empty;
				maxSegmentDiagonal = 0;
				segmentMovementOffset = CellStruct::Empty;
				totalMovementOffset = CellStruct::Empty;
				lastValidPosition = CellStruct::Empty;
				continue;
			}

			if (currentDirection == -2)
			{
				// 跳过无效方向标记并继续
				++processedStepCount;
				++pCurDir;
				continue;
			}

			const auto& dirOffset = Unsorted::AdjacentCell[currentDirection & 7];
			const auto newSegmentMovementOffset = segmentMovementOffset + dirOffset;
			const auto newTotalMovementOffset = totalMovementOffset + dirOffset;
			const auto newMovementDeviation = Point2D { std::abs(newTotalMovementOffset.X), std::abs(newTotalMovementOffset.Y) };

			if (newMovementDeviation.X < maxMovementDeviation.X || newMovementDeviation.Y < maxMovementDeviation.Y)
			{
				// 发现某个方向的偏移量变小，回退状态
				if (lastValidPosition != CellStruct::Empty)
				{
					// 回退到有记录的状态
					lastOptimizedIndex = lastValidOptimizedIndex;
					segmentMovementOffset = lastValidPosition - currentPosition;
					maxSegmentDiagonal = MaxCellAxisDeviation(segmentMovementOffset);
					maxMovementDeviation = Point2D::Empty;
					totalMovementOffset = CellStruct::Empty;
					lastValidPosition = currentPosition;
					lastValidOptimizedIndex = processedStepCount;
				}
				else
				{
					// 无可回退状态，重置跟踪变量，设置当前位置为优化位置
					maxMovementDeviation = Point2D::Empty;
					totalMovementOffset = CellStruct::Empty;
					lastValidPosition = currentPosition;
					lastValidOptimizedIndex = processedStepCount;
				}
			}
			else
			{
				// 偏移量未变小，更新最大偏移量
				maxMovementDeviation = newMovementDeviation;
				totalMovementOffset = newTotalMovementOffset;

				const int newDiagonalOffset = MaxCellAxisDeviation(newSegmentMovementOffset);
				currentPosition += Unsorted::AdjacentCell[currentDirection & 7];

				if (maxSegmentDiagonal >= newDiagonalOffset)
				{
					// 对角线偏移量未增加，可以优化这段路径为直线
					int step = 0;
					auto cell = currentPosition;
					AStarClass::Instance.GetFinalStepCell(pDirs, processedStepCount, lastValidOptimizedIndex, &step, &cell);

					const auto vecCell = currentPosition - cell;
					const int plotLength = processedStepCount - step + 1;
					AStarClass::PlotStraightPath(&pDirs[step], plotLength, &cell, &vecCell, pFoot, pLevels[step], false);
				}
				else
				{
					// 对角线偏移量增加，更新最大对角线偏移量
					maxSegmentDiagonal = newDiagonalOffset;
				}

				// 记录并继续
				++processedStepCount;
				++pCurDir;
				segmentMovementOffset = newSegmentMovementOffset;
			}
		}
		while (processedStepCount < pathLength);

		// 之前存在可优化路径，最后检查剩余路径是否可以优化
		if (lastValidPosition != CellStruct::Empty)
		{
			const int finalDiagonalOffset = MaxCellAxisDeviation(currentPosition - lastValidPosition);
			const int endStep = processedStepCount - 1;

			if (endStep - lastValidOptimizedIndex > finalDiagonalOffset)
			{
				// 最后一段路没有尽量沿着斜线走，可以尝试优化
				int step = 0;
				auto cell = currentPosition;
				AStarClass::Instance.GetFinalStepCell(pDirs, endStep, lastValidOptimizedIndex, &step, &cell);

				const auto vecCell = currentPosition - cell;
				const int plotLength = endStep - step + 1;
				AStarClass::PlotStraightPath(&pDirs[step], plotLength, &cell, &vecCell, pFoot, pLevels[step], true);
			}
		}
	}

	// 压缩路径，移除无效方向标记
	int dir = *pDirs;
	int validStepCount = 0;

	if (dir != -1)
	{
		const int* pSourceDir = pDirs;
		int* pDestDirs = pDirs;
		int steps = 0;

		do
		{
			if (steps >= pathLength)
				break;

			// 保留非标记方向
			if (dir != -2)
			{
				++validStepCount;
				*pDestDirs++ = *pSourceDir;
			}

			dir = *(++pSourceDir);
			++steps;
		}
		while (dir != -1);
	}

	// 填充剩余路径
	int steps = validStepCount;
	const int totalLength = pPath->PathLength + 1;

	if (validStepCount < totalLength)
	{
		int* pDestDirs = &pDirs[validStepCount];

		do
		{
			*pDestDirs++ = -1;
			++steps;
		}
		while (steps < totalLength);
	}

	// 记录最终长度
	pPath->PathLength = validStepCount + 1;
}

#pragma endregion

#pragma region GetFinalStepCell

void AStarClass::GetFinalStepCell(
	const int* const pDirs,
	const int segmentEndIdx,
	const int segmentStartIdx,
	int* const pOutIdx,
	CellStruct* const pAdjacent
) const
{
	auto finalCell = *pAdjacent;
	int currentIndex = segmentStartIdx;

	if (segmentEndIdx >= segmentStartIdx)
	{
		auto accumulated = CellStruct::Empty;
		bool foundBetterPath = false;
		int maxDeviation = 0;

		// 反向遍历路径
		for (int searchIndex = segmentEndIdx; searchIndex >= segmentStartIdx; --searchIndex)
		{
			const int curDir = pDirs[searchIndex];

			// 跳过无效方向
			if (curDir != -2)
			{
				const int oppDir = (curDir - 4) & 7;
				const auto& dirOffset = Unsorted::AdjacentCell[oppDir];

				// 计算新的累积偏移量和单元格位置以及当前路径点的最大偏差
				const auto newAccumulated = accumulated + dirOffset;
				const auto newCell = finalCell + dirOffset;
				const int currentDeviation = Math::max(std::abs(newAccumulated.X), std::abs(newAccumulated.Y));

				if (currentDeviation <= maxDeviation)
				{
					// 当前偏差小于等于最大偏差，标记找到更好路径
					foundBetterPath = true;
				}
				else if (!foundBetterPath)
				{
					// 尚未找到更好路径且当前偏差更大，更新最大偏差
					maxDeviation = currentDeviation;
				}
				else
				{
					// 找到更好的路径，计算最终方向
					*pOutIdx = searchIndex + 1;

					// 因为有特殊值存在，所以保留两次反向的逻辑
					*pAdjacent = newCell + Unsorted::AdjacentCell[(oppDir - 4) & 7];
					return;
				}

				// 更新累积偏移量和当前状态
				accumulated = newAccumulated;
				currentIndex = segmentStartIdx;
				finalCell = newCell;
			}
		}
	}

	// 没有找到更好的路径，返回默认值
	*pOutIdx = currentIndex;
	*pAdjacent = finalCell;
}

#pragma endregion

#pragma region PlotStraightPath

bool AStarClass::PlotStraightPath(
	int* const pDirs,
	const int maxLength,
	const CellStruct* const pCurrent,
	const CellStruct* const pVector,
	const FootClass* const pFoot,
	const int curLevel,
	const bool allowThreats
) const
{
	// 确定主要和次要移动方向
	const int vecX = pVector->X;
	const int vecY = pVector->Y;
	const int sumXY = vecY + vecX;
	int primaryDir = (vecX >= 0) ? (vecY >= 0 ? 3 : 1) : (vecY >= 0 ? 5 : 7);
	int secondaryDir = (vecX - vecY <= 0) ? (sumXY <= 0 ? 6 : 4) : (sumXY <= 0 ? 0 : 2);

	// 计算各方向步数
	const int absX = std::abs(vecX);
	const int absY = std::abs(vecY);
	int minSteps = Math::min(absX, absY);
	int diagSteps = Math::max(absX, absY) - minSteps;

	const double threat = reinterpret_cast<double(__thiscall*)(const FootClass*)>(0x4DC760)(pFoot); // GetThreatAvoidanceCoefficient
	const auto pOwner = pFoot->Owner;

	int phase = 0;
	bool blocked = false;

	// 尝试平行四边形的两侧的边
	while (true)
	{
		int firstSteps = minSteps;
		int secondSteps = diagSteps;
		int threatCount = 0;
		auto currentPosition = *pCurrent;

		// 交换方向顺序
		if (phase > 0)
		{
			std::swap(primaryDir, secondaryDir);
			std::swap(minSteps, diagSteps);
		}

		if (minSteps)
		{
			int currentLevel = curLevel;

			// 沿主要方向移动
			if (minSteps > 0)
			{
				do
				{
					currentPosition += Unsorted::AdjacentCell[primaryDir & 7];
					const auto pCell = MapClass::Instance.GetCellAt(currentPosition);

					if (threat > 0.00001 && MapClass::Instance.GetThreatPosed(currentPosition, pOwner) * threat >= 0.01)
						++threatCount;

					// 检查路径可行性
					blocked = (pFoot->IsCellOccupied(pCell, static_cast<FacingType>(primaryDir), currentLevel, nullptr, true) != Move::OK)
						|| (pCell->Flags & CellFlags::Tube)
						|| (threatCount > 3)
						|| (!allowThreats && threatCount > 0);

					--firstSteps;

					// 更新高度
					const int level = pCell->Level;
					const int upLevel = level + 4;
					currentLevel = (currentLevel == upLevel && pCell->ContainsBridge()) ? upLevel : level;
				}
				while (firstSteps > 0 && !blocked);
			}

			// 沿次要方向移动
			if (diagSteps > 0 && !blocked)
			{
				do
				{
					currentPosition += Unsorted::AdjacentCell[secondaryDir & 7];
					const auto pCell = MapClass::Instance.GetCellAt(currentPosition);

					if (threat > 0.00001 && MapClass::Instance.GetThreatPosed(currentPosition, pOwner) * threat >= 0.01)
						++threatCount;

					// 检查路径可行性
					blocked = (pFoot->IsCellOccupied(pCell, static_cast<FacingType>(secondaryDir), currentLevel, nullptr, true) != Move::OK)
						|| (pCell->Flags & CellFlags::Tube)
						|| (threatCount > 3)
						|| (!allowThreats && threatCount > 0);

					--secondSteps;

					// 更新高度
					const int level = pCell->Level;
					const int upLevel = level + 4;
					currentLevel = (currentLevel == upLevel && pCell->ContainsBridge()) ? upLevel : level;
				}
				while (secondSteps > 0 && !blocked);
			}

			// 尝试成功
			if (!blocked)
				break;
		}

		// 两边均失败
		if (++phase >= 2)
			return false;
	}

	// 确认成功后写入路径
	if (minSteps > 0)
		std::fill_n(pDirs, minSteps, primaryDir);

	if (diagSteps > 0)
		std::fill_n(pDirs + minSteps, diagSteps, secondaryDir);

	// 标记无效路径
	const int totalSteps = minSteps + diagSteps;
	const int remainingSteps = maxLength - totalSteps;

	if (remainingSteps > 0)
		std::fill_n(pDirs + totalSteps, remainingSteps, -2);

	return true;
}

#pragma endregion

#pragma region Hooks

// 在重写的函数中，检查分层寻路时，会和原版结果不同，导致单位卡住，暂时搞不懂，先放着
// DEFINE_FUNCTION_JUMP(CALL, 0x42CC02, AStarClass::FindRegularPath);
DEFINE_FUNCTION_JUMP(CALL, 0x429F8A, AStarClass::CalculateMoveCost);
DEFINE_FUNCTION_JUMP(CALL, 0x42A415, AStarClass::ProcessFinalPath);
DEFINE_FUNCTION_JUMP(CALL, 0x42A41E, AStarClass::OptimizeFinalPath);

#pragma endregion

#endif
