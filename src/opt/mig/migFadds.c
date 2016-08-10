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
#include "misc/vec/vecWec.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
#define ForEachCut( pList, pCut, i ) for ( i = 0, pCut = pList + 1; i < pList[0]; i++, pCut+= pCut[0] + 1 )
#define ForEachFadd( vFadds, i ) for ( i = 0; i < Vec_IntSize(vFadds)/7; i++ )

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int CutMergeOne( int* pCut0, int* pCut1, int* pCut )
{
    int i, k;
    for( k = 0; k <= pCut1[0]; k++ ) pCut[k] = pCut1[k];
    for( i = 1; i <= pCut0[0]; i++ )
    {
        for( k = 1; k <= pCut1[0]; k++ )
            if( pCut0[i] == pCut1[k] )break;
        if( k <= pCut1[0] ) continue; // same support found
        if( pCut[0] == 3 ) return 0;  // same support not found but size = 3
        pCut[1+pCut[0]++] = pCut0[i];
    }
    assert( pCut[0] == 2 || pCut[0] == 3 );
    if( pCut[1] > pCut[2] ) ABC_SWAP( int, pCut[1], pCut[2] );
    assert( pCut[1] < pCut[2] );
    if( pCut[0] == 2 ) return 1;
    if( pCut[2] > pCut[3] ) ABC_SWAP( int, pCut[2], pCut[3] );
    if( pCut[1] > pCut[2] ) ABC_SWAP( int, pCut[1], pCut[2] );
    assert( pCut[1] < pCut[2] );
    assert( pCut[2] < pCut[3] );
    return 1;
}

int CutCheckEqual( Vec_Int_t* vCuts, int* pCutNew )
{
    int* pList = Vec_IntArray(vCuts);
    int i, k, *pCut;
    ForEachCut( pList, pCut, i )
    {
        for( k = 0; k <= pCut[0]; k++ )
            if( pCut[k] != pCutNew[k] ) break;
        if( k > pCut[0] ) return 1;
    }
    return 0;
}

int ComputeTruth_rec( Abc_Obj_t * pObj )
{
    int t0, t1;
    if( pObj->iData ) return pObj->iData;
    assert( Abc_ObjIsNode(pObj) );
    t0 = ComputeTruth_rec( Abc_ObjFanin0(pObj) );
    t1 = ComputeTruth_rec( Abc_ObjFanin1(pObj) );
    return (pObj->iData = (Abc_ObjFaninC0(pObj) ? ~t0 : t0 ) & (Abc_ObjFaninC1(pObj) ? ~t1 : t1));
}

void CleanTruth_rec( Abc_Obj_t * pObj )
{
    if(!pObj->iData ) return;
    pObj->iData = 0;
    if( !Abc_ObjIsNode(pObj) ) return;
    CleanTruth_rec( Abc_ObjFanin0(pObj) );
    CleanTruth_rec( Abc_ObjFanin1(pObj) );
}

int ComputeTruth( Abc_Ntk_t * p, int iObj, int* pCut, int* pTruth )
{
    int i;
    unsigned t, tt[3] = { 0xAA, 0xCC, 0xF0 };
    for( i = 1; i <= pCut[0]; i++ )
        Abc_NtkObj( p, pCut[i] )->iData = tt[i-1];
    t = 0xFF & ComputeTruth_rec( Abc_NtkObj( p, iObj ) );
    CleanTruth_rec( Abc_NtkObj( p, iObj ) );
    if( pTruth ) *pTruth = t;
    if( t == 0x96 || t == 0x69 ) return 1;
    if( t == 0xE8 || t == 0xD4 || t == 0xB2 || t == 0x71 ||
        t == 0x17 || t == 0x2B || t == 0x4D || t == 0x8E) return 2;
    return 0;
}

