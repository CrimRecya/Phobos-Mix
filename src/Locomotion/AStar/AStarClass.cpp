#include "AStarClass.h"

#ifdef ENABLE_ASTAR_REIMPL

#include <Ext/Techno/Body.h>

#pragma region AllUncompleted

#ifdef DISABLED_ASTAR_REIMPL

AStarClass::PathType* AStarClassFake::DoPathfinding_AsVanilla
(
	CellStruct* pSource,
	CellStruct* pDestination,
	FootClass* pFoot,
	int* a5,
	int a6,
	MovementZone movementZone,
	int mode
)
{
  DynamicVectorClass_unsigned_int *SubzonesIndexes; // esi
  int v10; // edi
  CellClass *v12; // ebx
  MovementZone v13; // eax
  CellClass *v14; // eax
  int v15; // eax
  CellFlags v16; // ecx
  ILocomotion *m_pInterface; // eax
  int v18; // ebx
  HRESULT v19; // eax
  LocomotionClass *v20; // ebx
  bool IsCellWithinUsableArea; // al
  AStarClass_PathType *result; // eax
  int v23; // eax
  signed int v24; // ecx
  __int64 v25; // rax
  int v26; // eax
  bool Finding; // dl
  CellStruct a3; // [esp+18h] [ebp-20h] BYREF
  CellStruct a2; // [esp+1Ch] [ebp-1Ch] BYREF
  CellClass *CellAt; // [esp+20h] [ebp-18h]
  AStarClass_PathType *v31; // [esp+20h] [ebp-18h] SPLIT
  int MovementZoneType; // [esp+24h] [ebp-14h]
  int v33[4]; // [esp+28h] [ebp-10h] BYREF
  int a4a; // [esp+44h] [ebp+Ch]
  int a4b; // [esp+44h] [ebp+Ch]
  MovementZone v36; // [esp+54h] [ebp+1Ch] SPLIT
  bool v37; // [esp+54h] [ebp+1Ch] SPLIT BYREF
  void *v38; // [esp+54h] [ebp+1Ch] SPLIT BYREF

  this->Finding = 1;
  AStarClass::CleanUp(this);
  SubzonesIndexes = this->SubzonesIndexes;
  v10 = 3;
  do
  {
    SubzonesIndexes->Clear(SubzonesIndexes);
    ++SubzonesIndexes;
    --v10;
  }
  while ( v10 );
  this->FindMode = mode;
  CellAt = MapClass::GetCellAt(&MouseClass::Instance, pSource);
  v12 = MapClass::GetCellAt(&MouseClass::Instance, pDestination);
  if ( movementZone == MovementZone_None )
    v13 = pFoot->GetTechnoType(pFoot)->MovementZone;
  else
    v13 = movementZone;
  MovementZoneType = MapClass::GetMovementZoneType(&MouseClass::Instance, pSource, v13, pFoot->OnBridge);
  if ( movementZone == MovementZone_None )
    v36 = pFoot->GetTechnoType(pFoot)->MovementZone;
  else
    v36 = movementZone;
  v14 = MapClass::GetCellAt(&MouseClass::Instance, pDestination);
  v15 = MapClass::GetMovementZoneType(&MouseClass::Instance, pDestination, v36, v14->Flags & CellFlags_BridgeHead);
  a4a = v15;
  LOBYTE(v15) = pFoot->OnBridge;
  a2 = *MapClass::SnapToBridgeZone(&MouseClass::Instance, (CellStruct *)&v37, CellAt, v15);
  v16 = v12->Flags >> 8;
  LOBYTE(v16) = v16 & 1;
  a3 = *MapClass::SnapToBridgeZone(&MouseClass::Instance, (CellStruct *)&v37, v12, v16);
  if ( movementZone == MovementZone_None )
    movementZone = pFoot->GetTechnoType(pFoot)->MovementZone;
  if ( pFoot->WhatAmI(pFoot) == AbstractType_Infantry && pFoot->Type->JumpJet )
  {
    m_pInterface = pFoot->Locomotor.m_pInterface;
    v18 = 0;
    movementZone = MovementZone_Infantry;
    if ( !m_pInterface )
      goto LABEL_18;
    v19 = m_pInterface->QueryInterface(m_pInterface, (_GUID *)&unk_818858, &v38);
    LOBYTE(v18) = v19 < 0;
    v20 = (LocomotionClass *)((unsigned int)v38 & (v18 - 1));
    if ( v19 < 0 && v19 != -2147467262 )
      RaiseError(v19);
    if ( !v20 )
LABEL_18:
      RaiseError(-2147467261);
    v20->GetClassID(v20, (_GUID *)v33);
    v20->Release(v20);
  }
  if ( pFoot->GetTechnoType(pFoot)->IsTrain
    || !pFoot->IsInPlayfield
    || pFoot->CanGetOutOfMap(pFoot)
    || !MapClass::IsCellWithinUsableArea(&MouseClass::Instance, &a2, 1)
    || (IsCellWithinUsableArea = MapClass::IsCellWithinUsableArea(&MouseClass::Instance, &a3, 1),
        v37 = 1,
        !IsCellWithinUsableArea) )
  {
    v37 = 0;
  }
  if ( MovementZoneType == a4a )
  {
    if ( v37 && !AStarClass::FindHierarchical(this, &a2, &a3, movementZone, pFoot) )
    {
      Debug::Log("Hierarchical findpath failure: (%d,%d) to (%d, %d)\n", a2.X, a2.Y, a3.X, a3.Y);
      v37 = 0;
    }
  }
  else if ( v37 )
  {
    return 0;
  }
  a4b = 0;
  v23 = -(a6 != -1);
  LOBYTE(v23) = v23 & 0xFC;
  MovementZoneType = v23 + 5;
  while ( 1 )
  {
    if ( !v37 && (a2.X != a3.X || a2.Y != a3.Y) )
      Debug::Log("Warning.  A* without HS: (%d,%d) to (%d, %d)\n", a2.X, a2.Y, a3.X, a3.Y);
    result = AStarClass::FindPathRegular(this, pSource, pDestination, pFoot, a5, a6, v37);
    v31 = result;
    if ( result || !v37 )
      break;
    v24 = abs32(pSource->Y - pDestination->Y);
    v25 = pSource->X - pDestination->X;
    v26 = (HIDWORD(v25) ^ v25) - HIDWORD(v25);
    if ( v26 <= v24 )
      v26 = v24;
    if ( v26 > 1 )
      Debug::Log("Regular findpath failure: (%d,%d) to (%d, %d)\n", a2.X, a2.Y, a3.X, a3.Y);
    ++a4b;
    AStarClass::RecordCellIndex(this, pFoot);
    AStarClass::CleanUp(this);
    Finding = this->Finding;
    v37 = Finding;
    if ( a4b >= MovementZoneType || Finding && !AStarClass::FindHierarchical(this, &a2, &a3, movementZone, pFoot) )
      return v31;
  }
  return result;
}

int AStarClassFake::AttemptPath_AsVanilla
(
	CellStruct* pSource,
	CellStruct* pDestination,
	FootClass* pFoot,
	bool checkSourceBridge,
	bool checkDestinationBridge,
	MovementZone movementZone
)
{
  DynamicVectorClass_unsigned_int *SubzonesIndexes; // esi
  int v9; // edi
  CellClass *CellAt; // eax
  CellStruct *v11; // esi
  CellClass *v12; // edi
  CellClass *v13; // ebp
  CellStruct v14; // ecx
  MovementZone v15; // eax
  FootClass *pFoot_1; // edi
  CellStruct *v17; // edi
  MovementZone v18; // ecx
  __int64 v19; // rax
  int v20; // ebp
  int ZoneConnectionsIndex; // edi
  CellStruct v22; // ecx
  __int16 X; // ax
  __int16 v24; // dx
  signed int v25; // edi
  __int64 v26; // rax
  signed int v27; // eax
  CellStruct v28; // ecx
  __int16 v29; // ax
  __int16 Y; // dx
  signed int v31; // esi
  __int64 v32; // rax
  signed int v33; // eax
  int result; // eax
  CellStruct a3a; // [esp+10h] [ebp-8h] BYREF
  CellStruct Sourcea; // [esp+14h] [ebp-4h] BYREF
  CellStruct v37; // [esp+1Ch] [ebp+4h] SPLIT BYREF
  CellStruct v38; // [esp+20h] [ebp+8h] SPLIT BYREF
  int v39; // [esp+20h] [ebp+8h] SPLIT
  CellStruct v40; // [esp+2Ch] [ebp+14h] SPLIT BYREF
  MovementZone v41; // [esp+30h] [ebp+18h] SPLIT

  this->Finding = 1;
  AStarClass::CleanUp(this);
  SubzonesIndexes = this->SubzonesIndexes;
  v9 = 3;
  do
  {
    SubzonesIndexes->Clear(SubzonesIndexes);
    ++SubzonesIndexes;
    --v9;
  }
  while ( v9 );
  CellAt = MapClass::GetCellAt(&MouseClass::Instance, pSource);
  v11 = pDestination;
  v12 = CellAt;
  v13 = MapClass::GetCellAt(&MouseClass::Instance, pDestination);
  Sourcea = *MapClass::SnapToBridgeZone(&MouseClass::Instance, &v38, v12, checkSourceBridge);
  v14 = *MapClass::SnapToBridgeZone(&MouseClass::Instance, &v38, v13, checkDestinationBridge);
  v15 = movementZone;
  pFoot_1 = pFoot;
  a3a = v14;
  if ( movementZone == MovementZone_None )
  {
    if ( pFoot )
      v15 = pFoot->GetTechnoType(pFoot)->MovementZone;
    else
      v15 = MovementZone_Normal;
  }
  if ( AStarClass::FindHierarchical(this, &Sourcea, &a3a, v15, pFoot_1) )
  {
    v17 = pSource;
    v18 = abs32(pSource->Y - v11->Y);
    v19 = pSource->X - v11->X;
    v41 = (HIDWORD(v19) ^ v19) - HIDWORD(v19);
    if ( v41 <= v18 )
      v41 = v18;
    v20 = this->PassabilityCounts[0];
    v39 = 2 * v20 - 2;
    if ( !checkDestinationBridge )
      goto LABEL_27;
    if ( checkSourceBridge )
    {
      ZoneConnectionsIndex = MapClass::GetZoneConnectionsIndex(&MouseClass::Instance, v11, 3, 0);
      if ( ZoneConnectionsIndex == MapClass::GetZoneConnectionsIndex(&MouseClass::Instance, pSource, 3, 0)
        && ZoneConnectionsIndex != -1 )
      {
        return v41;
      }
      v17 = pSource;
    }
    if ( v20 < 4 )
      goto LABEL_18;
    v22 = *MapClass::FindBridgeAdjacentPathableCell(
             &MouseClass::Instance,
             &v40,
             v11,
             0,
             *((unsigned __int16 *)&this->SubzonesIndexes[2].CapacityIncrement + v20));
    X = AStarClass::CellStruct::Empty.X;
    v40 = v22;
    if ( v22.X != AStarClass::CellStruct::Empty.X )
    {
LABEL_24:
      v25 = abs32(v11->Y - v40.Y);
      v26 = v11->X - v22.X;
      v27 = (HIDWORD(v26) ^ v26) - HIDWORD(v26);
      if ( v27 <= v25 )
        v27 = v25;
      v17 = pSource;
      v39 += v27;
LABEL_27:
      if ( !checkSourceBridge )
        goto LABEL_40;
      if ( v20 < 4 )
        goto LABEL_31;
      v28 = *MapClass::FindBridgeAdjacentPathableCell(
               &MouseClass::Instance,
               &v37,
               v17,
               0,
               (unsigned __int16)this->PassabilityIndexes[0].Indexes[1]);
      v29 = AStarClass::CellStruct::Empty.X;
      v37 = v28;
      if ( v28.X != AStarClass::CellStruct::Empty.X )
        goto LABEL_37;
      Y = AStarClass::CellStruct::Empty.Y;
      if ( v37.Y == AStarClass::CellStruct::Empty.Y )
      {
LABEL_31:
        v28 = *MapClass::FindBridgeAdjacentPathableCell(
                 &MouseClass::Instance,
                 &v37,
                 v17,
                 0,
                 (unsigned __int16)this->PassabilityIndexes[0].Indexes[0]);
        v29 = AStarClass::CellStruct::Empty.X;
        Y = AStarClass::CellStruct::Empty.Y;
        v37 = v28;
      }
      if ( v28.X == v29 )
      {
        if ( v37.Y == Y )
        {
          v28.X = Sourcea.X;
          v37 = Sourcea;
        }
        if ( v28.X == v29 && v37.Y == Y )
        {
LABEL_40:
          result = v39;
          if ( v39 > v41 )
            return result;
          return v41;
        }
      }
LABEL_37:
      v31 = abs32(v17->Y - v37.Y);
      v32 = v17->X - v28.X;
      v33 = (HIDWORD(v32) ^ v32) - HIDWORD(v32);
      if ( v33 <= v31 )
        v33 = v31;
      v39 += v33;
      goto LABEL_40;
    }
    v24 = AStarClass::CellStruct::Empty.Y;
    if ( v40.Y == AStarClass::CellStruct::Empty.Y )
    {
LABEL_18:
      v22 = *MapClass::FindBridgeAdjacentPathableCell(
               &MouseClass::Instance,
               &v40,
               v11,
               0,
               *((unsigned __int16 *)&this->SubzonesIndexes[2].CapacityIncrement + v20 + 1));
      X = AStarClass::CellStruct::Empty.X;
      v24 = AStarClass::CellStruct::Empty.Y;
      v40 = v22;
    }
    if ( v22.X == X )
    {
      if ( v40.Y == v24 )
      {
        v22.X = a3a.X;
        v40 = a3a;
      }
      if ( v22.X == X && v40.Y == v24 )
        goto LABEL_27;
    }
    goto LABEL_24;
  }
  return 0x7FFFFFFF;
}

