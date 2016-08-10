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

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Cut_Man_t * Mig_MigStartCutManForRewrite( Mig_Man_t * pMan )
{
    static Cut_Params_t Params, * pParams = &Params;
    Cut_Man_t * pManCut;
    Mig_Obj_t * pObj;
    int i;
    // start the cut manager
    memset( pParams, 0, sizeof(Cut_Params_t) );
    pParams->nVarsMax = 4;
    pParams->nKeepMax = 250;
    pParams->fTruth   = 1;
    pParams->fFilter  = 1;
    pParams->fSeq     = 0;
    pParams->fDrop    = 0;
    pParams->fVerbose = 0;
    pParams->nIdsMax  = Mig_ManObjNum( pMan );
    pManCut = Cut_ManStart( pParams );

    Mig_ManForEachPi( pMan, pObj, i )
        if ( Mig_ObjFanoutNum(pObj) > 0 )
            Cut_NodeSetTriv( pManCut, pObj->Id );

    return pManCut;
}

static inline int Mig_CutCheckDominance( Cut_Cut_t * pDom, Cut_Cut_t * pCut )
{
    int i, k;
    for( i = 0; i < (int)pDom->nLeaves; i++ )
    {
        for( k = 0; k < (int)pCut->nLeaves; k++ )
            if( pDom->pLeaves[i] == pCut->pLeaves[k] ) break;
        if ( k == (int)pCut->nLeaves) return 0; // node i in pDom isn't contained in pCut
    }
    return 1;
}

static inline int Mig_CutFilterOne( Cut_Man_t * p, Cut_List_t * pSuperList, Cut_Cut_t * pCut )
{
    Cut_Cut_t * pTemp, *pTemp2, **ppTail;
    int a;

    // check if this cut is filtered out by smaller cuts
    for ( a = 2; a <= (int)pCut->nLeaves; a++ )
    {
        Cut_ListForEachCut( pSuperList->pHead[a], pTemp )
        {
            if( (pTemp->uSign & pCut->uSign) != pTemp->uSign ) continue;
            if( Mig_CutCheckDominance( pTemp, pCut ) )
            {
                p->nCutsFilter++;
                Cut_CutRecycle( p, pCut );
                return 1;
            }
        }
    }
    // filter out other cuts using this one
    for ( a = pCut->nLeaves +1; a <= (int)pCut->nVarsMax; a++ )
    {
        ppTail = pSuperList->pHead + a;
        Cut_ListForEachCutSafe( pSuperList->pHead[a], pTemp, pTemp2 )
        {
            if( (pTemp->uSign & pCut->uSign ) != pCut->uSign )
            {
                ppTail = &pTemp->pNext;
                continue;
            }
            if( Mig_CutCheckDominance( pCut, pTemp ) )
            {
                p->nCutsFilter++;
                p->nNodeCuts--;
                // move the head
                if( pSuperList->pHead[a] == pTemp ) pSuperList->pHead[a] = pTemp->pNext;
                // move the tail
                if( pSuperList->ppTail[a] == &pTemp->pNext ) pSuperList->ppTail[a] = ppTail;
                *ppTail = pTemp->pNext;
                Cut_CutRecycle( p, pTemp );
            }
            else
                ppTail = &pTemp->pNext;
        }
        assert( ppTail == pSuperList->ppTail[a] );
        assert( *ppTail == NULL );
    }
    return 0;
}
static inline unsigned Cut_TruthPhase( Cut_Cut_t * pCut, Cut_Cut_t * pCut1 )
{
    unsigned uPhase = 0;
    int i, k;
    for ( i = k = 0; i < (int)pCut->nLeaves; i++ )
    {
        if ( k == (int)pCut1->nLeaves ) break;
        if ( pCut->pLeaves[i] < pCut1->pLeaves[k] ) continue;
        assert( pCut->pLeaves[i] == pCut1->pLeaves[k] );
        uPhase |= (1 << i );
        k++;
    }
    return uPhase;
}