void CutMerge( Abc_Ntk_t * p, int iObj, int* pList0, int* pList1, Vec_Int_t* vCuts, Vec_Int_t* vCutsXor, Vec_Int_t* vCutsMaj )
{
    Vec_Int_t * vTemp;
    int i, k, c, Type, truth, *pCut0, *pCut1, pCut[4];
    Vec_IntFill( vCuts, 2, 1 );  // cut of iObj itself
    Vec_IntPush( vCuts, iObj );  // [ 1 1 iobj]

    ForEachCut( pList0, pCut0, i )
    ForEachCut( pList1, pCut1, k )
    {
        if( !CutMergeOne( pCut0, pCut1, pCut ) ) continue;
        if( CutCheckEqual( vCuts, pCut ) ) continue;
        Vec_IntAddToEntry( vCuts, 0 , 1 ); // an cut is added
        for( c = 0; c <= pCut[0]; c++ ) Vec_IntPush( vCuts, pCut[c] );
        if( pCut[0] != 3 ) continue;
        Type = ComputeTruth( p, iObj, pCut, &truth );
        if( Type == 0 ) continue;
        vTemp = Type == 1 ? vCutsXor : vCutsMaj;
        // [in0, in1, in2, iObj truth]
        for( c = 1; c <= pCut[0]; c++ ) Vec_IntPush( vTemp, pCut[c] );
        Vec_IntPush( vTemp, iObj );  
        Vec_IntPush( vTemp, truth );
    }
}

void ComputeCuts( Abc_Ntk_t * pNtk, Vec_Int_t ** pvCutsXor, Vec_Int_t ** pvCutsMaj)
{
    Abc_Obj_t * pObj;
    int *pList0, *pList1, i, nCuts = 0;
    Vec_Int_t * vTemp = Vec_IntAlloc( 1000 );
    Vec_Int_t * vCutsXor = Vec_IntAlloc( Abc_NtkNodeNum(pNtk) );
    Vec_Int_t * vCutsMaj = Vec_IntAlloc( Abc_NtkNodeNum(pNtk) );
    Vec_Int_t * vCuts = Vec_IntAlloc( 30*Abc_NtkNodeNum(pNtk) );
    Vec_IntFill( vCuts, Abc_NtkObjNum(pNtk), 0 );

    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        Vec_IntWriteEntry( vCuts, Abc_ObjId(pObj), Vec_IntSize(vCuts) ); // mapping
        Vec_IntPush( vCuts, 1 );  // # of cut
        Vec_IntPush( vCuts, 1 );  // |1st cut|
        Vec_IntPush( vCuts, Abc_ObjId(pObj) );
    }
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        //printf("Node Id: %d In0 Id: %d, In1 Id: %d\n", Abc_ObjId(pObj), Abc_ObjFaninId0(pObj), Abc_ObjFaninId1(pObj) );
        pList0 = Vec_IntEntryP( vCuts, Vec_IntEntry(vCuts, Abc_ObjFaninId0(pObj)) );
        pList1 = Vec_IntEntryP( vCuts, Vec_IntEntry(vCuts, Abc_ObjFaninId1(pObj)) );
        CutMerge( pNtk, i, pList0, pList1, vTemp, vCutsXor, vCutsMaj );
        Vec_IntWriteEntry( vCuts, i, Vec_IntSize(vCuts) ); // mapping
        Vec_IntAppend( vCuts, vTemp );
        nCuts+= Vec_IntEntry( vTemp, 0 );
        //int j; for( j = 0; j < Vec_IntSize(vCuts); j++ ) printf( "%d ", Vec_IntEntry(vCuts,j) ); printf("\n"); 
    }
    //printf( "Nodes = %d.  Cuts = %d.\n", Abc_NtkNodeNum(pNtk), nCuts );
    
    Vec_IntFree( vTemp );
    Vec_IntFree( vCuts );
    *pvCutsXor = vCutsXor;
    *pvCutsMaj = vCutsMaj;
    //int j; for( j = 0; j < Vec_IntSize(vCutsMaj); j++) printf( "%d ", Vec_IntEntry(vCutsMaj, j) ); printf("\n");
}