AStarClass::PathType* AStarClassFake::FindPathRegular_AsVanilla
(
	CellStruct* pSource,
	CellStruct* pDestination,
	FootClass* pFoot,
	int* a5,
	int a6,
	bool a7
)
{
  CellStruct *v7; // edi
  CellStruct *v8; // ebp
  CellClass *v10; // ecx
  UnitClass *v11; // ebx
  int Level; // eax
  int v13; // eax
  int SourceCellLevel; // ecx
  __int64 v15; // rax
  TechnoTypeClass *v16; // eax
  int *v17; // edx
  int v18; // eax
  int v19; // ebp
  int *v20; // edi
  __int64 v21; // rax
  int v22; // eax
  CellClass *v23; // ecx
  int v24; // eax
  CellClass **Items; // eax
  int TubeIndex; // eax
  CellClass **v27; // eax
  CellClass *v28; // ebx
  int v29; // edi
  int X; // eax
  int v31; // edi
  int v32; // edx
  int v33; // eax
  int v34; // ecx
  float *FindCount; // eax
  int v36; // ebp
  double v37; // st7
  char v38; // fps^1
  double v39; // st6
  bool v40; // c0
  char v41; // c2
  bool v42; // c3
  double v43; // st7
  char v44; // fps^1
  double v45; // st6
  bool v46; // c0
  char v47; // c2
  bool v48; // c3
  FootClass *v49; // edi
  Move v50; // ebx
  CellClass **v51; // edi
  signed int v52; // ecx
  __int64 v53; // rax
  AStarClass_WorkPathQueueNode *Node; // ebx
  PriorityQueueClass_WorkPathData *WorkPathPriorityQueueDatas; // edi
  unsigned int v56; // ecx
  unsigned int v57; // edx
  AStarClass_WorkPathQueueArray *v58; // ebp
  AStarClass_WorkPathQueueNode *v59; // ebx
  AStarClass_WorkPathQueueNode *v60; // eax
  AStarClass_WorkPathQueueNode *v61; // ecx
  unsigned int v62; // edx
  AStarClass_WorkPathQueueArray *WorkPathDatas; // ebp
  AStarClass_WorkPathQueueNode *v64; // ebx
  AStarClass_WorkPathQueueNode *TheMaxWorkPathData; // ecx
  float *AltDistances; // ecx
  int CellStructCount; // eax
  __int64 v68; // rax
  AStarClass_WorkPathQueueNode *v69; // ecx
  PriorityQueueClass_WorkPathData *v70; // edx
  AStarClass_WorkPathQueueArray *v71; // edi
  AStarClass_WorkPathQueueNode *v72; // ebp
  int v73; // ebx
  int v74; // ecx
  AStarClass_WorkPathQueueArray *v75; // eax
  AStarClass_WorkPathQueueNode *v76; // edi
  int v77; // ebp
  bool v78; // cc
  PriorityQueueClass_WorkPathData *v79; // ecx
  AStarClass_WorkPathQueueArray *v80; // eax
  int v81; // ebx
  AStarClass_WorkPathQueueNode *v82; // edi
  int v83; // edi
  int v84; // edx
  AStarClass_WorkPathQueueArray *v85; // eax
  AStarClass_WorkPathQueueNode *v86; // edi
  int Count; // ebp
  AStarClass_PathType *v88; // eax
  FootClass *v89; // ebx
  AStarClass_PathType *v90; // edi
  FootClass *v92; // [esp-2h] [ebp-60h]
  char v93; // [esp+14h] [ebp-4Ah]
  char v94; // [esp+15h] [ebp-49h]
  AStarClass_WorkPathQueueNode *a1; // [esp+16h] [ebp-48h]
  int v96; // [esp+1Ah] [ebp-44h]
  AStarClass_WorkPathQueueNode *v97; // [esp+1Eh] [ebp-40h]
  CellClass **arg0; // [esp+22h] [ebp-3Ch]
  CellClass **v99; // [esp+26h] [ebp-38h]
  AStarClass_WorkPathQueueNode *v100; // [esp+26h] [ebp-38h]
  int v101; // [esp+2Ah] [ebp-34h]
  CellClass **a3; // [esp+2Eh] [ebp-30h]
  int a3a; // [esp+2Eh] [ebp-30h]
  CellClass *v104; // [esp+32h] [ebp-2Ch]
  CellStruct *p_MapCoords; // [esp+32h] [ebp-2Ch]
  float v106; // [esp+36h] [ebp-28h]
  int v107; // [esp+36h] [ebp-28h]
  CellClass **v108; // [esp+3Ah] [ebp-24h]
  int v109; // [esp+3Eh] [ebp-20h]
  int *v110; // [esp+42h] [ebp-1Ch]
  float v111; // [esp+46h] [ebp-18h]
  float AccumulatedDistance; // [esp+4Ah] [ebp-14h]
  int v113; // [esp+4Eh] [ebp-10h]
  AStarClass_WorkPathQueueNode *v114; // [esp+4Eh] [ebp-10h]
  AStarClass_WorkPathQueueNode *v115; // [esp+4Eh] [ebp-10h]
  CellClass *v116; // [esp+62h] [ebp+4h] SPLIT
  bool v117; // [esp+62h] [ebp+4h] SPLIT
  CellClass *v118; // [esp+62h] [ebp+4h] SPLIT BYREF
  CellStruct ExitCell; // [esp+62h] [ebp+4h] SPLIT
  int v121; // [esp+62h] [ebp+4h] MAPDST SPLIT

  v7 = pDestination;
  v8 = pSource;
  v101 = 0;
  v108 = &MouseClass::Instance.Cells.Items[512 * pDestination->Y] + pDestination->X;
  v10 = *v108;
  a3 = &MouseClass::Instance.Cells.Items[512 * pSource->Y] + pSource->X;
  v104 = *v108;
  v116 = *a3;
  if ( !v116 || !v10 )
    return 0;
  v11 = (UnitClass *)pFoot;
  if ( pFoot->WhatAmI(pFoot) == AbstractType_Aircraft || (v104->Flags & CellFlags_BridgeHead) == 0 )
    Level = v104->Level;
  else
    Level = v104->Level + 4;
  this->DestinationCellLevel = Level;
  if ( v11->WhatAmI(v11) == AbstractType_Aircraft || !v11->OnBridge )
    v13 = v116->Level;
  else
    v13 = v116->Level + 4;
  this->SourceCellLevel = v13;
  if ( v11->GetTechnoType(v11)->IsTrain && (v116->Flags & CellFlags_BridgeHead) != 0 )
  {
    SourceCellLevel = this->SourceCellLevel;
    v15 = v11->Location.Z / AStarClass::LevelHeight_104 - SourceCellLevel;
    if ( (int)((HIDWORD(v15) ^ v15) - HIDWORD(v15)) > 2 )
      this->SourceCellLevel = SourceCellLevel + 4;
  }
  v16 = v11->GetTechnoType(v11);
  v17 = this->BothWayPassabilityCounts[0];
  this->FinderSpeedType = v16->SpeedType;
  this->CellStructCount = 0;
  this->CellStructBuffer = *v8;
  v110 = v17;
  a1 = AStarClass::CreateNode(this, 0, a3, v7, 0.0);
  if ( v8->X == v7->X && v8->Y == v7->Y && this->SourceCellLevel == this->DestinationCellLevel )
    return 0;
  if ( this->FindMode )
    AStarClass::PostProcessCells(this, v11);
  v18 = v116->MapCoords.X + MapClass::MapSides * v116->MapCoords.Y;
  if ( this->SourceCellLevel <= v116->Level )
  {
    this->AltCounts[v18] = this->FindCount;
    this->AltDistances[v18] = 0.0;
  }
  else
  {
    this->Counts[v18] = this->FindCount;
    this->Distances[v18] = 0.0;
  }
  v93 = 0;
  if ( v11->GetTechnoType(v11)->IsTrain )
  {
    v93 = 1;
    v19 = 0;
    v20 = dword_7E3774;
    do
    {
      v21 = (int)(((((*(unsigned int *)FacingClass::Current(&v11->PrimaryFacing, (DirStruct *)&v118) >> 12) + 1) >> 1) & 7)
                - v19);
      v22 = (HIDWORD(v21) ^ v21) - HIDWORD(v21);
      if ( v22 > 2 && v22 < 6 && v20 != (int *)flt_7E3794 )
      {
        v23 = a3[*v20];
        v24 = v23->MapCoords.X + MapClass::MapSides * v23->MapCoords.Y;
        if ( this->SourceCellLevel <= v23->Level + 1 )
        {
          this->AltCounts[v24] = this->FindCount;
          this->AltDistances[v24] = 0.0;
        }
        else
        {
          this->Counts[v24] = this->FindCount;
          this->Distances[v24] = 0.0;
        }
      }
      ++v20;
      ++v19;
    }
    while ( (int)v20 <= (int)flt_7E3794 );
  }
  if ( v11->WhatAmI(v11) != AbstractType_Unit || (v94 = 1, !v11->Type->Passive) )
    v94 = 0;
  if ( a6 < 0 )
    a6 = 65527;
  if ( !a1 )
  {
LABEL_156:
    if ( this->FindMode )
      AStarClass::PostProcessCells(this, v11);
    return 0;
  }
  do
  {
    if ( v101 >= a6 )
      break;
    Items = a1->WorkPathHeapNodes->Items;
    arg0 = Items;
    if ( Items == v108 && a1->WorkPathHeapNodes->Level == this->DestinationCellLevel )
      break;
    v97 = 0;
    v96 = 0;
    v109 = (*Items)->MapCoords.X + MapClass::MapSides * (*Items)->MapCoords.Y;
    do
    {
      if ( v96 == 8 )
      {
        TubeIndex = (*arg0)->TubeIndex;
        if ( TubeIndex == -1 )
        {
          v27 = &dword_89C2E0;
        }
        else
        {
          ExitCell = TubeClass::Array.Items[TubeIndex]->ExitCell;
          v27 = &MouseClass::Instance.Cells.Items[512 * ExitCell.Y] + ExitCell.X;
        }
      }
      else
      {
        v27 = &arg0[dword_7E3774[v96]];
      }
      v28 = *v27;
      v99 = v27;
      if ( !*v27 )
        goto LABEL_107;
      p_MapCoords = &v28->MapCoords;
      if ( v96 == 8 )
      {
        v29 = MapClass::MapSides * v28->MapCoords.Y;
        X = v28->MapCoords.X;
      }
      else
      {
        v29 = dword_89A304[v96];
        X = v109;
      }
      v31 = X + v29;
      if ( (v28->Flags & CellFlags_BridgeHead) == 0
        || (v32 = v28->Level, v33 = this->SourceCellLevel, v117 = 0, (int)abs32(v33 - v32) <= 1) )
      {
        v117 = 1;
      }
      v34 = MouseClass::Instance.LevelAndPassabilityStruct2pointer_70[MapClass::GetCellPathIndex(
                                                                        &MouseClass::Instance,
                                                                        &v28->MapCoords)].Indexes[0];
      FindCount = (float *)this->FindCount;
      v113 = v34;
      if ( (float *)v110[v34] != FindCount )
      {
        if ( !v117 )
          goto LABEL_62;
        if ( !v28->BlockedNeighbours && a7 )
          goto LABEL_107;
      }
      if ( !v117 )
      {
LABEL_62:
        v36 = v31;
        a3a = v31;
        if ( (float *)this->Counts[v31] == FindCount )
        {
          FindCount = this->Distances;
          v43 = a1->MovementDistance + 1.009;
          v45 = FindCount[v31];
          v46 = v45 < v43;
          v47 = 0;
          v48 = v45 == v43;
          BYTE1(FindCount) = v44; // ?
          if ( v45 < v43 )
            goto LABEL_107;
        }
        goto LABEL_64;
      }
      v36 = v31;
      a3a = v31;
      if ( (float *)this->AltCounts[v31] == FindCount )
      {
        FindCount = this->AltDistances;
        v37 = a1->MovementDistance + 1.009;
        v39 = FindCount[v31];
        v40 = v39 < v37;
        v41 = 0;
        v42 = v39 == v37;
        BYTE1(FindCount) = v38; // ?
        if ( v39 < v37 )
          goto LABEL_107;
      }
LABEL_64:
      LOBYTE(FindCount) = this->IsAlt;
      v49 = pFoot;
      v50 = pFoot->IsCellOccupied(pFoot, v28, (FacingType)v96, this->SourceCellLevel, *arg0, (bool)FindCount);
      if ( v93 && v50 < Move_No )
        v50 = Move_OK;
      if ( v96 == 8 )
      {
        v52 = abs32((*arg0)->MapCoords.Y - p_MapCoords->Y);
        v53 = (*arg0)->MapCoords.X - p_MapCoords->X;
        v107 = (HIDWORD(v53) ^ v53) - HIDWORD(v53);
        if ( v107 <= v52 )
          v107 = v52;
        v51 = v99;
        v106 = (float)v107;
      }
      else
      {
        v92 = v49;
        v51 = v99;
        v106 = AStarClass::GetMovementCost(this, arg0, v99, !v117, v50, v92) * this->FinderCoefficient + flt_81872C[v96];
      }
      if ( v50 >= Move_No )
      {
        if ( v51 == v108 && !v94 )
        {
          v68 = this->SourceCellLevel - this->DestinationCellLevel;
          if ( (int)((HIDWORD(v68) ^ v68) - HIDWORD(v68)) <= 1 )
            goto LABEL_148;
        }
      }
      else
      {
        if ( v117 )
        {
          if ( this->AltCounts[v36] == this->FindCount )
            goto LABEL_107;
        }
        else if ( this->Counts[v36] == this->FindCount )
        {
          goto LABEL_107;
        }
        Node = AStarClass::CreateNode(this, a1, v51, pDestination, v106);
        v100 = Node;
        if ( v97 )
        {
          WorkPathPriorityQueueDatas = this->WorkPathPriorityQueueDatas;
          v56 = WorkPathPriorityQueueDatas->Count + 1;
          if ( WorkPathPriorityQueueDatas->Count == -1 )
          {
            AccumulatedDistance = Node->AccumulatedDistance;
            v62 = v56 >> 1;
            if ( v56 < WorkPathPriorityQueueDatas->Capacity )
            {
              for ( ; v56 > 1; v62 >>= 1 )
              {
                WorkPathDatas = WorkPathPriorityQueueDatas->WorkPathDatas;
                v64 = WorkPathDatas->WorkPathData[v62];
                if ( v64->AccumulatedDistance <= (double)AccumulatedDistance )
                  break;
                WorkPathDatas->WorkPathData[v56] = v64;
                v56 = v62;
              }
              WorkPathPriorityQueueDatas->WorkPathDatas->WorkPathData[v56] = v100;
              TheMaxWorkPathData = WorkPathPriorityQueueDatas->TheMaxWorkPathData;
              ++WorkPathPriorityQueueDatas->Count;
              if ( v100 > TheMaxWorkPathData )
                WorkPathPriorityQueueDatas->TheMaxWorkPathData = v100;
              if ( v100 < WorkPathPriorityQueueDatas->TheMinWorkPathData )
                WorkPathPriorityQueueDatas->TheMinWorkPathData = v100;
            }
          }
          else
          {
            v57 = v56 >> 1;
            v111 = v97->AccumulatedDistance;
            if ( v56 < WorkPathPriorityQueueDatas->Capacity )
            {
              if ( v56 <= 1 )
              {
                v60 = v97;
              }
              else
              {
                do
                {
                  v58 = WorkPathPriorityQueueDatas->WorkPathDatas;
                  v59 = v58->WorkPathData[v57];
                  if ( v59->AccumulatedDistance <= (double)v111 )
                    break;
                  v58->WorkPathData[v56] = v59;
                  v56 = v57;
                  v57 >>= 1;
                }
                while ( v56 > 1 );
                v60 = v97;
                Node = v100;
              }
              WorkPathPriorityQueueDatas->WorkPathDatas->WorkPathData[v56] = v60;
              v61 = WorkPathPriorityQueueDatas->TheMaxWorkPathData;
              ++WorkPathPriorityQueueDatas->Count;
              if ( v60 > v61 )
                WorkPathPriorityQueueDatas->TheMaxWorkPathData = v60;
              if ( v60 < WorkPathPriorityQueueDatas->TheMinWorkPathData )
                WorkPathPriorityQueueDatas->TheMinWorkPathData = v60;
            }
            v97 = Node;
          }
        }
        else
        {
          v97 = Node;
        }
        if ( v117 )
        {
          this->AltCounts[a3a] = this->FindCount;
          AltDistances = this->AltDistances;
        }
        else
        {
          this->Counts[a3a] = this->FindCount;
          AltDistances = this->Distances;
        }
        AltDistances[a3a] = v100->MovementDistance;
        CellStructCount = this->CellStructCount;
        if ( v113 == (unsigned __int16)this->PassabilityIndexes[0].Indexes[CellStructCount + 1] )
        {
          this->CellStructCount = CellStructCount + 1;
          this->CellStructBuffer = *p_MapCoords;
        }
      }
LABEL_107:
      ++v96;
    }
    while ( v96 <= 8 );
    v69 = v97;
    if ( !v97 )
    {
      v79 = this->WorkPathPriorityQueueDatas;
      if ( !v79->Count )
      {
        a1 = 0;
        goto LABEL_147;
      }
      v80 = v79->WorkPathDatas;
      v81 = 1;
      v82 = v80->WorkPathData[1];
      v80->WorkPathData[1] = v80->WorkPathData[v79->Count];
      v115 = v82;
      v79->WorkPathDatas->WorkPathData[v79->Count] = 0;
      v83 = v79->Count - 1;
      v79->Count = v83;
      if ( v83 < 2
        || (v84 = 2,
            v79->WorkPathDatas->WorkPathData[1]->AccumulatedDistance <= (double)v79->WorkPathDatas->WorkPathData[2]->AccumulatedDistance) )
      {
        v84 = 1;
      }
      if ( v83 < 3
        || v79->WorkPathDatas->WorkPathData[v84]->AccumulatedDistance <= (double)v79->WorkPathDatas->WorkPathData[3]->AccumulatedDistance )
      {
        if ( v84 == 1 )
        {
LABEL_143:
          v69 = v115;
LABEL_144:
          a1 = v69;
          goto LABEL_145;
        }
      }
      else
      {
        v84 = 3;
      }
      do
      {
        v85 = v79->WorkPathDatas;
        v86 = v85->WorkPathData[v81];
        v85->WorkPathData[v81] = v85->WorkPathData[v84];
        v81 = v84;
        v79->WorkPathDatas->WorkPathData[v84] = v86;
        Count = v79->Count;
        v78 = 2 * v84 <= v79->Count;
        v121 = 2 * v84 + 1;
        if ( v78
          && v79->WorkPathDatas->WorkPathData[v84]->AccumulatedDistance > (double)v79->WorkPathDatas->WorkPathData[2 * v84]->AccumulatedDistance )
        {
          v84 *= 2;
        }
        if ( v121 <= Count
          && v79->WorkPathDatas->WorkPathData[v84]->AccumulatedDistance > (double)v79->WorkPathDatas->WorkPathData[v121]->AccumulatedDistance )
        {
          v84 = v121;
        }
      }
      while ( v84 != v81 );
      goto LABEL_143;
    }
    v70 = this->WorkPathPriorityQueueDatas;
    if ( !v70->Count )
      goto LABEL_144;
    v71 = v70->WorkPathDatas;
    v72 = v71->WorkPathData[1];
    v114 = v72;
    if ( v72->AccumulatedDistance > (double)v97->AccumulatedDistance )
      goto LABEL_144;
    v71->WorkPathData[1] = v97;
    v73 = 1;
    if ( v70->Count < 2
      || (v74 = 2,
          v70->WorkPathDatas->WorkPathData[1]->AccumulatedDistance <= (double)v70->WorkPathDatas->WorkPathData[2]->AccumulatedDistance) )
    {
      v74 = 1;
    }
    if ( v70->Count < 3
      || v70->WorkPathDatas->WorkPathData[v74]->AccumulatedDistance <= (double)v70->WorkPathDatas->WorkPathData[3]->AccumulatedDistance )
    {
      if ( v74 != 1 )
        goto LABEL_118;
    }
    else
    {
      v74 = 3;
      do
      {
LABEL_118:
        v75 = v70->WorkPathDatas;
        v76 = v75->WorkPathData[v73];
        v75->WorkPathData[v73] = v75->WorkPathData[v74];
        v73 = v74;
        v70->WorkPathDatas->WorkPathData[v74] = v76;
        v77 = v70->Count;
        v78 = 2 * v74 <= v70->Count;
        v121 = 2 * v74 + 1;
        if ( v78
          && v70->WorkPathDatas->WorkPathData[v74]->AccumulatedDistance > (double)v70->WorkPathDatas->WorkPathData[2 * v74]->AccumulatedDistance )
        {
          v74 *= 2;
        }
        if ( v121 <= v77
          && v70->WorkPathDatas->WorkPathData[v74]->AccumulatedDistance > (double)v70->WorkPathDatas->WorkPathData[v121]->AccumulatedDistance )
        {
          v74 = v121;
        }
      }
      while ( v74 != v73 );
      v72 = v114;
    }
    a1 = v72;
LABEL_145:
    if ( a1 )
      this->SourceCellLevel = a1->WorkPathHeapNodes->Level;
LABEL_147:
    ++v101;
  }
  while ( a1 );
LABEL_148:
  if ( v101 == 10000 || !a1 || v101 == a6 || a1->AccumulatedCount < 2 )
  {
    v11 = (UnitClass *)pFoot;
    goto LABEL_156;
  }
  v88 = AStarClass::BuildFinalPath(this, a1, a5);
  v89 = pFoot;
  v90 = v88;
  AStarClass::ProcessFinalPath(this, v88, pFoot);
  AStarClass::OptimizeFinalPath(this, v90, v89);
  if ( this->FindMode )
    AStarClass::PostProcessCells(this, v89);
  return v90;
}

