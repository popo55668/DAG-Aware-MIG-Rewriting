/**CFile****************************************************************

  FileName    [abcMigRewrite.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Network and node package.]

  Synopsis    [Technology-independent resynthesis of the MIG based on DAG aware rewriting.]

  Author      [Li-Wei Wang]
  
  Affiliation [NTU]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: abcRewrite.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "base/abc/abc.h"
#include "opt/mig/mig.h"
#include "opt/mig/rewrite.h"
#include "opt/mig/graph.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static Mig_Obj_t * Mig_GraphToNetwork ( Mig_Man_t * pMig, Gra_Graph_t * pGraph );
static Mig_Obj_t * Mig_GraphUpdateNetwork( Mig_Man_t * pMig, Mig_Obj_t * pRoot, Gra_Graph_t * pGraph, int fUpdateLevel );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Direct Trans to MIG and performs incremental rewriting of the MIG.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMigRewrite( Abc_Ntk_t * pNtk, char * output )
{
    Mig_Man_t * pMan;
    Cut_Man_t * pManCut;
    Cut_Cut_t * pCut;
    Gra_Graph_t * pGraph;
    Rewrite_Man_t * pRwr;
    int i, fCompl, nGain, nNodes;
    int fUpdateLevel = 1;
    Mig_Obj_t * pNode;

    pMan = Mig_AigToMig( pNtk );
    pRwr = Rewrite_ManStartFor4Var(0);

    pManCut = Mig_MigStartCutManForRewrite( pMan );  // 1. param  2. triv cut for PI
    if( fUpdateLevel ) Mig_ManStartReverseLevels( pMan, 0 );
    nNodes = Mig_ManObjNum(pMan);

    // resynthesize each node once
    Mig_ManForEachNode( pMan, pNode, i )
    {
//        printf("Id = %d.\n", pNode->Id );
        if ( pNode->isFix ) continue;
        if ( i >= nNodes  ) break;  // stop if all nodes have been tried. those with i >= nNodes is newly created nodes
        if ( Mig_ObjFanoutNum(pNode) > 1000 ) continue;
        pCut = Mig_NodeGetCutsRecursive( pManCut, pNode );
        nGain = Rewrite_NodeRewrite( pMan, pRwr, pManCut, pNode, pCut, fUpdateLevel );
        // checking condition
        if( nGain < 0 ) continue;  

        // if we end up here, a rewriting step is accepted
        pGraph = (Gra_Graph_t *)pRwr->pGraph;
        fCompl = pRwr->fCompl;
        if(fCompl) Gra_GraphComplement( pGraph );
        Mig_GraphUpdateNetwork( pMan, pNode, pGraph, fUpdateLevel );
        if(fCompl) Gra_GraphComplement( pGraph );  // recover the graph
    }
    Mig_ManReassignIds( pMan );
    Mig_MigCleanup(pMan);  // remove dangling node from hash table
    Mig_InvertCanonical( pMan );
    printf("Rewriting Completed.\n");
//    for( i = 0; i < 222; i++ ) 
//        if(pRwr->nScores[i]) printf("class %d : %d\n",i,  pRwr->nScores[i] );
//    Mig_Print( pMan );
    Mig_Write( pNtk, pMan, output );
    return 1;
}

Mig_Obj_t * Mig_GraphToNetwork ( Mig_Man_t * pMig, Gra_Graph_t * pGraph )
{
    Mig_Obj_t * pMaj0, *pMaj1, *pMaj2;
    Gra_Node_t * pNode = NULL;
    int i;

    if( Gra_GraphIsConst(pGraph) ) 
        return Mig_NotCond( pMig->pConst1, Gra_GraphIsComplement(pGraph) );
    if( Gra_GraphIsVar(pGraph) ) 
        return Mig_NotCond( (Mig_Obj_t*)Gra_GraphVar(pGraph)->pFunc, Gra_GraphIsComplement(pGraph) );

    Gra_GraphForEachNode( pGraph, pNode, i )
    {
        pMaj0 = Mig_Regular((Mig_Obj_t*)Gra_GraphNode( pGraph, pNode->eEdge0.Node)->pFunc);
        pMaj1 = Mig_Regular((Mig_Obj_t*)Gra_GraphNode( pGraph, pNode->eEdge1.Node)->pFunc);
        pMaj2 = Mig_Regular((Mig_Obj_t*)Gra_GraphNode( pGraph, pNode->eEdge2.Node)->pFunc);
//        printf("i = %d, fanin = %d, %d, %d\n", i , pMaj0->Id, pMaj1->Id, pMaj2->Id);
        
        pMaj0 = Mig_NotCond( (Mig_Obj_t*)Gra_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc, pNode->eEdge0.fCompl );
        pMaj1 = Mig_NotCond( (Mig_Obj_t*)Gra_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc, pNode->eEdge1.fCompl );
        pMaj2 = Mig_NotCond( (Mig_Obj_t*)Gra_GraphNode(pGraph, pNode->eEdge2.Node)->pFunc, pNode->eEdge2.fCompl );
        pNode->pFunc = Mig_MigMaj( pMig, pMaj0, pMaj1, pMaj2 );
    }
    return Mig_NotCond( (Mig_Obj_t*)pNode->pFunc, Gra_GraphIsComplement(pGraph) );
}

Mig_Obj_t * Mig_GraphUpdateNetwork( Mig_Man_t * pMig, Mig_Obj_t * pRoot, Gra_Graph_t * pGraph, int fUpdateLevel )
{
    Mig_Obj_t * pRootNew = Mig_GraphToNetwork( pMig, pGraph );
//    printf("new root id = %d orig id = %d\n", Mig_Regular(pRootNew)->Id, Mig_Regular(pRoot)->Id );
    Mig_MigReplace( pMig, pRoot, pRootNew, fUpdateLevel );
    return pRootNew;
}

/**Function*************************************************************

  Synopsis    [ Rewriting with carryout node detection.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkMigRewriteWithCarryoutDetection( Abc_Ntk_t * pNtk, char * output )
{
    Mig_Man_t * pMan;
    Cut_Man_t * pManCut;
    Cut_Cut_t * pCut;
    Gra_Graph_t * pGraph;
    Vec_Ptr_t * vChain;
    Rewrite_Man_t * pRwr4, * pRwr3;
    int i, j, k, fCompl, count, nNodes, totalCount = 0;
    int nGain;
    unsigned tValue;
    Mig_Obj_t * pNode, * pNode2;

    pMan = Mig_AigToMigWithAdderDetection( pNtk );
    nNodes = Mig_ManObjNum(pMan);
printf("Number of nodes: %d\n", nNodes );

    pManCut = Mig_MigStartCutManForRewrite( pMan );  // 1. param  2. triv cut for PI
    pRwr4 = Rewrite_ManStartFor4Var(1); // choose 221-specialized subgraph


    Vec_Ptr_t * vXor = Vec_PtrAlloc(100);
    simulate(pMan);
    Vec_VecForEachLevel( pMan->vChains, vChain, i )
    {
        printf( "\n      level %d\n", i );
        Vec_PtrForEachEntry( Mig_Obj_t *, vChain, pNode, j )
        {
            printf("carry = %d\n", pNode->Id);
            Vec_PtrClear(vXor);
            tValue = Mig_Regular(pNode->pFanin0)->simValue;
            tValue ^= Mig_Regular(pNode->pFanin1)->simValue;
            tValue ^= Mig_Regular(pNode->pFanin2)->simValue;
        
            Vec_Ptr_t * vTemp = Vec_VecEntry(pMan->pSimTable, tValue % pMan->nTableSize);
            Vec_PtrForEachEntry(Mig_Obj_t*, vTemp, pNode2, k )
                if( pNode2->simValue == tValue || pNode2->simValue == ~tValue) Vec_PtrPush( vXor, pNode2 );
            vTemp = Vec_VecEntry(pMan->pSimTable, (~tValue) % pMan->nTableSize);
            Vec_PtrForEachEntry(Mig_Obj_t*, vTemp, pNode2, k )
                if( pNode2->simValue == tValue || pNode2->simValue == ~tValue) Vec_PtrPush( vXor, pNode2 );

            Vec_PtrForEachEntry(Mig_Obj_t *, vXor, pNode2, k )
            {
                if( pNode2->target ) continue;
                if( pNode2->Id > nNodes ) continue;
                printf("    sum = %d\n", pNode2->Id );
                pCut = Mig_NodeGetCutsRecursive( pManCut, pNode2 );
                count = Rewrite_NodeRewriteForSimCandidate(pMan, pRwr4, pManCut, pNode2, pNode, pCut );
                totalCount+= count;
                
                if( count == 0 ) continue;
                printf("        count = %d\n", count);
                pNode2->target = 1;
            
                // update graph
                pGraph = (Gra_Graph_t *)pRwr4->pGraph;
                fCompl = pRwr4->fCompl;
                if(fCompl) Gra_GraphComplement( pGraph );
                /*Mig_Obj_t * pRootNew = */Mig_GraphUpdateNetwork( pMan, pNode2, pGraph, 0 ); // pRootNew may be complemented
                if(fCompl) Gra_GraphComplement( pGraph );  // recover the graph
                simulate(pMan);
            }
        }
    }
    Mig_ManForEachNode( pMan, pNode, i ) pNode->target = 0;

    printf("total count = %d\n", totalCount);
    Mig_ManReassignIds( pMan );
    Mig_MigCleanup(pMan);  // remove dangling node from hash table