Vec_Int_t * FindCommonCuts( Vec_Int_t * vCutsXor, Vec_Int_t * vCutsMaj )
{
    int * pCuts0 = Vec_IntArray(vCutsXor);
    int * pCuts1 = Vec_IntArray(vCutsMaj);
    int * pLimit0 = Vec_IntLimit(vCutsXor);
    int * pLimit1 = Vec_IntLimit(vCutsMaj);
    int i;
    Vec_Int_t * vFadds = Vec_IntAlloc( 1000 );
    assert( Vec_IntSize(vCutsXor) % 5 == 0 );
    assert( Vec_IntSize(vCutsMaj) % 5 == 0 );

    while( pCuts0 < pLimit0 && pCuts1 < pLimit1 )
    {
        for ( i = 0; i < 3; i++ )
            if ( pCuts0[i] != pCuts1[i] ) break;
        if ( i == 3 ) // detected
        {
            for ( i = 0; i < 5; i++ ) Vec_IntPush( vFadds, pCuts0[i] );
            Vec_IntPush( vFadds, pCuts1[3] );
            Vec_IntPush( vFadds, pCuts1[4] );
            pCuts0 += 5; pCuts1 += 5;
        }
        else if ( pCuts0[i] < pCuts1[i] ) pCuts0 += 5;
        else if ( pCuts0[i] > pCuts1[i] ) pCuts1 += 5;
    }
    assert( Vec_IntSize(vFadds) % 7 == 0 );
    return vFadds;
}

void PrintFadds( Vec_Int_t * vFadds )
{
    int i;
    ForEachFadd( vFadds, i )
    {
        printf( "%6d : ", i );
        printf( "%6d ", Vec_IntEntry(vFadds, 7*i+0) );
        printf( "%6d ", Vec_IntEntry(vFadds, 7*i+1) );
        printf( "%6d ", Vec_IntEntry(vFadds, 7*i+2) );
        printf( " ->  " );
        printf( "%6d ", Vec_IntEntry(vFadds, 7*i+3) );
        printf( "%6d ", Vec_IntEntry(vFadds, 7*i+5) );
        printf( "%6X  ", Vec_IntEntry(vFadds, 7*i+4) );
        printf( "%6X  ", Vec_IntEntry(vFadds, 7*i+6) );
        printf( "\n" );
    }
}

int Compare( int * pCut0, int * pCut1 )
{
    if ( pCut0[0] < pCut1[0] ) return -1;
    if ( pCut0[0] > pCut1[0] ) return  1;
    if ( pCut0[1] < pCut1[1] ) return -1;
    if ( pCut0[1] > pCut1[1] ) return  1;
    if ( pCut0[2] < pCut1[2] ) return -1;
    if ( pCut0[2] > pCut1[2] ) return  1;
    return 0;
}

int Compare2( int * pCut0, int * pCut1 )
{
    if ( pCut0[5] < pCut1[5] ) return -1;
    if ( pCut0[5] > pCut1[5] ) return  1;
    return 0;
}

// in0 in1 in2 sum truth carry truth
Vec_Int_t * DetectFullAdders( Abc_Ntk_t * pNtk, Vec_Int_t ** vMaj )
{
    Vec_Int_t * vCutsXor, *vFadds, *vCutsMaj;
    ComputeCuts( pNtk, &vCutsXor, &vCutsMaj );
    qsort( Vec_IntArray(vCutsXor), Vec_IntSize(vCutsXor)/5, 20, (int(*)(const void *, const void *)) Compare );
    qsort( Vec_IntArray(vCutsMaj), Vec_IntSize(vCutsMaj)/5, 20, (int(*)(const void *, const void *)) Compare );
    vFadds = FindCommonCuts( vCutsXor, vCutsMaj );
    qsort( Vec_IntArray(vFadds), Vec_IntSize(vFadds)/7, 28, (int(*)(const void *, const void *)) Compare2 );

    printf("XOR3 cuts = %d. MAJ cuts = %d. Full-adders = %d.\n", Vec_IntSize(vCutsXor)/5, Vec_IntSize(vCutsMaj)/5, Vec_IntSize(vFadds)/7);
    PrintFadds( vFadds );
    Vec_IntFree( vCutsXor );
    *vMaj = vCutsMaj;
    return vFadds;
}