bool AStarClassFake::FindHierarchical_AsVanilla
(
	CellStruct* pSource,
	CellStruct* pDestination,
	MovementZone movementZone,
	FootClass* pFoot
)
{
  AStarClass *v5; // edi
  double ThreatAvoidanceCoefficient; // st7
  int v7; // esi
  PriorityQueueClass_HierarchicalData *HierarchicalPriorityQueueDatas; // eax
  int i; // ecx
  int sourcePassabilityIndex; // ebx
  int *v11; // eax
  AStarClass_HierarchicalQueueNode *HierarchicalData; // eax
  unsigned int v13; // ecx
  unsigned int v14; // edx
  PriorityQueueClass_HierarchicalData *v15; // ecx
  AStarClass_HierarchicalQueueNode *v16; // eax
  AStarClass_HierarchicalQueueArray *v17; // eax
  AStarClass_HierarchicalQueueNode *v18; // ebx
  int v19; // edx
  int FinderIndex; // ecx
  int v21; // eax
  SubzoneTrackingStruct *v22; // ecx
  SubzoneConnectionStruct *Items; // edx
  int Count; // ecx
  int Index; // ebx
  SubzoneTrackingStruct *v26; // edx
  int v27; // esi
  int CoefficientIndex; // edx
  int ThreatPosedEstimates; // eax
  int v30; // eax
  double v31; // st7
  int FindCount; // ecx
  unsigned __int16 v33; // cx
  unsigned __int16 v34; // ax
  int v35; // eax
  int v36; // ecx
  unsigned int *v37; // edx
  AStarClass_HierarchicalQueueBuffer *HierarchicalPriorityQueueBuffer; // eax
  AStarClass_HierarchicalQueueNode *v39; // esi
  PriorityQueueClass_HierarchicalData *v40; // edi
  unsigned int v41; // ecx
  unsigned int v42; // edx
  AStarClass_HierarchicalQueueNode *TheMaxHierarchicalData; // eax
  bool v44; // zf
  PriorityQueueClass_HierarchicalData *v45; // ecx
  AStarClass_HierarchicalQueueArray *v46; // eax
  int v47; // ebx
  AStarClass_HierarchicalQueueNode *v48; // esi
  int v49; // esi
  int v50; // edx
  AStarClass_HierarchicalQueueArray *v51; // eax
  AStarClass_HierarchicalQueueNode *v52; // esi
  AStarClass_HierarchicalQueueNode *v53; // ecx
  int v54; // eax
  int v55; // eax
  __int16 *v56; // edx
  char v58; // [esp+10h] [ebp-58h]
  bool Supplement; // [esp+11h] [ebp-57h]
  bool v60; // [esp+12h] [ebp-56h]
  bool v61; // [esp+13h] [ebp-55h]
  AStarClass_HierarchicalQueueNode *v62; // [esp+14h] [ebp-54h]
  int v63; // [esp+18h] [ebp-50h]
  float v64; // [esp+18h] [ebp-50h]
  PriorityQueueClass_HierarchicalData *v66; // [esp+20h] [ebp-48h]
  int v67; // [esp+20h] [ebp-48h]
  int *v68; // [esp+24h] [ebp-44h]
  float *v69; // [esp+28h] [ebp-40h]
  int v70; // [esp+2Ch] [ebp-3Ch]
  int v71; // [esp+30h] [ebp-38h]
  int v72; // [esp+34h] [ebp-34h]
  SubzoneConnectionStruct *v73; // [esp+34h] [ebp-34h]
  int v74; // [esp+34h] [ebp-34h]
  int v75; // [esp+38h] [ebp-30h]
  int destinationPassabilityIndex; // [esp+3Ch] [ebp-2Ch]
  HouseClass *a1; // [esp+40h] [ebp-28h]
  int *v78; // [esp+44h] [ebp-24h]
  int a3a; // [esp+48h] [ebp-20h]
  AStarClass_HierarchicalQueueNode *v80; // [esp+4Ch] [ebp-1Ch]
  int v81; // [esp+4Ch] [ebp-1Ch]
  AStarClass_HierarchicalQueueArray *HierarchicalDatas; // [esp+50h] [ebp-18h]
  int v83; // [esp+50h] [ebp-18h]
  int v84; // [esp+54h] [ebp-14h]
  AStarClass_HierarchicalQueueNode *v85; // [esp+54h] [ebp-14h]
  AStarClass_HierarchicalQueueNode *v86; // [esp+54h] [ebp-14h]
  AStarClass_HierarchicalQueueNode *v87; // [esp+54h] [ebp-14h]
  int *v88; // [esp+5Ch] [ebp-Ch]
  double ThreatAvoidanceCoefficient_1; // [esp+60h] [ebp-8h]

  v5 = this;
  if ( !pFoot )
  {
    ThreatAvoidanceCoefficient_1 = 0.0;
    a1 = 0;
LABEL_5:
    v58 = 0;
    goto LABEL_6;
  }
  ThreatAvoidanceCoefficient = FootClass::GetThreatAvoidanceCoefficient(pFoot);
  ThreatAvoidanceCoefficient_1 = ThreatAvoidanceCoefficient;
  a1 = pFoot->Owner;
  v58 = 1;
  if ( ThreatAvoidanceCoefficient <= 0.00001 )
    goto LABEL_5;
LABEL_6:
  v7 = 2;
  v75 = 2;
  while ( 2 )
  {
    HierarchicalPriorityQueueDatas = v5->HierarchicalPriorityQueueDatas;
    for ( i = 0;
          i <= HierarchicalPriorityQueueDatas->Count;
          HierarchicalPriorityQueueDatas->HierarchicalDatas->HierarchicalData[i - 1] = 0 )
    {
      ++i;
    }
    HierarchicalPriorityQueueDatas->Count = 0;
    sourcePassabilityIndex = MouseClass::Instance.LevelAndPassabilityStruct2pointer_70[MapClass::GetCellPathIndex(
                                                                                         &MouseClass::Instance,
                                                                                         pSource)].Indexes[v7];
    v72 = sourcePassabilityIndex;
    destinationPassabilityIndex = MouseClass::Instance.LevelAndPassabilityStruct2pointer_70[MapClass::GetCellPathIndex(
                                                                                              &MouseClass::Instance,
                                                                                              pDestination)].Indexes[v7];
    v60 = v7 == 2;
    if ( v7 == 2 )
      v78 = 0;
    else
      v78 = v5->BothWayPassabilityCounts[v7 + 1];
    v11 = v5->BothWayPassabilityCounts[v7];
    v68 = v5->OneWayPassabilityCounts[v7];
    v69 = v5->OneWayPassabilityCoefficients[v7];
    v11[sourcePassabilityIndex] = v5->FindCount;
    v88 = v11;
    v11[destinationPassabilityIndex] = v5->FindCount;
    if ( sourcePassabilityIndex == destinationPassabilityIndex )
    {
      if ( !v7 )
      {
        HierarchicalData = v5->HierarchicalPriorityQueueBuffer->HierarchicalData;
        HierarchicalData->Count = 0;
        HierarchicalData->FinderIndex = sourcePassabilityIndex;
      }
      v5->PassabilityIndexes[v7].Indexes[0] = sourcePassabilityIndex;
      v5->PassabilityCounts[v7] = 1;
      goto LABEL_85;
    }
    v80 = v5->HierarchicalPriorityQueueBuffer->HierarchicalData;
    v80->NodeIndex = -1;
    v80->FinderIndex = sourcePassabilityIndex;
    v80->Coefficient = 0.0;
    v80->Count = 0;
    v66 = v5->HierarchicalPriorityQueueDatas;
    v13 = v66->Count + 1;
    v14 = v13 >> 1;
    if ( v13 < v66->Capacity )
    {
      for ( ; v13 > 1; v14 >>= 1 )
      {
        HierarchicalDatas = v66->HierarchicalDatas;
        if ( HierarchicalDatas->HierarchicalData[v14]->Coefficient <= 0.0 )
          break;
        HierarchicalDatas->HierarchicalData[v13] = HierarchicalDatas->HierarchicalData[v14];
        sourcePassabilityIndex = v72;
        v13 = v14;
      }
      v66->HierarchicalDatas->HierarchicalData[v13] = v80;
      ++v66->Count;
      if ( v80 > v66->TheMaxHierarchicalData )
        v66->TheMaxHierarchicalData = v80;
      if ( v80 < v66->TheMinHierarchicalData )
        v66->TheMinHierarchicalData = v80;
    }
    v70 = 1;
    v68[sourcePassabilityIndex] = v5->FindCount;
    v69[sourcePassabilityIndex] = 0.0;
    v15 = v5->HierarchicalPriorityQueueDatas;
    if ( v15->Count )
    {
      v17 = v15->HierarchicalDatas;
      v18 = v17->HierarchicalData[1];
      v17->HierarchicalData[1] = v17->HierarchicalData[v15->Count];
      v15->HierarchicalDatas->HierarchicalData[v15->Count--] = 0;
      PriorityQueueClass::Pop(v15, 1); // HeapifyDown
      v16 = v18;
    }
    else
    {
      v16 = 0;
    }
    v62 = v16;
    v19 = 24 * v7;
    v67 = v7;
    v61 = v5->SubzonesIndexes[v7].Count == 0;
    if ( !v16 )
      return 0;
    while ( 1 )
    {
      FinderIndex = v16->FinderIndex;
      a3a = FinderIndex;
      if ( FinderIndex == destinationPassabilityIndex )
        break;
      v21 = FinderIndex;
      v22 = *(SubzoneTrackingStruct **)((char *)&MouseClass::Instance.SubzoneTracking1.Items + v19);
      Items = v22[v21].SubzoneConnections.Items;
      Count = v22[v21].SubzoneConnections.Count;
      if ( Count > 0 )
      {
        v73 = Items;
        v71 = v70;
        v83 = Count;
        do
        {
          HIWORD(v27) = 0;
          v63 = 0;
          Index = v73->Index;
          Supplement = v73->Supplement;
          v26 = *(SubzoneTrackingStruct **)((char *)&MouseClass::Instance.SubzoneTracking1.Items + v67 * 24);
          v81 = v73->Index;
          LOWORD(v27) = v26[v73->Index].Index;
          CoefficientIndex = v26[v73->Index].CoefficientIndex;
          v84 = CoefficientIndex;
          if ( v58 )
          {
            ThreatPosedEstimates = MapClass::GetThreatPosedEstimates(&MouseClass::Instance, a1, v75, a3a, Index);
            v30 = F2I64((double)ThreatPosedEstimates * ThreatAvoidanceCoefficient_1);
            CoefficientIndex = v84;
            v63 = v30;
          }
          if ( Supplement )
            v31 = 0.001;
          else
            v31 = 0.0;
          FindCount = v5->FindCount;
          v64 = flt_7E3794[CoefficientIndex] + v62->Coefficient + (double)v63 + v31;
          if ( (v68[Index] != FindCount || v69[Index] > (double)v64)
            && (v60 || v78[v27] == FindCount || CoefficientIndex == 1)
            && MapClass::MovementAdjustArray[8 * movementZone + CoefficientIndex] == 1 )
          {
            if ( v61 )
              goto LABEL_49;
            v33 = Index;
            v34 = a3a;
            if ( (unsigned __int16)Index < (unsigned __int16)a3a )
            {
              v34 = Index;
              v33 = a3a;
            }
            v35 = v33 | (v34 << 16);
            v36 = v5->SubzonesIndexes[v67].Count - 1;
            if ( v36 < 0 )
            {
LABEL_49:
              HierarchicalPriorityQueueBuffer = this->HierarchicalPriorityQueueBuffer;
              HierarchicalPriorityQueueBuffer->HierarchicalData[v71].FinderIndex = Index;
              v39 = &HierarchicalPriorityQueueBuffer->HierarchicalData[v71];
              v39->NodeIndex = ((char *)v62 - (char *)HierarchicalPriorityQueueBuffer) >> 4;
              v39->Coefficient = v64;
              v39->Count = v62->Count + 1;
              v40 = this->HierarchicalPriorityQueueDatas;
              v41 = v40->Count + 1;
              v42 = v41 >> 1;
              if ( v41 < v40->Capacity )
              {
                for ( ; v41 > 1; v42 >>= 1 )
                {
                  v85 = v40->HierarchicalDatas->HierarchicalData[v42];
                  if ( v85->Coefficient <= (double)v64 )
                    break;
                  v40->HierarchicalDatas->HierarchicalData[v41] = v85;
                  Index = v81;
                  v41 = v42;
                }
                v40->HierarchicalDatas->HierarchicalData[v41] = v39;
                TheMaxHierarchicalData = v40->TheMaxHierarchicalData;
                ++v40->Count;
                if ( v39 > TheMaxHierarchicalData )
                  v40->TheMaxHierarchicalData = v39;
                if ( v39 < v40->TheMinHierarchicalData )
                  v40->TheMinHierarchicalData = v39;
              }
              v5 = this;
              v68[Index] = this->FindCount;
              v69[Index] = v64;
              ++v70;
              ++v71;
            }
            else
            {
              v37 = &v5->SubzonesIndexes[v67].Items[v36];
              while ( *v37 != v35 )
              {
                --v36;
                --v37;
                if ( v36 < 0 )
                  goto LABEL_49;
              }
            }
          }
          v44 = v83 == 1;
          ++v73;
          --v83;
        }
        while ( !v44 );
      }
      v45 = v5->HierarchicalPriorityQueueDatas;
      if ( !v45->Count )
        return 0;
      v46 = v45->HierarchicalDatas;
      v47 = 1;
      v48 = v46->HierarchicalData[1];
      v46->HierarchicalData[1] = v46->HierarchicalData[v45->Count];
      v86 = v48;
      v45->HierarchicalDatas->HierarchicalData[v45->Count] = 0;
      v49 = v45->Count - 1;
      v45->Count = v49;
      if ( v49 < 2
        || (v50 = 2,
            v45->HierarchicalDatas->HierarchicalData[1]->Coefficient <= (double)v45->HierarchicalDatas->HierarchicalData[2]->Coefficient) )
      {
        v50 = 1;
      }
      if ( v49 < 3
        || v45->HierarchicalDatas->HierarchicalData[v50]->Coefficient <= (double)v45->HierarchicalDatas->HierarchicalData[3]->Coefficient )
      {
        if ( v50 == 1 )
          goto LABEL_75;
      }
      else
      {
        v50 = 3;
      }
      do
      {
        v51 = v45->HierarchicalDatas;
        v52 = v51->HierarchicalData[v47];
        v51->HierarchicalData[v47] = v51->HierarchicalData[v50];
        v47 = v50;
        v45->HierarchicalDatas->HierarchicalData[v50] = v52;
        v74 = 2 * v50 + 1;
        if ( 2 * v50 <= v45->Count
          && v45->HierarchicalDatas->HierarchicalData[v50]->Coefficient > (double)v45->HierarchicalDatas->HierarchicalData[2 * v50]->Coefficient )
        {
          v50 *= 2;
        }
        if ( v74 <= v45->Count
          && v45->HierarchicalDatas->HierarchicalData[v50]->Coefficient > (double)v45->HierarchicalDatas->HierarchicalData[v74]->Coefficient )
        {
          v50 = v74;
        }
      }
      while ( v50 != v47 );
      v5 = this;
LABEL_75:
      v16 = v86;
      v62 = v86;
      if ( !v86 )
        return 0;
      v7 = v75;
      v19 = v67 * 24;
    }
    if ( !v16 )
      return 0;
    v53 = v16;
    v87 = v16;
    if ( v16->NodeIndex != -1 )
    {
      do
      {
        v88[v16->FinderIndex] = v5->FindCount;
        v16 = &v5->HierarchicalPriorityQueueBuffer->HierarchicalData[v16->NodeIndex];
      }
      while ( v16->NodeIndex != -1 );
      v53 = v87;
    }
    v54 = v53->Count + 1;
    v5->PassabilityCounts[v7] = v54;
    v55 = v54 - 1;
    if ( v55 > 0 )
    {
      v56 = &v5->PassabilityIndexes[v7].Indexes[v55];
      do
      {
        *v56-- = v53->FinderIndex;
        v53 = &v5->HierarchicalPriorityQueueBuffer->HierarchicalData[v53->NodeIndex];
        --v55;
      }
      while ( v55 );
    }
    v5->PassabilityIndexes[v7].Indexes[0] = v53->FinderIndex;
LABEL_85:
    v75 = --v7;
    if ( v7 >= 0 )
      continue;
    return 1;
  }
}