/*
    pRwr3 = Rewrite_ManStartFor3Var();
    pManCut = Mig_MigStartCutManForRewrite( pMan );  // 1. param  2. triv cut for PI
    nNodes = Mig_ManObjNum(pMan);
    // resynthesize each node with xor3 
    Mig_ManForEachNode( pMan, pNode, i )
    {
//        printf("Id = %d.\n", pNode->Id );
        if ( pNode->isFix ) continue;
        if ( i >= nNodes  ) break;  // stop if all nodes have been tried. those with i >= nNodes is newly created nodes
        if ( Mig_ObjFanoutNum(pNode) > 1000 ) continue;
        pCut = Mig_NodeGetCutsRecursive( pManCut, pNode );
        nGain = Rewrite_NodeRewriteFor3Var( pMan, pRwr3, pManCut, pNode, pCut );
        // checking condition
        if( nGain < 5 ) continue; 

        // if we end up here, a rewriting step is accepted
        pGraph = (Gra_Graph_t *)pRwr3->pGraph;
        fCompl = pRwr3->fCompl;
        if(fCompl) Gra_GraphComplement( pGraph );
        Mig_Obj_t * pRootNew = Mig_GraphUpdateNetwork( pMan, pNode, pGraph, 0 );
        if(fCompl) Gra_GraphComplement( pGraph );  // recover the graph
        pRootNew = Mig_Regular(pRootNew);
        pRootNew->isFix = 1;
        printf("sharing %d\n", pRootNew->Id); 
    }
    Mig_ManReassignIds( pMan );
    Mig_MigCleanup(pMan);  // remove dangling node from hash table
    printf("Rewriting Completed.\n");
   */ 
    Mig_WriteWithFadd( pNtk, pMan, output );

    return 1;
}