void CollectOneCarryChain_rec( Mig_Obj_t * pNode, Vec_Ptr_t * vChain)
{
    if( pNode->target == 0 ) return;

    assert( Mig_ObjIsNode(pNode) );
    Mig_Obj_t * pFanin;
    pFanin  = Mig_Regular(pNode->pFanin0); CollectOneCarryChain_rec( pFanin, vChain );
    pFanin  = Mig_Regular(pNode->pFanin1); CollectOneCarryChain_rec( pFanin, vChain );
    pFanin  = Mig_Regular(pNode->pFanin2); CollectOneCarryChain_rec( pFanin, vChain );

    Vec_PtrPush( vChain, pNode );
    pNode->target = 0;
}

Vec_Ptr_t * CollectOneCarryChain( Mig_Obj_t * pTop )
{
    // collect in topological order
    Vec_Ptr_t * vChain = Vec_PtrAlloc( 100 );
    CollectOneCarryChain_rec( pTop, vChain );

    return vChain;
}

void CollectChains( Mig_Man_t * pMan )
{
    int i, j, k = 0;
    Mig_Obj_t * pNode, * pFanin;
    Vec_Ptr_t * vCarry = Vec_PtrAlloc( 100 );
    Vec_Ptr_t * vChain;

    Mig_ManForEachNode( pMan, pNode, i )
    {
        if( pNode->target == 0 ) continue;
        Vec_PtrPush( vCarry, pNode );
    }

    Mig_ManIncTravId( pMan );
    Vec_PtrForEachEntry( Mig_Obj_t *, vCarry, pNode, i )
    {
        pFanin = Mig_Regular(pNode->pFanin0);  Mig_ObjSetTravIdCurrent( pMan, pFanin );
        pFanin = Mig_Regular(pNode->pFanin1);  Mig_ObjSetTravIdCurrent( pMan, pFanin );
        pFanin = Mig_Regular(pNode->pFanin2);  Mig_ObjSetTravIdCurrent( pMan, pFanin );
    }
    Vec_PtrForEachEntry( Mig_Obj_t *, vCarry, pNode, i )
    {
        if ( Mig_ObjIsTravIdCurrent( pMan, pNode ) ) continue;
        // pNode is topmost
        vChain = CollectOneCarryChain(pNode);
        if( Vec_PtrSize(vChain) < 3 ) {
            Vec_PtrFree(vChain);
            continue;
        }

        Vec_PtrForEachEntry( Mig_Obj_t *, vChain, pNode, j )
        {
            Vec_VecPush( pMan->vChains, k, pNode );
        }
        k++;
    }
    Vec_VecForEachLevel( pMan->vChains, vChain, i )
    {
        printf( "Chain %4d : %4d  ", i , Vec_PtrSize(vChain));
        Vec_PtrForEachEntry( Mig_Obj_t *, vChain, pNode, k )
        {
            printf( "(%d) ", pNode->Id );
            if( k != Vec_PtrSize(vChain)-1 ) printf( "-> " );
        }
        printf( "\n" );
    }

    Vec_PtrFree(vCarry);
}