AStarClass::WorkPathQueueNode* AStarClassFake::CreateNode_AsVanilla
(
	WorkPathQueueNode* a2,
	CellClass** a3,
	CellStruct* a4,
	float a5
)
{
  AStarClass_WorkPathQueueBuffer *WorkPathPriorityQueueBuffer; // eax
  AStarClass_WorkPathQueueNode *v6; // esi
  AStarClass_WorkPathHeapData *WorkPathHeapData; // eax
  int Count; // edx
  AStarClass_WorkPathHeapNode *v9; // edi
  CellClass *v10; // eax
  int Level; // edx
  CellClass *v12; // ecx
  int v13; // ebp
  __int64 v14; // rax
  int SourceCellLevel; // eax
  unsigned int v16; // ecx
  __int64 v17; // rax
  double v18; // st7
  AStarClass_WorkPathQueueNode *result; // eax

  WorkPathPriorityQueueBuffer = this->WorkPathPriorityQueueBuffer;
  v6 = &WorkPathPriorityQueueBuffer->WorkPathData[WorkPathPriorityQueueBuffer->Count++];
  WorkPathHeapData = this->WorkPathHeapData;
  Count = WorkPathHeapData->Count;
  WorkPathHeapData->Count = Count + 1;
  v9 = &WorkPathHeapData->WorkPathHeapData[Count];
  v9->Items = a3;
  if ( a2 )
  {
    v9->Prev = a2->WorkPathHeapNodes;
    v10 = *a3;
    Level = (*a3)->Level;
    v12 = *a2->WorkPathHeapNodes->Items;
    v9->Level = Level;
    if ( (v10->Flags & 0x100) == 0 )
      goto LABEL_12;
    v13 = v12->Flags & 0x100;
    if ( v13 )
    {
      if ( a2->WorkPathHeapNodes->Level == v12->Level + 4 )
      {
        v9->Level = Level + 4;
        goto LABEL_12;
      }
      v13 = v12->Flags & 0x100;
    }
    if ( v13 )
      goto LABEL_12;
    v14 = v10->Level - a2->WorkPathHeapNodes->Level + 3;
    if ( (int)((HIDWORD(v14) ^ v14) - HIDWORD(v14)) > 1 )
      goto LABEL_12;
    SourceCellLevel = v9->Level + 4;
  }
  else
  {
    v9->Prev = 0;
    SourceCellLevel = this->SourceCellLevel;
  }
  v9->Level = SourceCellLevel;
LABEL_12:
  v6->WorkPathHeapNodes = v9;
  if ( a2 )
  {
    v6->MovementDistance = a5 + a2->MovementDistance;
    v6->AccumulatedCount = a2->AccumulatedCount + 1;
  }
  else
  {
    v6->MovementDistance = 0.0;
    v6->AccumulatedCount = 1;
  }
  v16 = abs32((*a3)->MapCoords.X - a4->X);
  v17 = (*a3)->MapCoords.Y - a4->Y;
  v18 = sqrt((double)(int)(v16 * v16 + ((HIDWORD(v17) ^ v17) - HIDWORD(v17)) * ((HIDWORD(v17) ^ v17) - HIDWORD(v17))));
  result = v6;
  v6->AccumulatedDistance = v18 + v6->MovementDistance;
  return result;
}