/**Function*************************************************************

  Synopsis    [ Read a MIG netlist and perform rewriting.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkReadMigAndRewrite( char * name, char * des, int gain, Abc_Ntk_t * pNtk )
{
    Mig_Man_t * pMan;
    Cut_Man_t * pManCut;
    Cut_Cut_t * pCut;
    Gra_Graph_t * pGraph;
    Rewrite_Man_t * pRwr;
    int i, fCompl, nGain, nNodes;
    int fUpdateLevel = 1;
    Mig_Obj_t * pNode;
    abctime timeLoad, timeRewrite, timeTotal;
    abctime clk, clkStart = Abc_Clock();

    pMan = Mig_ReadMig( name, pNtk );

    pRwr = Rewrite_ManStartFor4Var(0);
timeLoad = Abc_Clock() - clkStart;
    nNodes = Mig_ManObjNum(pMan);
    pManCut = Mig_MigStartCutManForRewrite( pMan );  // 1. param  2. triv cut for PI
    if( fUpdateLevel ) Mig_ManStartReverseLevels( pMan, 0 );
clk = Abc_Clock();
    // resynthesize each node once
    Mig_ManForEachNode( pMan, pNode, i )
    {
//printf("Id = %d. Level = %d LevelR = %d\n", pNode->Id, pNode->Level, Mig_ObjReverseLevel(pMan,pNode) );
        if ( i >= nNodes  ) break;  // stop if all nodes have been tried. those with i >= nNodes is newly created nodes
        if ( Mig_ObjFanoutNum(pNode) > 1000 ) continue;
        pCut = Mig_NodeGetCutsRecursive( pManCut, pNode );
        nGain = Rewrite_NodeRewrite( pMan, pRwr, pManCut, pNode, pCut, fUpdateLevel );
        // checking condition
        if( nGain < gain ) continue;  

        // if we end up here, a rewriting step is accepted
        pGraph = (Gra_Graph_t *)pRwr->pGraph;
printf("A cut is accepted. graph root id = %d. nLeaves = %d\n", pGraph->eRoot.Node, pGraph->nLeaves );
        fCompl = pRwr->fCompl;
        if(fCompl) Gra_GraphComplement( pGraph );
        Mig_GraphUpdateNetwork( pMan, pNode, pGraph, fUpdateLevel );
        if(fCompl) Gra_GraphComplement( pGraph );  // recover the graph
    }
printf("Rewriting Completed.\n");
    Mig_ManReassignIds( pMan );
printf("Reassigning ID.\n");
    Mig_MigCleanup(pMan);  // remove dangling node from hash table
printf("Removing dangling nodes.\n");
    Mig_InvertCanonical( pMan );
printf("Inverting canonical.\n");
timeRewrite = Abc_Clock() - clk;
timeTotal = Abc_Clock() - clkStart;
    double ratio = 0;
    double nodes = 0;
    Mig_ManForEachNode( pMan, pNode, i )
    {
        nodes++;
        if( Mig_Regular(pNode->pFanin0) != pMan->pConst1 ) ratio++;
    }
    printf( "ratio = %f\n", ratio/nodes );
    ABC_PRT( " Load    ", timeLoad );
    ABC_PRT( " Rewrite ", timeRewrite );
    ABC_PRT( " TOTAL   ", timeTotal );

    Mig_Write( pNtk, pMan, des );
    return 1;
}

/*void PopoRewrite( Abc_Ntk_t * pNtk )
{
  //Rwr_Man_t * pRwr;
  //Vec_Ptr_t * pVec;
  //Rwr_Node_t * pNode;
  unsigned char * pMap;
  unsigned flag[222] = { 0 };
  unsigned nFunc = (1<<16);
  unsigned i;

  //pRwr = Rwr_ManStart(0);
  Dec_Man_t* pManDec;
  pManDec = (Dec_Man_t*) Abc_FrameReadManDec();
  pMap = pManDec->pMap;
  printf("%d\n", pManDec->puCanons[59798] );
  
  FILE * pFile = fopen( "class.txt", "w" );
  for ( i = 0; i < nFunc; i++ ) {
      if( flag[ pMap[i] ] ) continue;
      fprintf( pFile, "%d %d\n", pMap[i], i );
      printf("class: %d , tt =", pMap[i] );
      Extra_PrintHex( stdout, &i, 4 );
      printf("\n");
      flag[pMap[i]] = 1;
  }
  fclose(pFile);
  Abc_Aig_t * pAig;
  pAig = (Abc_Aig_t*)pNtk->pManFunc;
  Abc_Obj_t * pNode;
  int i;
  Abc_NtkForEachObj( pNtk, pNode, i )
      Abc_AigPrintNode( pNode );
}
*/

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