void Mig_TruthComputeTwo( Cut_Man_t * p, Cut_Cut_t* pCut, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1 )
{
    // fCompl0 = 1 means it is an AND gate, otherwise OR gate
    if( p->fCompl1 )
        Extra_TruthNot( p->puTemp[0], Cut_CutReadTruth(pCut0), pCut->nVarsMax );
    else
        Extra_TruthCopy( p->puTemp[0], Cut_CutReadTruth(pCut0), pCut->nVarsMax );
    Extra_TruthStretch( p->puTemp[2], p->puTemp[0], pCut0->nLeaves, pCut->nVarsMax, Cut_TruthPhase( pCut, pCut0) );

    if( p->fSimul )
        Extra_TruthNot( p->puTemp[1], Cut_CutReadTruth(pCut1), pCut->nVarsMax );
    else
        Extra_TruthCopy( p->puTemp[1], Cut_CutReadTruth(pCut1), pCut->nVarsMax );
    Extra_TruthStretch( p->puTemp[3], p->puTemp[1], pCut1->nLeaves, pCut->nVarsMax, Cut_TruthPhase( pCut, pCut1) );

    if( p->fCompl0 )   // AND gate
        Extra_TruthAnd( Cut_CutReadTruth(pCut), p->puTemp[2], p->puTemp[3], pCut->nVarsMax );
    
    else               // OR gate
        Extra_TruthOr( Cut_CutReadTruth(pCut), p->puTemp[2], p->puTemp[3], pCut->nVarsMax );
    
}

static inline int Mig_CutProcessTwo( Cut_Man_t * p, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1, Cut_List_t * pSuperList )
{
    Cut_Cut_t * pCut, *pTemp = 0;
    // from large to small
    if( pCut0->nLeaves < pCut1->nLeaves )
        pTemp = pCut0, pCut0 = pCut1, pCut1 = pTemp;
    assert( pCut0->nLeaves >= pCut1->nLeaves );

    // merge
    pCut = Cut_CutMergeTwo( p, pCut0, pCut1 );  // reuse function
    if( pTemp )  // recover the exchange
        pTemp = pCut0, pCut0 = pCut1, pCut1 = pTemp;
    if( pCut == NULL ) return 0;
    // signature
    pCut->uSign = pCut0->uSign | pCut1->uSign;
    // check dominance
    if( Mig_CutFilterOne(p, pSuperList, pCut ) ) return 0; // reuse function
    // truth table
    Mig_TruthComputeTwo( p, pCut, pCut0, pCut1 );
    Cut_ListAdd( pSuperList, pCut );
    return ++p->nNodeCuts == p->pParams->nKeepMax;
}