void AStarClassFake::PostProcessCells_AsVanilla(FootClass* pFoot)
{
  bool CanFind; // al
  FootClass *v3; // esi
  CellClass *CellAt; // edi
  unsigned int v5; // edx
  __int16 v6; // cx
  CellClass *v7; // ebx
  __int64 v8; // rax
  FootClass *AltObject; // ebp
  bool v10; // al
  FootClass_vtbl *v11; // eax
  TechnoTypeClass *v12; // esi
  int v13; // ebx
  TechnoTypeClass *v14; // eax
  int *PathDirections; // edi
  int v16; // eax
  int TubeIndex; // eax
  CellStruct ExitCell; // eax
  __int16 Y; // dx
  CellClass *v20; // esi
  CellClass *v21; // eax
  int v22; // edi
  int v23; // esi
  CellClass *v24; // eax
  CellStruct CurrentMapCoords; // [esp+10h] [ebp-28h] BYREF
  CellClass *v26; // [esp+14h] [ebp-24h]
  CellStruct a2; // [esp+18h] [ebp-20h] BYREF
  AStarClass *v28; // [esp+1Ch] [ebp-1Ch]
  CellStruct MapCoords; // [esp+20h] [ebp-18h]
  CellStruct v30; // [esp+28h] [ebp-10h]
  CellStruct v31; // [esp+2Ch] [ebp-Ch] BYREF
  TechnoTypeClass *v32; // [esp+30h] [ebp-8h]
  CellStruct v33; // [esp+30h] [ebp-8h] SPLIT BYREF
  DirStruct v34; // [esp+34h] [ebp-4h] BYREF
  CellStruct v35; // [esp+3Ch] [ebp+4h] SPLIT
  bool v36; // [esp+3Ch] [ebp+4h] SPLIT
  CellStruct v37; // [esp+3Ch] [ebp+4h] SPLIT

  CanFind = this->CanFind;
  v28 = this;
  if ( !CanFind )
    return;
  v3 = pFoot;
  a2 = *pFoot->GetMapCoords(pFoot, &pFoot);
  CellAt = MapClass::GetCellAt(&MouseClass::Instance, &a2);
  v5 = (((*(unsigned int *)FacingClass::Current(&v3->PrimaryFacing, &v34) >> 12) + 1) >> 1) & 7;
  v6 = Unsorted::AdjacentCell[v5].X + a2.X;
  LOWORD(v5) = Unsorted::AdjacentCell[v5].Y;
  v35.X = v6;
  v35.Y = a2.Y + v5;
  v31 = v35;
  v7 = MapClass::GetCellAt(&MouseClass::Instance, &v31);
  v26 = v7;
  if ( (v7->Flags & CellFlags_BridgeHead) != 0
    && ((v8 = CellAt->Level - v7->Level, (int)((HIDWORD(v8) ^ v8) - HIDWORD(v8)) > 3) || v3->OnBridge) )
  {
    AltObject = (FootClass *)v7->AltObject;
    v10 = 1;
  }
  else
  {
    AltObject = (FootClass *)v7->FirstObject;
    v10 = 0;
  }
  if ( !AltObject )
    AltObject = AStarClass::GetOccupier(v28, &v7->MapCoords, (v10 ? 4 : 0) + v7->Level);
  v11 = v3->TechnoClass::RadioClass::MissionClass::ObjectClass::AbstractClass::IPersistStream::IPersist::IUnknown::__vftable;
  v36 = 0;
  v12 = v11->GetTechnoType(v3);
  v32 = v12;
  if ( AltObject )
  {
    while ( 1 )
    {
      if ( AltObject->WhatAmI(AltObject) != AbstractType_Unit && AltObject->WhatAmI(AltObject) != AbstractType_Infantry )
        goto LABEL_36;
      v13 = 0;
      CurrentMapCoords = AltObject->CurrentMapCoords;
      v14 = AltObject->GetTechnoType(AltObject);
      if ( v28->FindMode == 2
        || v12 != v14
        && v12->Speed > v14->Speed
        && MapClass::IsCellWithinUsableArea(&MouseClass::Instance, &CurrentMapCoords, 1) )
      {
        PathDirections = AltObject->PathDirections;
        if ( AltObject->WhatAmI(AltObject) == AbstractType_Unit )
        {
          if ( *PathDirections != -1 && AltObject->PathDirections[1] != -1 )
            goto LABEL_27;
        }
        else if ( *PathDirections != -1 && AltObject->PathDirections[1] != -1 && AltObject->PathDirections[2] != -1 )
        {
LABEL_27:
          v36 = 1;
          do
          {
            if ( v13 >= 24 )
              break;
            v16 = *PathDirections;
            if ( *PathDirections == 8 )
            {
              TubeIndex = MapClass::GetCellAt(&MouseClass::Instance, &CurrentMapCoords)->TubeIndex;
              if ( TubeIndex == -1 )
              {
                MapCoords.X = 0;
                MapCoords.Y = 0;
                ExitCell = 0;
              }
              else
              {
                ExitCell = TubeClass::Array.Items[TubeIndex]->ExitCell;
              }
            }
            else
            {
              Y = Unsorted::AdjacentCell[v16].Y;
              v30.X = CurrentMapCoords.X + Unsorted::AdjacentCell[v16].X;
              v30.Y = CurrentMapCoords.Y + Y;
              ExitCell = v30;
            }
            CurrentMapCoords = ExitCell;
            v20 = MapClass::GetCellAt(&MouseClass::Instance, &CurrentMapCoords);
            v21 = MapClass::GetCellAt(&MouseClass::Instance, &CurrentMapCoords);
            ++PathDirections;
            ++v13;
            v21->Flags ^= (v21->Flags ^ ~v20->Flags) & CellFlags_Tube;
          }
          while ( *PathDirections != -1 );
          v7 = v26;
LABEL_36:
          AltObject = (FootClass *)AltObject->NextObject;
          v12 = v32;
          goto LABEL_20;
        }
      }
      AltObject = (FootClass *)AltObject->NextObject;
      v7 = v26;
LABEL_20:
      if ( !AltObject )
      {
        if ( !v36 )
          break;
LABEL_37:
        v22 = -2;
        do
        {
          v23 = -2;
          do
          {
            MapCoords = v7->MapCoords;
            v37.X = v22 + MapCoords.X;
            v37.Y = v23 + MapCoords.Y;
            v33 = v37;
            v24 = MapClass::GetCellAt(&MouseClass::Instance, &v33);
            if ( LOBYTE(v24->OccupationFlags) )
            {
              v30 = v24->MapCoords;
              if ( v30 != a2 )
                v24->Flags ^= (v24->Flags ^ ~v24->Flags) & CellFlags_Tube;
            }
            ++v23;
          }
          while ( v23 < 3 );
          ++v22;
        }
        while ( v22 < 3 );
        v7->Flags ^= (v7->Flags ^ ~v7->Flags) & CellFlags_Tube;
        return;
      }
    }
  }
  if ( v28->FindMode != 1 )
    goto LABEL_37;
  v28->FindMode = 0;
}