//////////////////////////////////////////////////////////////////////////////////
//                          MIG functions                                       //
//////////////////////////////////////////////////////////////////////////////////
void markNodeWithFA(Abc_Ntk_t * pNtk, Vec_Ptr_t * vMigMap, Abc_Obj_t * pObj )
{
    pObj->fMarkA = 1;
    if( !Abc_ObjIsNode( pObj ) ) return;

    Abc_Obj_t * pFanin;
    Mig_Obj_t * pNode = Vec_PtrEntry( vMigMap, pObj->Id );
    if( pNode && pNode->pFanin2 && Mig_Regular(pNode->pFanin2)->Id  < Abc_NtkObjNum(pNtk) ) {
        pFanin = Abc_NtkObj( pNtk, Mig_Regular(pNode->pFanin0)->Id );
        if( !pFanin->fMarkA ) markNodeWithFA( pNtk, vMigMap, pFanin );
        pFanin = Abc_NtkObj( pNtk, Mig_Regular(pNode->pFanin1)->Id );
        if( !pFanin->fMarkA ) markNodeWithFA( pNtk, vMigMap, pFanin );
        pFanin = Abc_NtkObj( pNtk, Mig_Regular(pNode->pFanin2)->Id );
        if( !pFanin->fMarkA ) markNodeWithFA( pNtk, vMigMap, pFanin );
    }
    else {
        pFanin = Abc_ObjFanin0(pObj) ;
        if( !pFanin->fMarkA ) markNodeWithFA( pNtk, vMigMap, pFanin );
        pFanin = Abc_ObjFanin1(pObj) ;
        if( !pFanin->fMarkA ) markNodeWithFA( pNtk, vMigMap, pFanin );
    }
}