void Mig_NodeDoComputeTwoCuts( Cut_Man_t * p, Cut_List_t * pSuper, Mig_Obj_t * pObj, Cut_Cut_t * pList0, Cut_Cut_t * pList1 )
{
    Cut_Cut_t * pTemp0, *pTemp1;
    Cut_Cut_t * pStop0, *pStop1;
    int i, nCutsOld, Limit;

    pTemp0 = Cut_CutCreateTriv( p, Mig_ObjId(pObj) );
    Cut_ListAdd( pSuper, pTemp0 );
    p->nNodeCuts++;

    if( pList0 == NULL || pList1 == NULL ) return;

    nCutsOld = p->nCutsCur;
    Limit = p->pParams->nVarsMax;

    p->fCompl0 = Mig_IsComplement(pObj->pFanin0);  // Regular(pFanin0) is const1
    p->fCompl1 = Mig_IsComplement(pObj->pFanin1);
    p->fSimul = Mig_IsComplement(pObj->pFanin2);

    // find the point in the list where the max-var cuts begin
    Cut_ListForEachCut( pList0, pStop0 )
        if( pStop0->nLeaves == (unsigned)Limit ) break;
    Cut_ListForEachCut( pList1, pStop1 )
        if( pStop1->nLeaves == (unsigned)Limit ) break;

    // small/small
    Cut_ListForEachCutStop( pList0, pTemp0, pStop0 )
    Cut_ListForEachCutStop( pList1, pTemp1, pStop1 )
        if ( Mig_CutProcessTwo( p, pTemp0, pTemp1, pSuper ) ) return;
    // small/large
    Cut_ListForEachCutStop( pList0, pTemp0, pStop0 )
    Cut_ListForEachCut( pStop1, pTemp1 )
    {
        if( (pTemp0->uSign & pTemp1->uSign) != pTemp0->uSign ) continue;
        if ( Mig_CutProcessTwo( p, pTemp0, pTemp1, pSuper ) ) return;
    }
    // large/small
    Cut_ListForEachCut( pStop0, pTemp0 )
    Cut_ListForEachCutStop( pList1, pTemp1, pStop1 )
    {
        if( (pTemp0->uSign & pTemp1->uSign) != pTemp1->uSign ) continue;
        if ( Mig_CutProcessTwo( p, pTemp0, pTemp1, pSuper ) ) return;
    }
    // large/large
    Cut_ListForEachCut( pStop0, pTemp0 )
    Cut_ListForEachCut( pStop1, pTemp1 )
    {
        assert( pTemp0->nLeaves == (unsigned)Limit );
        assert( pTemp1->nLeaves == (unsigned)Limit );
        if( pTemp0->uSign != pTemp1->uSign ) continue;
        for( i = 0; i < Limit; i++ )
            if( pTemp0->pLeaves[i] != pTemp1->pLeaves[i] ) break;
        if( i < Limit ) continue;
        if ( Mig_CutProcessTwo( p, pTemp0, pTemp1, pSuper ) ) return;
    }
    if( p->nNodeCuts == 0 ) p->nNodesNoCuts++;
}

void Mig_TruthComputeThree( Cut_Man_t * p, Cut_Cut_t * pCut, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1, Cut_Cut_t * pCut2 )
{
    if( p->fCompl0 )
        Extra_TruthNot( p->puTemp[0], Cut_CutReadTruth(pCut0), pCut->nVarsMax );
    else
        Extra_TruthCopy( p->puTemp[0], Cut_CutReadTruth(pCut0), pCut->nVarsMax );
    Extra_TruthStretch( p->puTemp[2], p->puTemp[0], pCut0->nLeaves, pCut->nVarsMax, Cut_TruthPhase( pCut, pCut0) );
/*
    int i;
    printf("Cut0: ");
    for( i = 0; i < (int)pCut0->nLeaves; i++ ) printf(" %d", pCut0->pLeaves[i] );
    printf("\n");
    Extra_PrintBinary( stdout, p->puTemp[2], 16 );
    printf("\n");
*/
    if( p->fCompl1 )
        Extra_TruthNot( p->puTemp[1], Cut_CutReadTruth(pCut1), pCut->nVarsMax );
    else
        Extra_TruthCopy( p->puTemp[1], Cut_CutReadTruth(pCut1), pCut->nVarsMax );
    Extra_TruthStretch( p->puTemp[3], p->puTemp[1], pCut1->nLeaves, pCut->nVarsMax, Cut_TruthPhase( pCut, pCut1) );
/*  
    printf("Cut1: ");
    for( i = 0; i < (int)pCut1->nLeaves; i++ ) printf(" %d", pCut1->pLeaves[i] );
    printf("\n");
    Extra_PrintBinary( stdout, p->puTemp[3], 16 );
    printf("\n");
*/
    if( p->fSimul )
        Extra_TruthNot( p->puTemp[0], Cut_CutReadTruth(pCut2), pCut->nVarsMax );
    else
        Extra_TruthCopy( p->puTemp[0], Cut_CutReadTruth(pCut2), pCut->nVarsMax );
    Extra_TruthStretch( p->puTemp[1], p->puTemp[0], pCut2->nLeaves, pCut->nVarsMax, Cut_TruthPhase( pCut, pCut2) );
/*  
    printf("Cut2: ");
    for( i = 0; i < (int)pCut2->nLeaves; i++ ) printf(" %d", pCut2->pLeaves[i] );
    printf("\n");
    Extra_PrintBinary( stdout, p->puTemp[1], 16 );
    printf("\n\n");
*/
    assert( Extra_TruthWordNum(pCut->nVarsMax) == 1 );
    unsigned * uTruth = Cut_CutReadTruth(pCut);
    uTruth[0] = (p->puTemp[1][0] & p->puTemp[2][0]) | (p->puTemp[1][0] & p->puTemp[3][0] ) | (p->puTemp[2][0] & p->puTemp[3][0] );
}