double AStarClassFake::GetMovementCost
(
	CellClass** a2,
	CellClass** a3,
	bool a4,
	int a5,
	FootClass* pFoot
)
{
  CellClass **v6; // esi
  CellClass *v7; // ebx
  bool v8; // zf
  FootClass *AltObject; // edi
  int v10; // ebp
  unsigned int v11; // esi
  CellStruct *v12; // eax
  int v13; // esi
  CellClass *CellAt; // esi
  FootClass *FirstObject; // esi
  CellFlags Flags; // edx
  double v17; // st7
  int v18; // eax
  CellClass *v19; // ecx
  int v20; // edx
  CellStruct v22; // [esp+Ch] [ebp-18h]
  AStarClass *v23; // [esp+10h] [ebp-14h]
  CellStruct a24; // [esp+14h] [ebp-10h] BYREF
  CellClass *v25; // [esp+18h] [ebp-Ch]
  DirStruct v26; // [esp+1Ch] [ebp-8h] BYREF
  CellStruct v27; // [esp+20h] [ebp-4h] BYREF
  float a5a; // [esp+34h] [ebp+10h]

  v6 = a3;
  v7 = *a3;
  v8 = a5 == 2;
  v23 = this;
  a5a = flt_81870C[a5];
  v25 = *a2;
  if ( v8 )
  {
    if ( a4 )
      AltObject = (FootClass *)v7->AltObject;
    else
      AltObject = (FootClass *)v7->FirstObject;
    v10 = 0;
    if ( this->FindMode )
      goto LABEL_20;
    while ( AltObject )
    {
      if ( (AltObject->AbstractFlags & 4) == 0 )
        goto LABEL_20;
      if ( AltObject->SpeedPercentage == 0.0 )
      {
        v11 = AltObject->PathDirections[0];
        if ( v11 == -1 )
          break;
      }
      else
      {
        v11 = (((*(unsigned int *)FacingClass::Current(&AltObject->PrimaryFacing, &v26) >> 12) + 1) >> 1) & 7;
      }
      v12 = AltObject->GetMapCoords(AltObject, &v27);
      v13 = v11 & 7;
      v22.X = Unsorted::AdjacentCell[v13].X + v12->X;
      v22.Y = v12->Y + Unsorted::AdjacentCell[v13].Y;
      a24 = v22;
      CellAt = MapClass::GetCellAt(&MouseClass::Instance, &a24);
      if ( (CellAt->Flags & CellFlags_BridgeHead) != 0
        && (AltObject->OnBridge || AltObject->GetCell(AltObject)->Level - CellAt->Level > 2) )
      {
        FirstObject = (FootClass *)CellAt->AltObject;
      }
      else
      {
        FirstObject = (FootClass *)CellAt->FirstObject;
      }
      this = v23;
      ++v10;
      AltObject = FirstObject;
      if ( v10 >= 10 )
        goto LABEL_20;
    }
    if ( this->FindMode )
LABEL_20:
      a5a = 4.0;
    if ( this->FindMode == 2 )
      a5a = 1000.0;
    v6 = a3;
  }
  Flags = v7->Flags;
  if ( (Flags & CellFlags_Tube) != 0 )
    a5a = a5a * 4.0;
  if ( !a4 || !this->FindBridgeOwner )
    return a5a;
  v17 = 1.0;
  v18 = dword_7E3760[3 * (__int16)(v7->MapCoords.Y - v25->MapCoords.Y) + (__int16)(v7->MapCoords.X - v25->MapCoords.X)];
  if ( (BYTE1(Flags) & 8) != 0 )                // CellFlags_BridgeOwner
  {
    v19 = v6[dword_7E3730[v18]];
    v20 = dword_7E3730[((_BYTE)v18 - 4) & 7];
  }
  else
  {
    v19 = v6[dword_7E3710[v18]];
    v20 = dword_7E3710[((_BYTE)v18 - 4) & 7];
  }
  if ( (v19->Flags & CellFlags_BridgeHead) == 0 )
    return 10.0 * a5a;
  if ( (v6[v20]->Flags & CellFlags_BridgeHead) != 0 )
    v17 = 2.0;
  return v17 * a5a;
}

AStarClass::PathType* AStarClassFake::BuildFinalPath_AsVanilla(WorkPathQueueNode* a2, int* a3)
{
  AStarClass_WorkPathQueueNode *v3; // esi
  int v4; // eax
  int *v5; // edx
  AStarClass_WorkPathHeapNode *WorkPathHeapNodes; // ebp
  AStarClass_WorkPathHeapNode *Prev; // ebx
  int v8; // eax
  int v9; // ecx
  __int64 v10; // rax
  int X; // ecx
  int v12; // esi
  int v13; // eax
  bool v14; // zf
  int *v16; // [esp+0h] [ebp-Ch]
  int v17; // [esp+4h] [ebp-8h]

  if ( (PathFinderOn & 1) == 0 )
  {
    PathFinderOn |= 1u;
    atexit(nullsub_4);
  }
  v3 = a2;
  v4 = F2I64(a2->AccumulatedDistance);
  v5 = a3;
  AStarClass::PathData.AccumulatedDistance = v4;
  AStarClass::PathData.AccumulatedCount = a2->AccumulatedCount;
  AStarClass::PathData.unknown_int_10 = 0;
  AStarClass::PathData.unknown_cellstruct_18.X = 0;
  AStarClass::PathData.unknown_cellstruct_18.Y = 0;
  AStarClass::PathData.Facings = a3;
  AStarClass::PathData.Levels = &dword_89A324;
  WorkPathHeapNodes = a2->WorkPathHeapNodes;
  Prev = a2->WorkPathHeapNodes->Prev;
  v8 = a2->AccumulatedCount - 2;
  if ( v8 >= 0 )
  {
    v9 = (char *)&dword_89A324 - (char *)a3;
    v16 = &a3[v8];
    v17 = v8 + 1;
    do
    {
      if ( Prev )
      {
        *(int *)((char *)v16 + v9) = Prev->Level;
        v10 = (*Prev->Items)->MapCoords.Y - (*WorkPathHeapNodes->Items)->MapCoords.Y;
        if ( (int)((HIDWORD(v10) ^ v10) - HIDWORD(v10)) > 1
          || (X = (*Prev->Items)->MapCoords.X, v12 = (*WorkPathHeapNodes->Items)->MapCoords.X, (int)abs32(X - v12) > 1) )
        {
          v13 = 8;
        }
        else
        {
          v13 = dword_818750[X + 4 + 3 * ((*Prev->Items)->MapCoords.Y - (*WorkPathHeapNodes->Items)->MapCoords.Y) - v12];
        }
        v3 = a2;
        v5 = a3;
        *v16 = v13;
        v9 = (char *)&dword_89A324 - (char *)a3;
      }
      WorkPathHeapNodes = WorkPathHeapNodes->Prev;
      Prev = Prev->Prev;
      v14 = v17 == 1;
      --v16;
      --v17;
    }
    while ( !v14 );
  }
  v5[v3->AccumulatedCount - 1] = -1;
  AStarClass::PathData.Start = (*WorkPathHeapNodes->Items)->MapCoords;
  if ( !AStarClass::PathData.AccumulatedDistance )
    AStarClass::PathData.AccumulatedDistance = 1;
  return &AStarClass::PathData;
}

void AStarClassFake::ProcessFinalPath_AsVanilla(PathType* pPath, FootClass* pFoot)
{
  int *Levels; // edx
  int *Facings; // edi
  CellStruct Start; // eax
  int v6; // ecx
  int v7; // ebx
  int v8; // ebp
  int v9; // esi
  bool v10; // zf
  int v11; // eax
  int v12; // ecx
  int TubeIndex; // eax
  __int16 v14; // dx
  int v15; // eax
  __int16 Y; // dx
  CellStruct a2; // [esp+10h] [ebp-2Ch] BYREF
  int a5; // [esp+14h] [ebp-28h]
  CellStruct a6; // [esp+18h] [ebp-24h] BYREF
  int v20; // [esp+1Ch] [ebp-20h]
  int v21; // [esp+20h] [ebp-1Ch]
  CellStruct v22; // [esp+24h] [ebp-18h]
  int v23; // [esp+28h] [ebp-14h]
  CellStruct v24; // [esp+2Ch] [ebp-10h]
  int v25; // [esp+30h] [ebp-Ch]
  int *v26; // [esp+34h] [ebp-8h]
  AStarClass *v27; // [esp+38h] [ebp-4h]
  char arg0a; // [esp+40h] [ebp+4h]

  v27 = this;
  Levels = pPath->Levels;
  Facings = pPath->Facings;
  Start = pPath->Start;
  v7 = -1;
  v8 = 0;
  v9 = 0;
  v10 = pPath->AccumulatedCount == 1;
  v25 = pPath->AccumulatedCount - 1;
  v6 = v25;
  v23 = -1;
  v26 = Levels;
  v20 = 0;
  arg0a = 0;
  a5 = 0;
  a6 = Start;
  a2 = Start;
  if ( v25 >= 0 && !v10 )
  {
    while ( 1 )
    {
      if ( a5 + v20 >= v6 )
      {
LABEL_26:
        if ( arg0a )
          AStarClass::FixupFinalPath(v27, pFoot, &Facings[v9], &v26[v9], v8, a5, &a6);
        return;
      }
      if ( !arg0a )
        break;
      if ( Facings[a5 + v20] != v23 )
      {
        v9 += AStarClass::FixupFinalPath(v27, pFoot, &Facings[v9], &v26[v9], v8, a5, &a6);
        v8 = 1;
        arg0a = 0;
        v15 = Facings[v9] & 7;
        Y = Unsorted::AdjacentCell[v15].Y;
        v24.X = Unsorted::AdjacentCell[v15].X + a6.X;
        v24.Y = a6.Y + Y;
        a2 = v24;
        v7 = Facings[v9];
LABEL_24:
        v6 = v25;
        goto LABEL_25;
      }
      ++a5;
LABEL_25:
      if ( v9 + v8 >= v6 )
        goto LABEL_26;
    }
    v11 = Facings[v9 + v8];
    v12 = ((_BYTE)v11 - (_BYTE)v7) & 7;
    if ( v11 == v7 )
    {
      ++v8;
    }
    else if ( v12 != 2 && v12 != 6 || v7 == -1 || v7 == 8 || v11 == 8 )
    {
      v9 += v8;
      v8 = 1;
      if ( (v11 & 1) != 0 )
        v7 = v11;
      else
        v7 = -1;
      a6 = a2;
    }
    else
    {
      arg0a = 1;
      v23 = Facings[v9 + v8];
      a5 = 1;
      v20 = v9 + v8;
    }
    if ( v11 == 8 )
    {
      TubeIndex = MapClass::GetCellAt(&MouseClass::Instance, &a2)->TubeIndex;
      if ( TubeIndex == -1 )
      {
        v21 = 0;
        a2 = 0;
      }
      else
      {
        a2 = TubeClass::Array.Items[TubeIndex]->ExitCell;
      }
    }
    else
    {
      v14 = Unsorted::AdjacentCell[v11].Y;
      v22.X = a2.X + Unsorted::AdjacentCell[v11].X;
      v22.Y = a2.Y + v14;
      a2 = v22;
    }
    goto LABEL_24;
  }
}

