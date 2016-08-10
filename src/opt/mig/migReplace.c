/**CFile****************************************************************

  FileName    [.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName []

  Synopsis    []

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: .c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "mig.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static void Mig_MigReplace_int( Mig_Man_t * pMan, Mig_Obj_t * pOld, Mig_Obj_t * pNew, int fUpdateLevel );
static void Mig_ManUpdateLevel_int( Mig_Man_t * pMan );
static void Mig_ManUpdateLevelR_int( Mig_Man_t * pMan );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mig_MigReplace( Mig_Man_t * pMan, Mig_Obj_t * pOld, Mig_Obj_t * pNew, int fUpdateLevel )
{
    assert( Vec_PtrSize(pMan->vStackReplaceOld) == 0 );
    assert( Vec_PtrSize(pMan->vStackReplaceNew) == 0 );
    Vec_PtrPush( pMan->vStackReplaceOld, pOld );
    Vec_PtrPush( pMan->vStackReplaceNew, pNew );
    assert( !Mig_IsComplement(pOld) ); // pNew may have compl

    while ( Vec_PtrSize( pMan->vStackReplaceOld ) )
    {
        pOld = (Mig_Obj_t *)Vec_PtrPop( pMan->vStackReplaceOld );
        pNew = (Mig_Obj_t *)Vec_PtrPop( pMan->vStackReplaceNew );
        Mig_MigReplace_int( pMan, pOld, pNew, fUpdateLevel);
    }
    if( fUpdateLevel )
    {
        Mig_ManUpdateLevel_int( pMan );
        Mig_ManUpdateLevelR_int( pMan );
    }
}

void Mig_MigReplace_int( Mig_Man_t * pMan, Mig_Obj_t * pOld, Mig_Obj_t * pNew, int fUpdateLevel )
{
    Mig_Obj_t * pFanin0, *pFanin1, *pFanin2, *pFanout, *pFanoutNew;
    int k, fCompl;

    assert( !Mig_IsComplement(pOld) );

    Vec_PtrClear( pMan->vNodes );
    Mig_ObjForEachFanout( pMan, pOld, pFanout, k )
        Vec_PtrPush( pMan->vNodes, pFanout );
    Vec_PtrForEachEntry( Mig_Obj_t *, pMan->vNodes, pFanout, k )
    {
        if ( Mig_ObjIsPo(pFanout) )  // patch the pNew to pFanout->pFanin0
        {
            //printf("fanout is a PO\n");
            assert( !Mig_IsComplement(pFanout) );  assert( !Mig_IsComplement(pOld) );
            pFanin0 = Mig_Regular(pNew); 
            assert( pFanin0 != pOld );
            fCompl = Mig_IsComplement(pFanout->pFanin0) ^ Mig_IsComplement(pNew);
            pFanout->pFanin0 = pFanin0;
            pFanout->pFanin0 = Mig_NotCond( pFanout->pFanin0, fCompl );

            if( !Vec_IntRemove( &pOld->vFanouts, pFanout->Id ) ) assert(0); // numFanout--
            Vec_IntPush( &pFanin0->vFanouts, pFanout->Id );
            continue;
        }
        pFanin0 = pFanout->pFanin0;
        pFanin1 = pFanout->pFanin1;
        pFanin2 = pFanout->pFanin2;
        if( Mig_Regular(pFanin0) == pOld ) 
            pFanin0 = Mig_NotCond( pNew, Mig_IsComplement(pFanin0) );
        else if ( Mig_Regular(pFanin1) == pOld )
            pFanin1 = Mig_NotCond( pNew, Mig_IsComplement(pFanin1) );
        else if ( Mig_Regular(pFanin2) == pOld )
            pFanin2 = Mig_NotCond( pNew, Mig_IsComplement(pFanin2) );
        else
            assert(0);

        // checking new fanins combination
        if( (pFanoutNew = Mig_MigMajLookup(pMan, pFanin0, pFanin1, pFanin2) ) )
        {
            // such node exists (it may be a constant)
            // schedule replacement of the old fanout by the new fanout
            Vec_PtrPush( pMan->vStackReplaceOld, pFanout );
            Vec_PtrPush( pMan->vStackReplaceNew, pFanoutNew );
            continue;
        }
        // such node doesn't exist - modify the old fanout node               
        assert( Mig_Regular(pFanin0) != Mig_Regular(pFanin1) );
        assert( Mig_Regular(pFanin0) != Mig_Regular(pFanin2) );

        if( pFanout->fMarkA )
            Mig_ManRemoveFromLevelStructure( pMan->vLevels, pFanout );
        if( pFanout->fMarkB )
            Mig_ManRemoveFromLevelStructureR( pMan->vLevelsR, pMan, pFanout );

        
        // pFanout->vFanouts remains the same
        Mig_MigDelete( pMan, pFanout ); // remove pFanout from hash table
        Mig_ObjRemoveFanins( pFanout ); // numFanout-- for each fanin
        Mig_MigMajCreateFrom( pMan, pFanin0, pFanin1, pFanin2, pFanout ); // id the same

        if( fUpdateLevel )
        {
            assert( pFanout->fMarkA == 0 );
            assert( pFanout->fMarkB == 0 );
            assert( Mig_ObjIsNode(pFanout) );
            pFanout->fMarkA = 1;
            pFanout->fMarkB = 1;
            Vec_VecPush( pMan->vLevels, pFanout->Level, pFanout );
            Vec_VecPush( pMan->vLevelsR, Mig_ObjReverseLevel(pMan, pFanout), pFanout );
        }
    }
    // remove fanin cone
    if( Mig_ObjFanoutNum(pOld) == 0 ) 
        Mig_MigDeleteNode( pMan, pOld );
}

/**Function*************************************************************

  Synopsis    [ Level-related function ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mig_ManDfsReverse_rec( Mig_Man_t * pMan, Mig_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Mig_Obj_t * pFanout;
    int i;

    if( Mig_ObjIsTravIdCurrent( pMan, pNode ) ) return;
    Mig_ObjSetTravIdCurrent( pMan, pNode );

    if( Mig_ObjIsPo(pNode) ) return;
    assert( Mig_ObjIsNode(pNode) );
    Mig_ObjForEachFanout( pMan, pNode, pFanout, i )
        Mig_ManDfsReverse_rec( pMan, pFanout, vNodes );
    // add the node after the fanouts have been added
    Vec_PtrPush( vNodes, pNode );
}

// will skip PO/PI/CONST
Vec_Ptr_t * Mig_ManDfsReverse( Mig_Man_t * pMan )
{
    Vec_Ptr_t * vNodes;
    Mig_Obj_t * pObj, * pFanout;
    int i, k;

    Mig_ManIncTravId( pMan );
    vNodes = Vec_PtrAlloc( 100 );
    Mig_ManForEachPi( pMan, pObj, i )
    {
        Mig_ObjSetTravIdCurrent( pMan, pObj );
        Mig_ObjForEachFanout( pMan, pObj, pFanout, k )
            Mig_ManDfsReverse_rec( pMan, pFanout, vNodes );
    }

    return vNodes;
}

int Mig_ObjReverseLevelNew( Mig_Man_t * pMan, Mig_Obj_t * pObj )
{
    Mig_Obj_t * pFanout;
    int i, LevelCur, Level = 0;
    Mig_ObjForEachFanout( pMan, pObj, pFanout, i )
    {
        LevelCur = Mig_ObjReverseLevel( pMan, pFanout );
        Level = Abc_MaxInt( Level, LevelCur );
    }
    return Level + 1;
}

void Mig_ObjSetReverseLevel( Mig_Man_t * pMan, Mig_Obj_t * pObj, int LevelR )
{
    assert( pMan->vLevelsReverse );
    Vec_IntFillExtra( pMan->vLevelsReverse, pObj->Id + 1, 0 );
    Vec_IntWriteEntry( pMan->vLevelsReverse, pObj->Id, LevelR );
}

void Mig_ManUpdateLevel_int( Mig_Man_t * pMan )
{
    Mig_Obj_t * pNode, * pFanout;
    Vec_Ptr_t * vVec;
    int LevelNew, i, k, v;

    Vec_VecForEachLevel( pMan->vLevels, vVec, i )
    {
        if( Vec_PtrSize(vVec) == 0 ) continue;
        Vec_PtrForEachEntry( Mig_Obj_t *, vVec, pNode, k )
        {
            if( pNode == NULL ) continue;
            assert( Mig_ObjIsNode(pNode) );
            assert( (int)pNode->Level == i );
            assert( pNode->fMarkA == 1 );
            pNode->fMarkA = 0;

            Mig_ObjForEachFanout( pMan, pNode, pFanout, v )
            {
                if( Mig_ObjIsPo(pFanout) ) continue;
                LevelNew = 1 + Abc_MaxInt( Mig_Regular(pFanout->pFanin0)->Level, Abc_MaxInt( Mig_Regular(pFanout->pFanin1)->Level, Mig_Regular(pFanout->pFanin2)->Level) );
                assert( LevelNew > i );
                if( (int)pFanout->Level == LevelNew ) continue; // no change
                if( pFanout->fMarkA ) Mig_ManRemoveFromLevelStructure( pMan->vLevels, pFanout );
                pFanout->Level = LevelNew;
                assert( pFanout->fMarkA == 0);
                pFanout->fMarkA = 1;
                Vec_VecPush( pMan->vLevels, pFanout->Level, pFanout );
            }
        }
        Vec_PtrClear(vVec);
    }
}

void Mig_ManUpdateLevelR_int( Mig_Man_t * pMan )
{
    Mig_Obj_t * pNode, * pFanin, * pFanout;
    Vec_Ptr_t * vVec;
    int LevelNew, i, k, v, j;

    Vec_VecForEachLevel( pMan->vLevelsR, vVec, i )
    {
        if( Vec_PtrSize(vVec) == 0 ) continue;
        Vec_PtrForEachEntry( Mig_Obj_t *, vVec, pNode, k )
        {
            if( pNode == NULL ) continue;
            assert( Mig_ObjIsNode(pNode) );
            assert( Mig_ObjReverseLevel(pMan, pNode) == i );
            assert( pNode->fMarkB == 1 );
            pNode->fMarkB = 0;

            for( v = 0; v < 3; v++ )
            {
                if( v == 0 ) pFanin = Mig_Regular(pNode->pFanin0);
                else if ( v == 1 ) pFanin = Mig_Regular(pNode->pFanin1);
                else pFanin = Mig_Regular(pNode->pFanin2);

                if( Mig_ObjIsPi(pFanin) || Mig_ObjIsConst(pFanin) ) continue;
                LevelNew = 0;
                Mig_ObjForEachFanout( pMan, pFanin, pFanout, j )
                    if( LevelNew < Mig_ObjReverseLevel(pMan,pFanout) )
                        LevelNew = Mig_ObjReverseLevel(pMan,pFanout);
                LevelNew+=1;
                if( Mig_ObjReverseLevel(pMan,pFanin) == LevelNew ) continue; // no change
                if( pFanin->fMarkB ) 
                    Mig_ManRemoveFromLevelStructureR( pMan->vLevelsR, pMan, pFanin );
                Mig_ObjSetReverseLevel( pMan, pFanin, LevelNew );
                assert( pFanin->fMarkB == 0);
                pFanin->fMarkB = 1;
                Vec_VecPush( pMan->vLevelsR, LevelNew, pFanin );
            }
        }
        Vec_PtrClear(vVec);
    }
}

/**Function*************************************************************

  Synopsis    [ Externed level-related function]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mig_ObjRequiredLevel( Mig_Man_t * pMan, Mig_Obj_t * pObj )
{
//    return pMan->LevelMax + 1 - Mig_ObjReverseLevel( pMan, pObj );
    return pMan->LevelMax - Mig_ObjReverseLevel( pMan, pObj );
}

int Mig_ObjReverseLevel( Mig_Man_t * pMan, Mig_Obj_t * pObj )
{
    Vec_IntFillExtra( pMan->vLevelsReverse, pObj->Id + 1, 0 );
    return Vec_IntEntry( pMan->vLevelsReverse, pObj->Id );
}

void Mig_ManStartReverseLevels( Mig_Man_t * pMan, int nMaxLevelIncrease )
{
    Vec_Ptr_t * vNodes;
    Mig_Obj_t * pObj;
    int i;
//    if(pMan->vLevelsReverse) Vec_IntFree( pMan->vLevelsReverse );
    pMan->vLevelsReverse = Vec_IntAlloc( 0 );
    Vec_IntFill( pMan->vLevelsReverse, 1 + Mig_ManObjNum(pMan), 0 );
    // compute levels in reverse topological order
    vNodes = Mig_ManDfsReverse( pMan );
    Vec_PtrForEachEntry( Mig_Obj_t *, vNodes, pObj, i )
    {
        Mig_ObjSetReverseLevel( pMan, pObj, Mig_ObjReverseLevelNew(pMan, pObj) );
    }
    Vec_PtrFree(vNodes);
}

void Mig_ManRemoveFromLevelStructure( Vec_Vec_t * vStruct, Mig_Obj_t * pNode )
{
    Vec_Ptr_t * vVecTemp;
    Mig_Obj_t * pTemp;
    int i;
    assert( pNode->fMarkA );
    vVecTemp = Vec_VecEntry( vStruct, pNode->Level );
    Vec_PtrForEachEntry( Mig_Obj_t *, vVecTemp, pTemp, i )
    {
        if( pTemp != pNode ) continue;
        Vec_PtrWriteEntry( vVecTemp, i, NULL );
        break;
    }
    assert( i < Vec_PtrSize(vVecTemp) );
    pNode->fMarkA = 0;
}

void Mig_ManRemoveFromLevelStructureR( Vec_Vec_t * vStruct, Mig_Man_t * pMan, Mig_Obj_t * pNode )
{
    Vec_Ptr_t * vVecTemp;
    Mig_Obj_t * pTemp;
    int i;
    assert( pNode->fMarkB );
    vVecTemp = Vec_VecEntry( vStruct, Mig_ObjReverseLevel(pMan, pNode) );
    Vec_PtrForEachEntry( Mig_Obj_t *, vVecTemp, pTemp, i )
    {
        if( pTemp != pNode ) continue;
        Vec_PtrWriteEntry( vVecTemp, i, NULL );
        break;
    }
    assert( i < Vec_PtrSize(vVecTemp) );
    pNode->fMarkB = 0;
}


////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