static inline int Mig_CutProcessThree( Cut_Man_t * p, Cut_Cut_t * pCut0, Cut_Cut_t * pCut1, Cut_Cut_t * pCut2, Cut_List_t * pSuperList )
{
    // Warning: cuts are exchanged , may causing bugs
    Cut_Cut_t * pCut, *pTemp = 0;
    unsigned uSign;

    uSign = pCut0->uSign | pCut1->uSign | pCut2->uSign;
    // 1st merge
    if( pCut0->nLeaves < pCut1->nLeaves )
        pTemp = pCut0, pCut0 = pCut1, pCut1 = pTemp;
    assert( pCut0->nLeaves >= pCut1->nLeaves );
    pCut = Cut_CutMergeTwo( p, pCut0, pCut1 );
    if( pCut == NULL ) return 0;
    if( pTemp )   // recover the exchange
        pTemp = pCut0, pCut0 = pCut1, pCut1 = pTemp, pTemp = 0;

    // 2nd merge-> pTemp points to original pCut2 since truth table computation
    if( pCut->nLeaves < pCut2->nLeaves ) 
        pTemp = pCut, pCut = pCut2, pCut2 = pTemp;
    assert( pCut->nLeaves >= pCut2->nLeaves );
    pTemp = pTemp ? pCut : pCut2;  // point ot original pCut2
    pCut = Cut_CutMergeTwo( p, pCut, pCut2 );
    if( pCut == NULL ) return 0;

    // signature
    pCut->uSign = uSign;
    // check dominance
    if( Mig_CutFilterOne(p, pSuperList, pCut ) ) return 0; //reuse function
    // truth table
    Mig_TruthComputeThree( p, pCut, pCut0, pCut1, pTemp ); // NOTE: fCompl is not exchange, so the cut order should remain the same
    Cut_ListAdd( pSuperList, pCut );
    return ++p->nNodeCuts == p->pParams->nKeepMax;
}