void AigToMigWithAdderDetection( Abc_Ntk_t * pNtk, Mig_Man_t * pMig, Vec_Int_t * vFadds, Vec_Int_t * vMaj )
{
    int i, in0Id, in1Id, in2Id, sumId, carryId;
    Abc_Obj_t * pObj;
    Mig_Obj_t * in0 = 0, * in1 = 0, * in2 = 0;
    Mig_Obj_t * sum = 0, * carry = 0;

    // alloc mapping array
    Vec_Ptr_t * vMigMap = Vec_PtrAlloc( Abc_NtkObjNum(pNtk) );
    Vec_PtrFill( vMigMap, Abc_NtkObjNum(pNtk), 0 );

    Vec_PtrWriteEntry( vMigMap, 0,  pMig->pConst1 );
    Abc_NtkForEachCi( pNtk, pObj, i )
        Vec_PtrWriteEntry( vMigMap, pObj->Id , Vec_PtrEntry( pMig->vPis, i) );
    Abc_NtkForEachCo( pNtk, pObj, i )
        Vec_PtrWriteEntry( vMigMap, pObj->Id , Vec_PtrEntry( pMig->vPos, i) );

    // construct adder
    for( i = 0; i < Vec_IntSize(vFadds)/7; i++ ) 
    {
        in0Id = Vec_IntEntry( vFadds, 7*i );
        in1Id = Vec_IntEntry( vFadds, 7*i+1 );
        in2Id = Vec_IntEntry( vFadds, 7*i+2 );
        sumId = Vec_IntEntry( vFadds, 7*i+3 );
        carryId = Vec_IntEntry( vFadds, 7*i+5 );

        if( Vec_PtrEntry(vMigMap,in0Id) ) in0 = Vec_PtrEntry(vMigMap,in0Id);
        else {
            in0 = Mig_ObjAlloc( MIG_OBJ_NODE );
            in0->Id = in0Id;
            Vec_PtrWriteEntry(vMigMap,in0Id,in0);
        }
        if( Vec_PtrEntry(vMigMap,in1Id) ) in1 = Vec_PtrEntry(vMigMap,in1Id);
        else {
            in1 = Mig_ObjAlloc( MIG_OBJ_NODE );
            in1->Id = in1Id;
            Vec_PtrWriteEntry(vMigMap,in1Id,in1);
        }
        if( Vec_PtrEntry(vMigMap,in2Id) ) in2 = Vec_PtrEntry(vMigMap,in2Id);
        else {
            in2 = Mig_ObjAlloc( MIG_OBJ_NODE );
            in2->Id = in2Id;
            Vec_PtrWriteEntry(vMigMap,in2Id, in2);
        }
        if( Vec_PtrEntry(vMigMap, carryId) ) carry = Vec_PtrEntry(vMigMap, carryId);
        else {
            carry = Mig_ObjAlloc( MIG_OBJ_NODE );
            carry->Id = carryId;
            Vec_PtrWriteEntry( vMigMap, carryId, carry );
        }
        if( Vec_PtrEntry(vMigMap, sumId) ) sum = Vec_PtrEntry(vMigMap, sumId);
        else {
            sum = Mig_ObjAlloc( MIG_OBJ_NODE );
            sum->Id = sumId;
            Vec_PtrWriteEntry( vMigMap, sumId, sum );
        }
        carry->pFanin0 = in0;  carry->pFanin1 = in1; carry->pFanin2 = in2;
        unsigned tt = Vec_IntEntry( vFadds, 7*i+6 );
        Mig_Obj_t * tmp = Mig_ObjAlloc( MIG_OBJ_NODE );
        tmp->Id = Vec_PtrSize(vMigMap); Vec_PtrPush(vMigMap, tmp ); assert( Vec_PtrEntry(vMigMap,tmp->Id) == tmp );
        if( tt == 0xE8 ) {
            tmp->pFanin0 = Mig_Not(in0); tmp->pFanin1 = in1; tmp->pFanin2 = in2;
            sum->pFanin0 = Mig_Not(carry); sum->pFanin1 = in0; sum->pFanin2 = tmp;
        }
        else if( tt == 0xD4 ) {
            carry->pFanin0 = Mig_Not(carry->pFanin0);
            tmp->pFanin0 = in0; tmp->pFanin1 = in1; tmp->pFanin2 = in2;
            sum->pFanin0 = carry; sum->pFanin1 = in0; sum->pFanin2 = Mig_Not(tmp);
        }
        else if( tt == 0xB2 ) {
            carry->pFanin1 = Mig_Not(carry->pFanin1);
            tmp->pFanin0 = in0; tmp->pFanin1 = in1; tmp->pFanin2 = in2;
            sum->pFanin0 = carry; sum->pFanin1 = in1; sum->pFanin2 = Mig_Not(tmp);
        }
        else if( tt == 0x8E ) {
            carry->pFanin2 = Mig_Not(carry->pFanin2);
            tmp->pFanin0 = in0; tmp->pFanin1 = in1; tmp->pFanin2 = in2;
            sum->pFanin0 = carry; sum->pFanin1 = in2; sum->pFanin2 = Mig_Not(tmp);
        }
        else if( tt == 0x71 ) {
            carry->pFanin0 = Mig_Not(carry->pFanin0);  carry->pFanin1 = Mig_Not(carry->pFanin1);
            tmp->pFanin0 = in0; tmp->pFanin1 = in1; tmp->pFanin2 = in2;
            sum->pFanin0 = Mig_Not(carry); sum->pFanin1 = in2; sum->pFanin2 = Mig_Not(tmp);
        }
        else if( tt == 0x2B ) {
            carry->pFanin1 = Mig_Not(carry->pFanin1); carry->pFanin2 = Mig_Not(carry->pFanin2);
            tmp->pFanin0 = in0; tmp->pFanin1 = in1; tmp->pFanin2 = in2;
            sum->pFanin0 = Mig_Not(carry); sum->pFanin1 = in0; sum->pFanin2 = Mig_Not(tmp);
        }
        else if( tt == 0x4D ) {
            carry->pFanin0 = Mig_Not(carry->pFanin0); carry->pFanin2 = Mig_Not(carry->pFanin2);
            tmp->pFanin0 = in0; tmp->pFanin1 = in1; tmp->pFanin2 = in2;
            sum->pFanin0 = Mig_Not(carry); sum->pFanin1 = in1; sum->pFanin2 = Mig_Not(tmp);
        }
        else if( tt == 0x17 ) {
            carry->pFanin0 = Mig_Not(carry->pFanin0); carry->pFanin1 = Mig_Not(carry->pFanin1); carry->pFanin2 = Mig_Not(carry->pFanin2);
            tmp->pFanin0 = Mig_Not(in0); tmp->pFanin1 = in1; tmp->pFanin2 = in2;
            sum->pFanin0 = carry; sum->pFanin1 = in0; sum->pFanin2 = tmp;
        }
        else
            assert(0);

        if( Vec_IntEntry( vFadds, 7*i+4 ) == 0x69 ) {
            sum->pFanin0 = Mig_Not(sum->pFanin0);
            sum->pFanin1 = Mig_Not(sum->pFanin1);
            sum->pFanin2 = Mig_Not(sum->pFanin2);
        }
        carry->isFix = 1; sum->isFix = 1;
    }
    // maj node construction
    for( i = 0; i < Vec_IntSize(vMaj)/5; i++ ) 
    {
        in0Id = Vec_IntEntry( vMaj, 5*i );
        in1Id = Vec_IntEntry( vMaj, 5*i+1 );
        in2Id = Vec_IntEntry( vMaj, 5*i+2 );
        carryId = Vec_IntEntry( vMaj, 5*i+3 );
        if( Vec_PtrEntry(vMigMap, carryId) ) carry = Vec_PtrEntry(vMigMap, carryId);  // maybe created without fanin
        else {
            carry = Mig_ObjAlloc( MIG_OBJ_NODE );
            carry->Id = carryId;
            Vec_PtrWriteEntry( vMigMap, carryId, carry );
        }
        // already a fadd
        if( carry->pFanin0 ) continue;  
        carry->isFix = 1;
        carry->target = 1; 
        if( Vec_PtrEntry(vMigMap,in0Id) ) in0 = Vec_PtrEntry(vMigMap,in0Id);
        else {
            in0 = Mig_ObjAlloc( MIG_OBJ_NODE );
            in0->Id = in0Id;
            Vec_PtrWriteEntry(vMigMap,in0Id,in0);
        }
        if( Vec_PtrEntry(vMigMap,in1Id) ) in1 = Vec_PtrEntry(vMigMap,in1Id);
        else {
            in1 = Mig_ObjAlloc( MIG_OBJ_NODE );
            in1->Id = in1Id;
            Vec_PtrWriteEntry(vMigMap,in1Id,in1);
        }
        if( Vec_PtrEntry(vMigMap,in2Id) ) in2 = Vec_PtrEntry(vMigMap,in2Id);
        else {
            in2 = Mig_ObjAlloc( MIG_OBJ_NODE );
            in2->Id = in2Id;
            Vec_PtrWriteEntry(vMigMap,in2Id, in2);
        }
        carry->pFanin0 = in0;  carry->pFanin1 = in1; carry->pFanin2 = in2;
        unsigned tt = Vec_IntEntry( vMaj, 5*i+4 );
        if( tt == 0xD4 || tt == 0x71 || tt == 0x17 || tt == 0x4D ) carry->pFanin0 = Mig_Not(carry->pFanin0);
        if( tt == 0xB2 || tt == 0x71 || tt == 0x17 || tt == 0x2B ) carry->pFanin1 = Mig_Not(carry->pFanin1);
        if( tt == 0x17 || tt == 0x2B || tt == 0x4D || tt == 0x8E ) carry->pFanin2 = Mig_Not(carry->pFanin2);
    }
    printf("Number of MAJ: %d\n", Vec_IntSize(vMaj)/5 );

    // mark tracable and node 
    Abc_NtkForEachCo( pNtk, pObj, i ) 
    {
        Abc_Obj_t * pFanin = Abc_ObjFanin0(pObj);
        if( ! pFanin->fMarkA ) markNodeWithFA( pNtk, vMigMap, pFanin );
    }
    Abc_NtkForEachNode( pNtk, pObj, i )
    {
        // untraced node
        if( pObj->fMarkA == 0 ) continue;
        if( (carry = Vec_PtrEntry(vMigMap, pObj->Id)) == 0 ) // should create new node
        {
            carry = Mig_ObjAlloc( MIG_OBJ_NODE );
            carry->Id = pObj->Id;
            Vec_PtrWriteEntry( vMigMap, carry->Id, carry );
        }
        if( carry->pFanin0 ) continue;
        in0 = Mig_Not(pMig->pConst1);
        in1 = Vec_PtrEntry( vMigMap, Abc_ObjFaninId0(pObj) );
        in2 = Vec_PtrEntry( vMigMap, Abc_ObjFaninId1(pObj) );
        if( Abc_ObjFaninC0(pObj) ) in1 = Mig_Not(in1);
        if( Abc_ObjFaninC1(pObj) ) in2 = Mig_Not(in2);
        assert(in0); assert(in1); assert(in2);
        carry->pFanin0 = in0; carry->pFanin1 = in1; carry->pFanin2 = in2;
    }
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        Abc_Obj_t * fanin = Abc_ObjFanin0( pObj );
        Mig_Obj_t * po = Mig_ManPo( pMig, i );
        po->pFanin0 = Mig_NotCond( Vec_PtrEntry( vMigMap, fanin->Id ), Abc_ObjFaninC0(pObj) );
    }
    Abc_NtkForEachObj( pNtk, pObj, i ) pObj->fMarkA = 0;
    Vec_PtrFree(vMigMap);
    
    // 1. clear fanout 2. genDfs 3. update ID 4, update fanout 5. rehash  6.compute level
    Mig_ManReassignIds( pMig );
    
    // sort fanins  
    Mig_ManForEachNode( pMig, carry, i )
    {
        if( Mig_Regular(carry->pFanin0)->Id > Mig_Regular(carry->pFanin1)->Id )
            sum = carry->pFanin0, carry->pFanin0 = carry->pFanin1, carry->pFanin1 = sum;
        if( Mig_Regular(carry->pFanin1)->Id > Mig_Regular(carry->pFanin2)->Id )
            sum = carry->pFanin1, carry->pFanin1 = carry->pFanin2, carry->pFanin2 = sum;
        if( Mig_Regular(carry->pFanin0)->Id > Mig_Regular(carry->pFanin1)->Id )
            sum = carry->pFanin0, carry->pFanin0 = carry->pFanin1, carry->pFanin1 = sum;
    }
    Mig_MigStrash( pMig );

    CollectChains( pMig );
    Mig_ManForEachNode( pMig, carry, i ) carry->target = 0;

    // collect target
    /*Mig_ManForEachNode( pMig, carry, i )
    {
        if( carry->target == 0 ) continue;
        Vec_PtrPush( pMig->vTarget, carry );
        carry->target = 0;
    }
    Vec_PtrReverseOrder( pMig->vTarget );*/
    //Mig_ManForEachObj( pMig, carry, i )
    //    printf("%d\n", carry->Id);
    //Mig_Write(pNtk,pMig,"fadd.v");
}

/**Function*************************************************************

  Synopsis    [ Detect full adders and match them into mig structure ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mig_Man_t * Mig_AigToMigWithAdderDetection( Abc_Ntk_t * pNtk )
{
    Vec_Int_t * vFadds, * vMaj;
    //Vec_Int_t * vMap;
    //Vec_Wec_t * vChains;
    Mig_Man_t * pMig;
    //int nFaddMin = 3;

   pMig = Mig_MigStart(pNtk);

   // collect full adders in the ntk 
   vFadds = DetectFullAdders( pNtk, &vMaj );
   // vMap[carry id] = iFadd, vMap[other] = -1
   /*vMap = CreateMap( pNtk, vMaj );
   // reorder the in0, in1, in2 -> in0 is the longest chain
   FindChains( pNtk, vMaj, vMap );
   // 
   vChains = CollectTopmost( pNtk, vMaj, vMap, nFaddMin );
   PrintChains( vMaj, vChains );*/ 


   AigToMigWithAdderDetection( pNtk, pMig, vFadds, vMaj );
   return pMig;
}


////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