int AStarClassFake::FixupFinalPath_AsVanilla
(
	FootClass* pFoot,
	int* facings,
	int* levels,
	int a5,
	int a6,
	CellStruct* pCurCell
)
{
  int *facings_1; // ebp
  int count_1; // esi
  int firstDir; // edx
  int lastDir; // eax
  int avgDir; // edi
  CellStruct currentCell_2; // eax
  int *facings_2; // ebx
  int revIndex; // ebp
  int TubeIndex; // eax
  Vector2D_short *newCell; // eax
  double ThreatAvoidanceCoefficient; // st7
  HouseClass *Owner; // eax
  __int16 X; // ax
  int v20; // ecx
  CellStruct *v21; // ebp
  __int16 v22; // dx
  __int16 v23; // dx
  __int16 v24; // ax
  int v25; // esi
  CellClass *CellAt; // edi
  bool v27; // bl
  __int16 v28; // dx
  __int16 v29; // ax
  int Level; // eax
  int v31; // eax
  __int16 v32; // dx
  CellStruct v33; // eax
  int *v34; // edi
  int v35; // ebx
  int *v37; // edx
  int v38; // esi
  CellStruct *v39; // ebp
  CellStruct ExitCell; // eax
  int *v41; // edi
  int v42; // ebx
  int v43; // eax
  Vector2D_short *v44; // eax
  CellStruct *pCurrentCell_1; // ebp
  CellStruct currentCell; // eax
  int v47; // ebx
  int *facings_3; // esi
  int v49; // edi
  CellStruct currentCell_3; // [esp+14h] [ebp-34h] BYREF
  CellStruct currentCell_4; // [esp+18h] [ebp-30h] BYREF
  int avgDir_1; // [esp+1Ch] [ebp-2Ch]
  int v53; // [esp+20h] [ebp-28h]
  int v54; // [esp+24h] [ebp-24h]
  CellStruct v55; // [esp+28h] [ebp-20h]
  CellStruct v56; // [esp+2Ch] [ebp-1Ch]
  CellStruct v57; // [esp+30h] [ebp-18h]
  Vector2D_short v58; // [esp+34h] [ebp-14h] BYREF
  HouseClass *v59; // [esp+34h] [ebp-14h] SPLIT
  Vector2D_short ThreatPosed; // [esp+38h] [ebp-10h] BYREF
  int firstDir_1; // [esp+3Ch] [ebp-Ch]
  double v62; // [esp+40h] [ebp-8h]
  CellStruct v63; // [esp+50h] [ebp+8h] SPLIT BYREF
  CellStruct v64; // [esp+60h] [ebp+18h] SPLIT BYREF
  CellStruct currentCell_1; // [esp+60h] [ebp+18h] SPLIT BYREF

  facings_1 = facings;
  count_1 = a5;
  firstDir = *facings;
  lastDir = facings[a5];
  firstDir_1 = firstDir;
  avgDir = (firstDir + lastDir) >> 1;
  avgDir_1 = avgDir;
  if ( avgDir + 1 != lastDir && avgDir + 1 != firstDir )
  {
    avgDir_1 = 0;
    LOBYTE(avgDir) = 0;
  }
  if ( firstDir == 8 || lastDir == 8 )
  {
    pCurrentCell_1 = pCurCell;
    currentCell = *pCurCell;
    v47 = a5 + a6;
    currentCell_1 = *pCurCell;
    if ( a5 + a6 > 0 )
    {
      facings_3 = facings;
      v49 = a5 + a6;
      do
      {
        currentCell = *CellStruct::NextPathCell((CellStruct *)&facings, &currentCell_1, *facings_3++);
        --v49;
        currentCell_1 = currentCell;
      }
      while ( v49 );
    }
    *pCurrentCell_1 = currentCell;
    return v47;
  }
  currentCell_2 = *pCurCell;
  currentCell_3 = *pCurCell;
  if ( a5 < a6 )
  {
    a6 = a5;
    goto LABEL_20;
  }
  if ( a6 < a5 )
  {
    currentCell_4 = currentCell_2;
    if ( a5 - a6 <= 0 )
      goto LABEL_19;
    facings_2 = facings;
    revIndex = a5 - a6;
    while ( *facings_2 == 8 )
    {
      TubeIndex = MapClass::GetCellAt(&MouseClass::Instance, &currentCell_4)->TubeIndex;
      if ( TubeIndex == -1 )
      {
        newCell = CellStruct::SetCellStruct(&ThreatPosed, 0, 0);
LABEL_16:
        currentCell_2 = *newCell;
        goto LABEL_17;
      }
      currentCell_2 = TubeClass::Array.Items[TubeIndex]->ExitCell;
LABEL_17:
      ++facings_2;
      --revIndex;
      currentCell_4 = currentCell_2;
      if ( !revIndex )
      {
        facings_1 = facings;
LABEL_19:
        currentCell_3 = currentCell_2;
        goto LABEL_20;
      }
    }
    newCell = CellStruct::AddCellStruct(&currentCell_4, &v58, &Unsorted::AdjacentCell[*facings_2]);
    goto LABEL_16;
  }
LABEL_20:
  ThreatAvoidanceCoefficient = FootClass::GetThreatAvoidanceCoefficient(pFoot);
  Owner = pFoot->Owner;
  v62 = ThreatAvoidanceCoefficient;
  v59 = Owner;
  if ( a6 <= 0 )
    goto LABEL_38;
  X = currentCell_3.X;
  v20 = 2 * a6;
  v54 = 2 * a6;
  v21 = &Unsorted::AdjacentCell[avgDir & 7];
  while ( 1 )
  {
    v22 = v21->X;
    v53 = v20;
    v23 = X + v22;
    v24 = currentCell_3.Y + v21->Y;
    v55.X = v23;
    v55.Y = v24;
    currentCell_4 = v55;
    v25 = levels[count_1 + a6 - v20];
    CellAt = MapClass::GetCellAt(&MouseClass::Instance, &currentCell_4);
    do
    {
      if ( v53 <= 0 )
      {
        v37 = facings;
        v38 = a5 - a6;
        if ( 2 * a6 > 0 )
          memset32(&facings[a5 - a6], avgDir_1, 2 * a6);
        v39 = pCurCell;
        ExitCell = *pCurCell;
        v64 = *pCurCell;
        if ( v38 <= 0 )
        {
LABEL_53:
          *v39 = ExitCell;
          return v38;
        }
        v41 = v37;
        v42 = v38;
        while ( *v41 == 8 )
        {
          v43 = MapClass::GetCellAt(&MouseClass::Instance, &v64)->TubeIndex;
          if ( v43 == -1 )
          {
            v44 = CellStruct::SetCellStruct((CellStruct *)&facings, 0, 0);
LABEL_51:
            ExitCell = *v44;
            goto LABEL_52;
          }
          ExitCell = TubeClass::Array.Items[v43]->ExitCell;
LABEL_52:
          ++v41;
          --v42;
          v64 = ExitCell;
          if ( !v42 )
            goto LABEL_53;
        }
        v44 = CellStruct::AddCellStruct(&v64, (Vector2D_short *)&a5, &Unsorted::AdjacentCell[*v41]);
        goto LABEL_51;
      }
      v27 = 1;
      if ( pFoot->IsCellOccupied(pFoot, CellAt, (FacingType)avgDir_1, v25, 0, 1) == Move_OK
        && (CellAt->Flags & CellFlags_Tube) == 0 )
      {
        ThreatPosed = (Vector2D_short)MapClass::GetThreatPosed(&MouseClass::Instance, &currentCell_3, v59);
        if ( (double)*(int *)&ThreatPosed * v62 < 1.0 )
          v27 = 0;
      }
      v28 = currentCell_4.X + v21->X;
      --v53;
      v29 = currentCell_4.Y + v21->Y;
      v56.X = v28;
      v56.Y = v29;
      currentCell_4 = v56;
      CellAt = MapClass::GetCellAt(&MouseClass::Instance, &currentCell_4);
      Level = CellAt->Level;
      if ( v25 - Level == 4 )
      {
        v25 = Level + 4;
        if ( (CellAt->Flags & CellFlags_BridgeHead) != 0 )
          continue;
      }
      v25 = CellAt->Level;
    }
    while ( !v27 );
    v31 = firstDir_1 & 7;
    v32 = currentCell_3.Y + Unsorted::AdjacentCell[v31].Y;
    v57.X = Unsorted::AdjacentCell[v31].X + currentCell_3.X;
    v57.Y = v32;
    X = v57.X;
    v54 -= 2;
    count_1 = a5;
    currentCell_3 = v57;
    if ( --a6 <= 0 )
      break;
    v20 = v54;
  }
  facings_1 = facings;
LABEL_38:
  v33 = *pCurCell;
  v63 = *pCurCell;
  if ( count_1 > 0 )
  {
    v34 = facings_1;
    v35 = count_1;
    do
    {
      v33 = *CellStruct::NextPathCell((CellStruct *)&a5, &v63, *v34++);
      --v35;
      v63 = v33;
    }
    while ( v35 );
  }
  *pCurCell = v33;
  return count_1;
}

void AStarClassFake::OptimizeFinalPath_AsVanilla(PathType* pPath, FootClass* pFoot)
{
  AStarClass_PathType *v3; // ecx
  int *Facings; // edx
  CellStruct Start; // edi
  int *Levels; // ebp
  int v7; // ebx
  int v8; // esi
  bool v9; // cc
  int v10; // eax
  int v11; // eax
  __int16 X; // cx
  __int16 v13; // bx
  __int16 v14; // bp
  int v15; // ecx
  int v16; // eax
  int v17; // ecx
  int v18; // eax
  int v19; // edx
  __int16 v20; // cx
  __int16 v21; // bx
  AStarClass *v22; // ebp
  int v23; // ecx
  int v24; // ecx
  int v25; // eax
  AStarClass *v26; // ebp
  int v27; // esi
  int *v28; // edx
  int v29; // esi
  int v30; // eax
  int v31; // ebp
  int *v32; // edi
  int v33; // edx
  int *v34; // eax
  int *v35; // [esp+10h] [ebp-80h]
  CellStruct v36; // [esp+14h] [ebp-7Ch]
  CellStruct v37; // [esp+18h] [ebp-78h]
  int *v38; // [esp+1Ch] [ebp-74h]
  int v39; // [esp+20h] [ebp-70h]
  int v40; // [esp+24h] [ebp-6Ch]
  int v41; // [esp+28h] [ebp-68h]
  int v42; // [esp+2Ch] [ebp-64h]
  CellStruct v43; // [esp+30h] [ebp-60h]
  int v44; // [esp+34h] [ebp-5Ch]
  int a5; // [esp+38h] [ebp-58h] BYREF
  CellStruct v46; // [esp+3Ch] [ebp-54h]
  __int16 v47; // [esp+40h] [ebp-50h]
  __int16 v48; // [esp+42h] [ebp-4Eh]
  int v49; // [esp+44h] [ebp-4Ch]
  int v50; // [esp+48h] [ebp-48h]
  CellStruct v51; // [esp+4Ch] [ebp-44h]
  CellStruct v52; // [esp+50h] [ebp-40h]
  int v53; // [esp+54h] [ebp-3Ch]
  CellStruct v54; // [esp+58h] [ebp-38h]
  int v55; // [esp+5Ch] [ebp-34h]
  AStarClass *v56; // [esp+60h] [ebp-30h]
  CellStruct v57; // [esp+60h] [ebp-30h] SPLIT
  int *v58; // [esp+64h] [ebp-2Ch]
  int v59; // [esp+68h] [ebp-28h]
  CellStruct v60; // [esp+6Ch] [ebp-24h] SPLIT BYREF
  CellStruct v61; // [esp+70h] [ebp-20h]
  CellStruct a3; // [esp+78h] [ebp-18h] BYREF
  CellStruct v63; // [esp+80h] [ebp-10h]
  CellStruct v64; // [esp+88h] [ebp-8h]

  v56 = this;
  v3 = pPath;
  Facings = pPath->Facings;
  Start = pPath->Start;
  Levels = pPath->Levels;
  v7 = 0;
  v8 = 0;
  v9 = pPath->AccumulatedCount - 1 <= 0;
  v38 = Facings;
  v59 = pPath->AccumulatedCount - 1;
  v44 = 0;
  v63 = Start;
  v58 = Levels;
  v42 = 0;
  v41 = 0;
  v37 = 0;
  v61.X = 0;
  v61.Y = 0;
  v39 = 0;
  v40 = 0;
  v36 = 0;
  if ( !v9 )
  {
    v35 = Facings;
    do
    {
      if ( v8 >= 20 )
        break;
      v10 = *v35;
      v60 = (CellStruct)*v35;
      if ( v60 == 8 )
      {
        v47 = 0;
        v48 = 0;
        v46.Y = Unsorted::AdjacentCell[0].Y + v63.Y;
        ++v8;
        v46.X = Unsorted::AdjacentCell[0].X + Start.X;
        Start = v46;
        v37 = 0;
        ++v35;
        v49 = 0;
        v50 = 0;
        v63 = v46;
        v44 = v8;
        v41 = 0;
        v39 = 0;
        v40 = 0;
        v36 = 0;
        v42 = v8;
        v61 = 0;
      }
      else if ( v10 == -2 )
      {
        ++v8;
        ++v35;
      }
      else
      {
        v11 = v10 & 7;
        X = Unsorted::AdjacentCell[v11].X;
        v13 = v37.X + X;
        LOWORD(v11) = Unsorted::AdjacentCell[v11].Y;
        v14 = v37.Y + v11;
        v43.X = v36.X + X;
        v52.X = v37.X + X;
        v52.Y = v37.Y + v11;
        v43.Y = v36.Y + v11;
        v15 = abs16(v36.X + X);
        if ( v15 < v39 || (v16 = abs16(v43.Y), v16 < v40) )
        {
          if ( v61.X || v61.Y )
          {
            v44 = v42;
            v54.X = v61.X - Start.X;
            v54.Y = v61.Y - v63.Y;
            v37 = v54;
            v39 = 0;
            v40 = 0;
            v55 = 0;
            v36 = 0;
            v23 = abs16(v61.Y - v63.Y);
            v41 = abs16(v61.X - Start.X);
            if ( v41 <= v23 )
              v41 = v23;
            v61 = Start;
            v42 = v8;
          }
          else
          {
            v39 = 0;
            v40 = 0;
            v53 = 0;
            v61 = Start;
            v42 = v8;
            v36 = 0;
          }
        }
        else
        {
          v40 = v16;
          v36 = v43;
          v39 = v15;
          v17 = abs16(v14);
          v18 = abs16(v13);
          if ( v18 <= v17 )
            v18 = v17;
          v19 = v60.X & 7;
          v20 = Unsorted::AdjacentCell[v19].X;
          LOWORD(v19) = Unsorted::AdjacentCell[v19].Y;
          v64.X = Start.X + v20;
          v64.Y = v63.Y + v19;
          v21 = Start.X + v20;
          Start = v64;
          v63 = v64;
          if ( v41 >= v18 )
          {
            v22 = v56;
            a3 = v64;
            AStarClass::AdjacentCell(v56, v38, v8, v44, &a5, &a3);
            v51.X = v21 - a3.X;
            v51.Y = v64.Y - a3.Y;
            v60 = v51;
            AStarClass::PlotStraightLine(v22, &v38[a5], v8 - a5 + 1, &a3, &v60, pFoot, v58[a5], 0);
          }
          else
          {
            v41 = v18;
          }
          ++v8;
          v37 = v52;
          ++v35;
        }
        v3 = pPath;
        v7 = 0;
      }
    }
    while ( v8 < v59 );
    if ( v61.X || v61.Y )
    {
      v24 = abs16(v63.Y - v61.Y);
      v25 = abs16(Start.X - v61.X);
      if ( v25 <= v24 )
        v25 = v24;
      if ( v8 - v42 - 1 > v25 )
      {
        v26 = v56;
        v27 = v8 - 1;
        a3 = Start;
        AStarClass::AdjacentCell(v56, v38, v27, v42, &a5, &a3);
        v57.X = Start.X - a3.X;
        v57.Y = v63.Y - a3.Y;
        v60 = v57;
        AStarClass::PlotStraightLine(v26, &v38[a5], v27 - a5 + 1, &a3, &v60, pFoot, v58[a5], 1);
      }
      v3 = pPath;
    }
  }
  v28 = v38;
  v29 = 0;
  v30 = *v38;
  if ( *v38 != -1 )
  {
    v31 = v59;
    v32 = v38;
    do
    {
      if ( v29 >= v31 )
        break;
      if ( v30 != -2 )
      {
        ++v7;
        *v32++ = *v28;
      }
      v30 = v28[1];
      ++v28;
      ++v29;
    }
    while ( v30 != -1 );
  }
  v33 = v7;
  if ( v7 < v3->AccumulatedCount + 1 )
  {
    v34 = &v38[v7];
    do
    {
      *v34 = -1;
      ++v33;
      ++v34;
    }
    while ( v33 < v3->AccumulatedCount + 1 );
  }
  v3->AccumulatedCount = v7 + 1;
}