void Mig_NodeDoComputeThreeCuts( Cut_Man_t * p, Cut_List_t * pSuper, Mig_Obj_t * pObj, Cut_Cut_t * pList0, Cut_Cut_t * pList1, Cut_Cut_t * pList2 )
{
    Cut_Cut_t * pTemp0, *pTemp1, *pTemp2;
    Cut_Cut_t * pStop0, *pStop1, *pStop2;
    int i, nCutsOld, Limit;

    pTemp0 = Cut_CutCreateTriv( p, Mig_ObjId(pObj) );
    Cut_ListAdd( pSuper, pTemp0 );
    p->nNodeCuts++;

    if( pList0 == NULL || pList1 == NULL || pList2 == NULL ) return;

    nCutsOld = p->nCutsCur;
    Limit = p->pParams->nVarsMax;

    // complement setting : used for truth computation
    p->fCompl0 = Mig_IsComplement(pObj->pFanin0);
    p->fCompl1 = Mig_IsComplement(pObj->pFanin1);
    p->fSimul = Mig_IsComplement(pObj->pFanin2);    // fSimul is stored as compl2

    // find the point in the list where the max-var cuts begin
    Cut_ListForEachCut( pList0, pStop0 )
        if( pStop0->nLeaves == (unsigned)Limit ) break;
    Cut_ListForEachCut( pList1, pStop1 )
        if( pStop1->nLeaves == (unsigned)Limit ) break;
    Cut_ListForEachCut( pList2, pStop2 )
        if( pStop2->nLeaves == (unsigned)Limit ) break;
  
    // small/small/small
    Cut_ListForEachCutStop( pList0, pTemp0, pStop0 )
    Cut_ListForEachCutStop( pList1, pTemp1, pStop1 )
    Cut_ListForEachCutStop( pList2, pTemp2, pStop2 )
        if ( Mig_CutProcessThree( p, pTemp0, pTemp1, pTemp2, pSuper ) ) 
            return;
    
    // small/small/large
    Cut_ListForEachCutStop( pList0, pTemp0, pStop0 )
    Cut_ListForEachCutStop( pList1, pTemp1, pStop1 )
    Cut_ListForEachCut( pStop2, pTemp2 )
    {
        unsigned uSign = pTemp0->uSign | pTemp1->uSign;
        if( (uSign & pTemp2->uSign) != uSign ) continue;
        if ( Mig_CutProcessThree( p, pTemp0, pTemp1, pTemp2, pSuper ) ) 
            return;
    }
    // small/large/small
    Cut_ListForEachCutStop( pList0, pTemp0, pStop0 )
    Cut_ListForEachCut( pStop1, pTemp1 )
    Cut_ListForEachCutStop( pList2, pTemp2, pStop2 )
    {
        unsigned uSign = pTemp0->uSign | pTemp2->uSign;
        if( (uSign & pTemp1->uSign) != uSign ) continue;
        if ( Mig_CutProcessThree( p, pTemp0, pTemp1, pTemp2, pSuper ) ) 
            return;
    }
    // large/small/small
    Cut_ListForEachCut( pStop0, pTemp0 )
    Cut_ListForEachCutStop( pList1, pTemp1, pStop1 )
    Cut_ListForEachCutStop( pList2, pTemp2, pStop2 )
    {
        unsigned uSign = pTemp1->uSign | pTemp2->uSign;
        if( (uSign & pTemp0->uSign) != uSign ) continue;
        if ( Mig_CutProcessThree( p, pTemp0, pTemp1, pTemp2, pSuper ) ) 
            return;
    }
    // small/large/large
    Cut_ListForEachCutStop( pList0, pTemp0, pStop0 )
    Cut_ListForEachCut( pStop1, pTemp1 )
    Cut_ListForEachCut( pStop2, pTemp2 )
    {
        assert( pTemp1->nLeaves == (unsigned)Limit );
        assert( pTemp2->nLeaves == (unsigned)Limit );
        if( pTemp1->uSign != pTemp2->uSign ) continue;
        if( (pTemp0->uSign & pTemp1->uSign) != pTemp0->uSign ) continue;
        for ( i = 0; i < Limit; i++ )
            if ( pTemp1->pLeaves[i] != pTemp2->pLeaves[i] ) break;
        if ( i < Limit ) continue;
        if ( Mig_CutProcessThree( p, pTemp0, pTemp1, pTemp2, pSuper ) ) 
            return;
    }
    // large/small/large
    Cut_ListForEachCut( pStop0, pTemp0 )
    Cut_ListForEachCutStop( pList1, pTemp1, pStop1 )
    Cut_ListForEachCut( pStop2, pTemp2 )
    {
        assert( pTemp0->nLeaves == (unsigned)Limit );
        assert( pTemp2->nLeaves == (unsigned)Limit );
        if( pTemp0->uSign != pTemp2->uSign ) continue;
        if( (pTemp1->uSign & pTemp0->uSign) != pTemp1->uSign ) continue;
        for ( i = 0; i < Limit; i++ )
            if ( pTemp0->pLeaves[i] != pTemp2->pLeaves[i] ) break;
        if ( i < Limit ) continue;
        if ( Mig_CutProcessThree( p, pTemp0, pTemp1, pTemp2, pSuper ) ) 
            return;
    }
    // large/large/small
    Cut_ListForEachCut( pStop0, pTemp0 )
    Cut_ListForEachCut( pStop1, pTemp1 )
    Cut_ListForEachCutStop( pList2, pTemp2, pStop2 )
    {
        assert( pTemp1->nLeaves == (unsigned)Limit );
        assert( pTemp0->nLeaves == (unsigned)Limit );
        if( pTemp1->uSign != pTemp0->uSign ) continue;
        if( (pTemp0->uSign & pTemp2->uSign) != pTemp2->uSign ) continue;
        for ( i = 0; i < Limit; i++ )
            if ( pTemp1->pLeaves[i] != pTemp0->pLeaves[i] ) break;
        if ( i < Limit ) continue;
        if ( Mig_CutProcessThree( p, pTemp0, pTemp1, pTemp2, pSuper ) ) 
            return;
    }
    // large/large/large
    Cut_ListForEachCut( pStop0, pTemp0 )
    Cut_ListForEachCut( pStop1, pTemp1 )
    Cut_ListForEachCut( pStop2, pTemp2 )
    {
        assert( pTemp0->nLeaves == (unsigned)Limit );
        assert( pTemp1->nLeaves == (unsigned)Limit );
        assert( pTemp2->nLeaves == (unsigned)Limit );

        if( pTemp0->uSign != pTemp1->uSign ) continue;
        if( pTemp1->uSign != pTemp2->uSign ) continue;
        assert( pTemp0->uSign == pTemp1->uSign );
        assert( pTemp1->uSign == pTemp2->uSign );
        for ( i = 0; i < Limit; i++ ) {
            if ( pTemp0->pLeaves[i] != pTemp1->pLeaves[i] ) break;
            if ( pTemp0->pLeaves[i] != pTemp2->pLeaves[i] ) break;
        }
        if ( i < Limit ) continue;
        if ( Mig_CutProcessThree( p, pTemp0, pTemp1, pTemp2, pSuper ) ) 
            return;
    }
    if( p->nNodeCuts == 0 ) p->nNodesNoCuts++;
}

