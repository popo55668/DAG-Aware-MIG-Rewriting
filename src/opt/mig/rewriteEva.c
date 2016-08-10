#include "rewrite.h"
#include "graph.h"

ABC_NAMESPACE_IMPL_START

// counting number of nodes
static Gra_Graph_t * Rewrite_CutEvaluate( Mig_Man_t * pMig, Rewrite_Man_t * p, Mig_Obj_t * pRoot, Cut_Cut_t * pCut, int NodesSaved, int * GainCur, int LevelMax, unsigned FFFF );
static int Rewrite_GraphToNetworkCount( Mig_Man_t * pMig, Mig_Obj_t * pRoot, Gra_Graph_t * pGraph, int NodeMax, int LevelMax );

static Gra_Graph_t * Rewrite_CutDetectSimValue( Mig_Man_t * pMig, Rewrite_Man_t * p, Mig_Obj_t * pRoot, Mig_Obj_t * pCandidate, Cut_Cut_t * pCut, int * pGainBest );
static int Rewrite_GraphToNetworkCountWithDetection( Mig_Man_t * pMig, Mig_Obj_t * pRoot, Mig_Obj_t * pCandidate, Gra_Graph_t * pGraph  );

/**Function*************************************************************

  Synopsis    [ Given a node, evaluate its cuts and save the best one ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rewrite_NodeRewrite( Mig_Man_t * pMig, Rewrite_Man_t * p, Cut_Man_t * pManCut, Mig_Obj_t * pNode, Cut_Cut_t * pCut, int fUpdateLevel )
{
    Gra_Graph_t * pGraph;
    Mig_Obj_t * pFanin;
    unsigned uPhase;
    unsigned uTruthBest = 0;
    unsigned uTruth;
    char * pPerm;
    int Required, nNodesSaved, nNodesSaveCur = -1;
    int i, GainCur = -1, GainBest = -1;

    Required = fUpdateLevel ? Mig_ObjRequiredLevel(pMig, pNode) : ABC_INFINITY;

    // triv cut is omitted
    for ( pCut = pCut->pNext; pCut; pCut = pCut->pNext )
    {
        if ( pCut->nLeaves < 4 ) continue;

        // NPN conditions
        uTruth = 0xFFFF & *Cut_CutReadTruth(pCut);
        pPerm = p->pPerms4[ (int)p->pPerms[uTruth] ];
        uPhase = p->pPhases[uTruth];
        // collect fanin nodes
        Vec_PtrClear( p->vFaninsCur );
        Vec_PtrFill( p->vFaninsCur, (int)pCut->nLeaves, 0 );
        for ( i = 0; i < (int)pCut->nLeaves; i++ )
        {
            pFanin = Mig_ManObj( pMig, pCut->pLeaves[(int)pPerm[i]] );
            if( pFanin == NULL ) break;
            //assert( pFanin ); // bad cut condition-> fanin may be removed
            pFanin = Mig_NotCond( pFanin, ((uPhase & (1<<i)) > 0) );
            Vec_PtrWriteEntry( p->vFaninsCur, i, pFanin );
        }
        if ( i != (int)pCut->nLeaves ) continue;  // bad cut: fanin is removed

        // 1. mark boundary 2. mark MFFC  3.recover boundary
        Vec_PtrForEachEntry( Mig_Obj_t *, p->vFaninsCur, pFanin, i )
            Mig_Regular(pFanin)->vFanouts.nSize++;
        Mig_ManIncTravId( pMig );
        nNodesSaved = Mig_NodeMffcLabelMig( pMig, pNode );
        Vec_PtrForEachEntry( Mig_Obj_t *, p->vFaninsCur, pFanin, i )
            Mig_Regular(pFanin)->vFanouts.nSize--;

        // evaluate the cut

        pGraph = Rewrite_CutEvaluate( pMig, p, pNode, pCut, nNodesSaved, &GainCur, Required, 0xFFFF );

        if( pGraph != NULL && GainBest < GainCur )
        {
            nNodesSaveCur = nNodesSaved;
            GainBest = GainCur;
            p->pGraph = pGraph;
            p->fCompl = ((uPhase & (1<<4)) > 0 );
            uTruthBest =  0xFFFF & *Cut_CutReadTruth(pCut);
            Vec_PtrClear( p->vFanins );
            Vec_PtrForEachEntry( Mig_Obj_t *, p->vFaninsCur, pFanin, i )
                Vec_PtrPush( p->vFanins, pFanin );
        }
    }
    if( GainBest == -1 ) return -1;
    // copy the leaves
    Gra_GraphNode( (Gra_Graph_t*)p->pGraph, 0 )->pFunc = (Mig_Obj_t*)pMig->pConst1;
    Vec_PtrForEachEntry( Mig_Obj_t *, p->vFanins, pFanin, i )
        Gra_GraphNode((Gra_Graph_t*)p->pGraph, i+1)->pFunc = pFanin;
    p->nScores[ p->pMap[uTruthBest] ]++;
    return GainBest;
}

static Gra_Graph_t * Rewrite_CutEvaluate( Mig_Man_t * pMig, Rewrite_Man_t * p, Mig_Obj_t * pRoot, Cut_Cut_t * pCut, int nNodesSaved, int * pGainBest, int LevelMax, unsigned FFFF )
{
    Vec_Ptr_t * vSubgraphs;
    Gra_Graph_t * pGraphBest = NULL;
    Gra_Graph_t * pGraphCur;
    Rwr_Obj_t * pNode, * pFanin;
    int nNodesAdded, GainBest, i, k;
    unsigned uTruth;
    
    uTruth = FFFF & *Cut_CutReadTruth(pCut);
    vSubgraphs = Vec_VecEntry( p->vClasses, p->pMap[uTruth] );
    //p->nSubgraphs += vSubgraphs->nSize;
    GainBest = -1;
// Vec_PtrForEachEntry( Rwr_Obj_t *, p->vFaninsCur, pFanin, k ) 
//printf( "%d ", Mig_Regular((Mig_Obj_t*)pFanin)->Level );
//printf("   LevelMax = %d\n", LevelMax );

    Vec_PtrForEachEntry( Rwr_Obj_t *, vSubgraphs, pNode, i )
    {
        pGraphCur = (Gra_Graph_t*)pNode->pNext;  
        assert(pGraphCur);
        Gra_GraphNode( pGraphCur, 0 )->pFunc = (Rwr_Obj_t*)pMig->pConst1;
        Vec_PtrForEachEntry( Rwr_Obj_t *, p->vFaninsCur, pFanin, k ) // Rwr_Obj_t?
            Gra_GraphNode( pGraphCur, k + 1 )->pFunc = pFanin;

        nNodesAdded = Rewrite_GraphToNetworkCount( pMig, pRoot, pGraphCur, nNodesSaved, LevelMax );
        if ( nNodesAdded == -1 ) continue;
        assert( nNodesSaved >= nNodesAdded );

        if( GainBest < nNodesSaved - nNodesAdded )
        {
            GainBest = nNodesSaved - nNodesAdded;
            pGraphBest = pGraphCur;
        }
    }
    if( GainBest == -1 ) return NULL;
    *pGainBest = GainBest;
    return pGraphBest;
}

int Rewrite_GraphToNetworkCount( Mig_Man_t * pMig, Mig_Obj_t * pRoot, Gra_Graph_t * pGraph, int NodeMax, int LevelMax )
{
    Gra_Node_t * pNode, *pNode0, *pNode1, *pNode2;
    Mig_Obj_t * pMaj, *pMaj0, *pMaj1, *pMaj2;
    int i, Counter;
    int LevelNew;

    if ( Gra_GraphIsConst(pGraph) || Gra_GraphIsVar(pGraph) ) return 0;

    Gra_GraphForEachLeaf( pGraph, pNode, i ) 
            pNode->Level = Mig_Regular((Mig_Obj_t *)pNode->pFunc)->Level;

    Counter = 0;
    Gra_GraphForEachNode( pGraph, pNode, i )
    {
        pNode0 = Gra_GraphNode( pGraph, pNode->eEdge0.Node );
        pNode1 = Gra_GraphNode( pGraph, pNode->eEdge1.Node );
        pNode2 = Gra_GraphNode( pGraph, pNode->eEdge2.Node );

        pMaj0 = (Mig_Obj_t *) pNode0->pFunc;
        pMaj1 = (Mig_Obj_t *) pNode1->pFunc;
        pMaj2 = (Mig_Obj_t *) pNode2->pFunc;
        if( pMaj0 && pMaj1 && pMaj2 )
        {
            pMaj0 = Mig_NotCond( pMaj0, pNode->eEdge0.fCompl );
            pMaj1 = Mig_NotCond( pMaj1, pNode->eEdge1.fCompl );
            pMaj2 = Mig_NotCond( pMaj2, pNode->eEdge2.fCompl );
            pMaj = Mig_MigMajLookup( pMig, pMaj0, pMaj1, pMaj2 );  // will sort fanins in lookup
            // return -1 if the node is the same as the original
            if( Mig_Regular(pMaj) == pRoot ) return -1;
        }
        else
            pMaj = NULL;
        // no reused node condition: 1. lookup not found 2. already in the MFFC
        if( pMaj == NULL || Mig_ObjIsTravIdCurrent( pMig, Mig_Regular(pMaj) ) )
        {
            if( ++Counter > NodeMax ) return -1;
        }
        LevelNew = 1 + Abc_MaxInt( pNode0->Level, Abc_MaxInt( pNode1->Level, pNode2->Level ) );
        if( pMaj )
        {
            if( Mig_Regular(pMaj) == pMig->pConst1 ) LevelNew = 0;
            else if ( Mig_Regular(pMaj) == Mig_Regular(pMaj0) ) LevelNew = (int) Mig_Regular(pMaj0)->Level;
            else if ( Mig_Regular(pMaj) == Mig_Regular(pMaj1) ) LevelNew = (int) Mig_Regular(pMaj1)->Level;
            else if ( Mig_Regular(pMaj) == Mig_Regular(pMaj2) ) LevelNew = (int) Mig_Regular(pMaj2)->Level;
        }
//        printf("%d %d\n", LevelNew, LevelMax );
        if( LevelNew > LevelMax ) return -1;
        pNode->pFunc = pMaj;
        pNode->Level = LevelNew;
    }
    return Counter;
}

/**Function*************************************************************

  Synopsis    [ Given a node, evaluate its cuts and save the best one ]

  Description [ 3 input subgraph ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Rewrite_NodeRewriteFor3Var( Mig_Man_t * pMig, Rewrite_Man_t * p, Cut_Man_t * pManCut, Mig_Obj_t * pNode, Cut_Cut_t * pCut )
{
    Gra_Graph_t * pGraph;
    Mig_Obj_t * pFanin;
    unsigned uPhase;
    unsigned uTruthBest = 0;
    unsigned uTruth;
    char * pPerm;
    int nNodesSaved, nNodesSaveCur = -1;
    int i, GainCur = -1, GainBest = -1;

    // triv cut is omitted
    for ( pCut = pCut->pNext; pCut; pCut = pCut->pNext )
    {
        if ( pCut->nLeaves != 3 ) continue;
        // NPN conditions
        uTruth = 0x00FF & *Cut_CutReadTruth(pCut);
        pPerm = p->pPerms4[ (int)p->pPerms[uTruth] ];
        
        if( p->puCanons[uTruth] != 0x0069 ) continue;

        uPhase = p->pPhases[uTruth];
        // collect fanin nodes
        Vec_PtrClear( p->vFaninsCur );
        Vec_PtrFill( p->vFaninsCur, (int)pCut->nLeaves, 0 );
        for ( i = 0; i < (int)pCut->nLeaves; i++ )
        {
            pFanin = Mig_ManObj( pMig, pCut->pLeaves[(int)pPerm[i]] );
            if( pFanin == NULL ) break;
            //assert( pFanin ); // bad cut condition-> fanin may be removed
            pFanin = Mig_NotCond( pFanin, ((uPhase & (1<<i)) > 0) );
            Vec_PtrWriteEntry( p->vFaninsCur, i, pFanin );
        }
        if ( i != (int)pCut->nLeaves ) continue;  // bad cut: fanin is removed

        // 1. mark boundary 2. mark MFFC  3.recover boundary
        Vec_PtrForEachEntry( Mig_Obj_t *, p->vFaninsCur, pFanin, i )
            Mig_Regular(pFanin)->vFanouts.nSize++;
        Mig_ManIncTravId( pMig );
        nNodesSaved = Mig_NodeMffcLabelMig( pMig, pNode );
        Vec_PtrForEachEntry( Mig_Obj_t *, p->vFaninsCur, pFanin, i )
            Mig_Regular(pFanin)->vFanouts.nSize--;

        //Cut_CutPrint(pCut, 0); 
        //printf("  nodeSaved = %d\n", nNodesSaved );
        // evaluate the cut
        pGraph = Rewrite_CutEvaluate( pMig, p, pNode, pCut, nNodesSaved, &GainCur, ABC_INFINITY, 0x00FF );

        if( pGraph != NULL && GainBest < GainCur )
        {
            nNodesSaveCur = nNodesSaved;
            GainBest = GainCur;
            p->pGraph = pGraph;
            p->fCompl = ((uPhase & (1<<3)) > 0 );
            uTruthBest =  0x00FF & *Cut_CutReadTruth(pCut);
            Vec_PtrClear( p->vFanins );
            Vec_PtrForEachEntry( Mig_Obj_t *, p->vFaninsCur, pFanin, i )
                Vec_PtrPush( p->vFanins, pFanin );
        }
    }
    if( GainBest == -1 ) return -1;
    // copy the leaves
    Gra_GraphNode( (Gra_Graph_t*)p->pGraph, 0 )->pFunc = (Mig_Obj_t*)pMig->pConst1;
    Vec_PtrForEachEntry( Mig_Obj_t *, p->vFanins, pFanin, i )
        Gra_GraphNode((Gra_Graph_t*)p->pGraph, i+1)->pFunc = pFanin;

    return GainBest; 
}

/**Function*************************************************************

  Synopsis    [ Given a node(pNode), candidate, detect a subgraph containing carry(pCandidate) fanins' sim value ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
// For Each Cut
int Rewrite_NodeRewriteForSimCandidate( Mig_Man_t * pMig, Rewrite_Man_t * p, Cut_Man_t * pCutMan, Mig_Obj_t * pNode, Mig_Obj_t * pCandidate, Cut_Cut_t * pCut )
{
    Gra_Graph_t * pGraph = NULL;
    Mig_Obj_t * pFanin;
    unsigned uPhase;
    unsigned uTruthBest = 0;
    unsigned uTruth;
    char * pPerm;
    int i, CountCur = 0, CountBest = 0;

    // triv cut is omitted
    for ( pCut = pCut->pNext; pCut; pCut = pCut->pNext )
    {
        if ( pCut->nLeaves < 4 ) continue;
        uTruth = 0xFFFF & *Cut_CutReadTruth(pCut);

        if( p->puCanons[uTruth] != 0x6996 ) continue;
//        Cut_CutPrint( pCut, 0 );  printf("\n");
        
        pPerm = p->pPerms4[ (int)p->pPerms[uTruth] ];
        uPhase = p->pPhases[uTruth];
        // collect fanin nodes
        Vec_PtrClear( p->vFaninsCur );
        Vec_PtrFill( p->vFaninsCur, (int)pCut->nLeaves, 0 );
        for ( i = 0; i < (int)pCut->nLeaves; i++ )
        {
            pFanin = Mig_ManObj( pMig, pCut->pLeaves[(int)pPerm[i]] );
            if( pFanin == NULL ) break;
            //assert( pFanin ); // bad cut condition-> fanin may be removed
            pFanin = Mig_NotCond( pFanin, ((uPhase & (1<<i)) > 0) );
            Vec_PtrWriteEntry( p->vFaninsCur, i, pFanin );
        }
        if ( i != (int)pCut->nLeaves ) continue;  // bad cut: fanin is removed

        // detect the cut
        pGraph = Rewrite_CutDetectSimValue( pMig, p, pNode, pCandidate, pCut, &CountCur );

        if( pGraph != NULL && CountBest < CountCur)
        {
            CountBest = CountCur;
            p->pGraph = pGraph;
            p->fCompl = ((uPhase & (1<<4)) > 0 );
            uTruthBest =  0xFFFF & *Cut_CutReadTruth(pCut);
            Vec_PtrClear( p->vFanins );
            Vec_PtrForEachEntry( Mig_Obj_t *, p->vFaninsCur, pFanin, i )
                Vec_PtrPush( p->vFanins, pFanin );
        }
    }
    if( CountBest == 0 ) return 0;
    // copy the leaves
    Gra_GraphNode( (Gra_Graph_t*)p->pGraph, 0 )->pFunc = (Mig_Obj_t*)pMig->pConst1;
    Vec_PtrForEachEntry( Mig_Obj_t *, p->vFanins, pFanin, i )
        Gra_GraphNode((Gra_Graph_t*)p->pGraph, i+1)->pFunc = pFanin;
    p->nScores[ p->pMap[uTruthBest] ]++;

    return CountBest;
}

// For Each Graph
static Gra_Graph_t * Rewrite_CutDetectSimValue( Mig_Man_t * pMig, Rewrite_Man_t * p, Mig_Obj_t * pRoot, Mig_Obj_t * pCandidate, Cut_Cut_t * pCut, int * pGainBest )
{
    Vec_Ptr_t * vSubgraphs;
    Gra_Graph_t * pGraphBest = NULL;
    Gra_Graph_t * pGraphCur;
    Rwr_Obj_t * pNode, * pFanin;
    int count, countBest, i, k;
    unsigned uTruth;
    
    uTruth = 0xFFFF & *Cut_CutReadTruth(pCut);
    vSubgraphs = Vec_VecEntry( p->vClasses, p->pMap[uTruth] );
    //printf("   class = %d\n", p->pMap[uTruth] );
    //p->nSubgraphs += vSubgraphs->nSize;
    
    countBest = 0;
    Vec_PtrForEachEntry( Rwr_Obj_t *, vSubgraphs, pNode, i )
    {
        pGraphCur = (Gra_Graph_t*)pNode->pNext;
        Gra_GraphNode( pGraphCur, 0 )->pFunc = (Rwr_Obj_t*)pMig->pConst1;
        Vec_PtrForEachEntry( Rwr_Obj_t *, p->vFaninsCur, pFanin, k ) 
            Gra_GraphNode( pGraphCur, k + 1 )->pFunc = pFanin;
//printf("Target:\n");
//Extra_PrintBinary( stdout, &Mig_Regular(pCandidate->pFanin0)->simValue, 32 ); printf("\n");
//Extra_PrintBinary( stdout, &Mig_Regular(pCandidate->pFanin1)->simValue, 32 ); printf("\n");
//Extra_PrintBinary( stdout, &Mig_Regular(pCandidate->pFanin2)->simValue, 32 ); printf("\n\n");
        count = Rewrite_GraphToNetworkCountWithDetection( pMig, pRoot, pCandidate, pGraphCur );
//printf("\n");        
        if( count == 0 ) continue;
        if( count > countBest ) 
        {
            countBest = count;
            pGraphBest = pGraphCur;
        }
    }
    if( countBest == 0 ) return NULL;
    *pGainBest = countBest;
    return pGraphBest;
}

static int Rewrite_GraphToNetworkCountWithDetection( Mig_Man_t * pMig, Mig_Obj_t * pRoot, Mig_Obj_t * pCandidate, Gra_Graph_t * pGraph  )
{
    Gra_Node_t * pNode, *pNode0, *pNode1, *pNode2;
    int i, count = 0;
    unsigned sim0, sim1, sim2, rootSim = 0;
    unsigned can0 = Mig_Regular(pCandidate->pFanin0)->simValue;
    unsigned can1 = Mig_Regular(pCandidate->pFanin1)->simValue;
    unsigned can2 = Mig_Regular(pCandidate->pFanin2)->simValue;

    if ( Gra_GraphIsConst(pGraph) || Gra_GraphIsVar(pGraph) ) return 0;

    Gra_GraphForEachLeaf( pGraph, pNode, i )
    {
        pNode->simValue = Mig_Regular((Mig_Obj_t *)pNode->pFunc)->simValue;
        if( Mig_IsComplement((Mig_Obj_t*)pNode->pFunc) ) pNode->simValue = ~pNode->simValue;
    }

    // pNode is a graph node
    Gra_GraphForEachNode( pGraph, pNode, i )
    {
        pNode0 = Gra_GraphNode( pGraph, pNode->eEdge0.Node );
        pNode1 = Gra_GraphNode( pGraph, pNode->eEdge1.Node );
        pNode2 = Gra_GraphNode( pGraph, pNode->eEdge2.Node );

        sim0 =  pNode0->simValue; if(pNode->eEdge0.fCompl) sim0 = ~sim0; 
        sim1 =  pNode1->simValue; if(pNode->eEdge1.fCompl) sim1 = ~sim1; 
        sim2 =  pNode2->simValue; if(pNode->eEdge2.fCompl) sim2 = ~sim2; 
        pNode->simValue = (sim0 & sim1) | (sim0 & sim2) | (sim1 & sim2);
        rootSim = pNode->simValue;
//Extra_PrintBinary( stdout, &pNode->simValue, 32 ); printf("\n");

        if( pNode->simValue == can0 || pNode->simValue == ~can0 ) count++;
        if( pNode->simValue == can1 || pNode->simValue == ~can1 ) count++;
        if( pNode->simValue == can2 || pNode->simValue == ~can2 ) count++;
    }
    //assert( pRoot->simValue == rootSim || pRoot->simValue == ~rootSim );
    return count;
}