void AStarClassFake::AdjacentCell_AsVanilla
(
	int* a2,
	int a3,
	int a4,
	int* a5,
	CellStruct* pAdjCell
)
{
  CellStruct v7; // edx
  int v8; // esi
  __int16 v9; // cx
  __int16 v10; // bx
  bool v11; // cc
  int v12; // eax
  __int16 X; // si
  CellStruct *v14; // eax
  __int16 v15; // bx
  __int16 v16; // bp
  int v17; // ecx
  int v18; // eax
  int v19; // edi
  int v20; // [esp+10h] [ebp-18h]
  CellStruct v21; // [esp+14h] [ebp-14h]
  __int16 v22; // [esp+18h] [ebp-10h]
  __int16 v23; // [esp+1Ah] [ebp-Eh]
  CellStruct v24; // [esp+1Ch] [ebp-Ch]
  int v25; // [esp+20h] [ebp-8h]
  int *v26; // [esp+2Ch] [ebp+4h]
  char v27; // [esp+30h] [ebp+8h]
  CellStruct v28; // [esp+34h] [ebp+Ch]

  v7 = *pAdjCell;
  v8 = a4;
  v9 = 0;
  v10 = 0;
  v11 = a3 < a4;
  v25 = 0;
  v20 = a3;
  v21 = *pAdjCell;
  v27 = 0;
  if ( v11 )
  {
LABEL_13:
    *a5 = v8;
    *pAdjCell = v7;
    return;
  }
  v26 = &a2[a3];
  while ( *v26 == -2 )
  {
    --v20;
    --v26;
LABEL_12:
    if ( v20 < v8 )
      goto LABEL_13;
  }
  v12 = ((unsigned __int8)*v26 - 4) & 7;
  X = Unsorted::AdjacentCell[v12].X;
  v14 = &Unsorted::AdjacentCell[v12];
  v22 = v9 + X;
  v23 = v10 + v14->Y;
  v15 = v21.Y + v14->Y;
  v16 = v7.X + v14->X;
  v24.X = v16;
  v24.Y = v15;
  v21.Y = v15;
  v17 = abs16(v23);
  v18 = abs16(v22);
  if ( v18 <= v17 )
    v18 = v17;
  if ( v18 <= v25 )
  {
    v27 = 1;
    goto LABEL_11;
  }
  if ( !v27 )
  {
    v25 = v18;
LABEL_11:
    v10 = v23;
    v8 = a4;
    v7 = v24;
    --v20;
    v9 = v22;
    --v26;
    goto LABEL_12;
  }
  v19 = ((((unsigned __int8)*v26 - 4) & 7) - 4) & 7;
  *a5 = v20 + 1;
  v28.X = v16 + Unsorted::AdjacentCell[v19].X;
  v28.Y = v15 + Unsorted::AdjacentCell[v19].Y;
  *pAdjCell = v28;
}

#endif

#pragma endregion

#pragma region PlotStraightLine

bool AStarClassFake::PlotStraightLine_AsVanilla
(
	int* const pDirs,
	const int maxLength,
	const CellStruct* const pCurCell,
	const CellStruct* const pVecCell,
	const FootClass* const pFoot,
	int& curLevel, // Change to reference
	const bool allowThreats
)
{
	const int vecX = pVecCell->X;
	const int vecY = pVecCell->Y;
	const int sumXY = vecY + vecX;
	int primaryDir = (vecX >= 0) ? (vecY >= 0 ? 3 : 1) : (vecY >= 0 ? 5 : 7);
	int secondaryDir = (vecX - vecY <= 0) ? (sumXY <= 0 ? 6 : 4) : (sumXY <= 0 ? 0 : 2);

	const int absX = std::abs(vecX);
	const int absY = std::abs(vecY);
	int minSteps = Math::min(absX, absY);
	int diagSteps = Math::max(absX, absY) - minSteps;

	const double threat = reinterpret_cast<double(__thiscall*)(const FootClass*)>(0x4DC760)(pFoot);
	const auto pOwner = pFoot->Owner;

	int currentLevel = curLevel;
	int phase = 0;
	bool blocked = false;

	while (true)
	{
		int firstSteps = minSteps;
		int secondSteps = diagSteps;
		int threatCount = 0;
		auto currentCell = *pCurCell;

		if (phase > 0)
		{
			std::swap(primaryDir, secondaryDir);
			std::swap(minSteps, diagSteps);
		}

		if (minSteps)
		{
			currentLevel = curLevel;

			if (minSteps > 0)
			{
				do
				{
					currentCell += Unsorted::AdjacentCell[primaryDir & 7];
					const auto pCell = MapClass::Instance.GetCellAt(currentCell);

					if (threat > 0.00001 && MapClass::Instance.GetThreatPosed(currentCell, pOwner) * threat >= 0.01)
						++threatCount;

					blocked = pFoot->IsCellOccupied(pCell, static_cast<FacingType>(primaryDir), currentLevel, 0, true) != Move::OK
						|| (pCell->Flags & CellFlags::Tube)
						|| threatCount > 3
						|| (!allowThreats && threatCount > 0);

					--firstSteps;

					const int Level = pCell->Level;
					const int upLevel = Level + 4;
					currentLevel = (currentLevel == upLevel && pCell->ContainsBridge()) ? upLevel : Level;
				}
				while (firstSteps > 0 && !blocked);
			}

			if (diagSteps > 0 && !blocked)
			{
				do
				{
					currentCell += Unsorted::AdjacentCell[secondaryDir & 7];
					const auto pCell = MapClass::Instance.GetCellAt(currentCell);

					if (threat > 0.00001 && MapClass::Instance.GetThreatPosed(currentCell, pOwner) * threat >= 0.01)
						++threatCount;

					blocked = pFoot->IsCellOccupied(pCell, static_cast<FacingType>(secondaryDir), currentLevel, 0, true) != Move::OK
						|| (pCell->Flags & CellFlags::Tube)
						|| threatCount > 3
						|| (!allowThreats && threatCount > 0);

					--secondSteps;

					const int Level = pCell->Level;
					const int upLevel = Level + 4;
					currentLevel = (currentLevel == upLevel && pCell->ContainsBridge()) ? upLevel : Level;
				}
				while (secondSteps > 0 && !blocked);
			}

			if (!blocked)
				break;
		}

		if (++phase >= 2)
			return false;
	}

	const int safeMinSteps = Math::min(minSteps, maxLength);

	if (safeMinSteps > 0)
		std::fill_n(pDirs, safeMinSteps, primaryDir);

	const int safeDiagSteps = Math::min(diagSteps, maxLength - safeMinSteps);

	if (safeDiagSteps > 0)
		std::fill_n(pDirs + safeMinSteps, safeDiagSteps, secondaryDir);

	const int totalSteps = safeMinSteps + safeDiagSteps;
	const int remainingSteps = maxLength - totalSteps;

	if (remainingSteps > 0)
		std::fill_n(pDirs + totalSteps, remainingSteps, -2);

	curLevel = currentLevel;
	return true;
}

void AStarClassFake::PlotStraightLine_Optimized
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
	constexpr int segmentMaxLength = 8;
	std::vector<int> buffer(maxLength, -2);
	auto currentCell = *pCurCell;
	auto remainingVec = *pVecCell;
	int ratio = (std::abs(pVecCell->X) + std::abs(pVecCell->Y)) / segmentMaxLength + 1;
	int remainingLength = maxLength;
	auto divideRoundUp = [ratio](short value) { return static_cast<short>((value > 0) ? (value + ratio - 1) / ratio : -((-value + ratio - 1) / ratio)); };

	do
	{
		auto segmentVec = remainingVec;

		if (ratio > 1 && remainingVec.X && remainingVec.Y)
		{
			segmentVec.X = divideRoundUp(segmentVec.X);
			segmentVec.Y = divideRoundUp(segmentVec.Y);
		}

		const int segmentLength = Math::min(remainingLength, Math::max(std::abs(segmentVec.X), std::abs(segmentVec.Y)));
		int* const pVecBuffer = buffer.data() + maxLength - remainingLength;

		if (!AStarClassFake::PlotStraightLine_AsVanilla(pVecBuffer, segmentLength, &currentCell, &segmentVec, pFoot, curLevel, allowThreats))
			return;

		currentCell += segmentVec;
		remainingVec -= segmentVec;
		remainingLength -= segmentLength;
	}
	while (remainingVec != CellStruct::Empty && remainingLength > 0);

	// Copy only when success
	std::copy(buffer.begin(), buffer.end(), pDirs);
}

#pragma endregion

#pragma region AllHooks

void __fastcall AStarClass_PlotStraightLine_Wrapper
(
	void* const pThis,
	const discard_t _,
	int* const pDirs,
	const int maxLength,
	const CellStruct* const pCurCell,
	const CellStruct* const pVecCell,
	const FootClass* const pFoot,
	const int curLevel,
	const bool allowThreats
)
{
	AStarClassFake::PlotStraightLine_Reimplement(pDirs, maxLength, pCurCell, pVecCell, pFoot, curLevel, allowThreats);
}
DEFINE_FUNCTION_JUMP(CALL, 0x42BA96, AStarClass_PlotStraightLine_Wrapper);
DEFINE_FUNCTION_JUMP(CALL, 0x42BC2E, AStarClass_PlotStraightLine_Wrapper);

DEFINE_HOOK(0x42BE20, AStarClass_PlotStraightLine_CheckOptimize, 0x7)
{
	enum { SkipGameCode = 0x42C1B1 };

	GET_STACK(int* const, pDirs, STACK_OFFSET(0x0, 0x4));
	GET_STACK(const int, maxLength, STACK_OFFSET(0x0, 0x8));
	GET_STACK(const CellStruct* const, pCurCell, STACK_OFFSET(0x0, 0xC));
	GET_STACK(const CellStruct* const, pVecCell, STACK_OFFSET(0x0, 0x10));
	GET_STACK(const FootClass* const, pFoot, STACK_OFFSET(0x0, 0x14));
	GET_STACK(const int, curLevel, STACK_OFFSET(0x0, 0x18));
	GET_STACK(const bool, allowThreats, STACK_OFFSET(0x0, 0x1C));

	AStarClassFake::PlotStraightLine_Reimplement(pDirs, maxLength, pCurCell, pVecCell, pFoot, curLevel, allowThreats);

	return SkipGameCode;
}

#pragma endregion

#endif