Cut_Cut_t * Mig_NodeGetCuts( Cut_Man_t * p, Mig_Obj_t * pObj )
{
    Cut_List_t Super, * pSuper = &Super;
    Cut_Cut_t * pList, *pList0, *pList1, *pList2;
    // start the number of cuts at the node
//    printf("processing node %d.\n", pObj->Id );
    p->nNodes++;
    p->nNodeCuts = 0;
    Cut_ListStart( pSuper );
    pList0 = Cut_NodeReadCutsNew(p, Mig_Regular(pObj->pFanin0)->Id);
    pList1 = Cut_NodeReadCutsNew(p, Mig_Regular(pObj->pFanin1)->Id);
    pList2 = Cut_NodeReadCutsNew(p, Mig_Regular(pObj->pFanin2)->Id);
    if( Mig_Regular(pObj->pFanin0)->Id == 0 ) {
        assert( pList0 == NULL );
        Mig_NodeDoComputeTwoCuts( p, pSuper, pObj, pList1, pList2 );
    }
    else
        Mig_NodeDoComputeThreeCuts( p, pSuper, pObj, pList0, pList1, pList2 );

    pList = Cut_ListFinish( pSuper );
    if( p->nNodeCuts == p->pParams->nKeepMax ) p->nCutsLimit++;

    Vec_PtrFillExtra( p->vCutsNew, Mig_ObjId(pObj)+1, NULL );
    assert( Cut_NodeReadCutsNew(p, Mig_ObjId(pObj) ) == NULL );
    Cut_NodeWriteCutsNew( p, Mig_ObjId(pObj), pList );
    return pList;
}

Cut_Cut_t * Mig_NodeGetCutsRecursive( Cut_Man_t * p, Mig_Obj_t * pObj )
{
    Cut_Cut_t * pList;
    if( Mig_ObjId(pObj) == 0 ) return NULL;
    if( (pList = Cut_NodeReadCutsNew(p, pObj->Id)) ) return pList;
    Mig_NodeGetCutsRecursive( p, Mig_Regular(pObj->pFanin0) );
    Mig_NodeGetCutsRecursive( p, Mig_Regular(pObj->pFanin1) );
    Mig_NodeGetCutsRecursive( p, Mig_Regular(pObj->pFanin2) );

    return Mig_NodeGetCuts( p, pObj );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

