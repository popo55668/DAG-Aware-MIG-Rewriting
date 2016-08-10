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

// Mig_Obj_t is used as Graph node, fPhase is used as fUsed (marking variables)
// The output of each graph should not complemented
// Graph is manipulated with Mig_Man_t, vObjs is stored as vForest

static unsigned canon_truth [] = { 
    0 ,1 ,3 ,6 ,7 ,15 ,22 ,23 ,24 ,25 ,
    27 ,30 ,31 ,60 ,61 ,63 ,105 ,107 ,111 ,126 ,
    127 ,255 ,278 ,279 ,280 ,281 ,282 ,283 ,286 ,287 ,
    300 ,301 ,303 ,316 ,317 ,318 ,319 ,360 ,361 ,362 ,
    363 ,366 ,367 ,382 ,383 ,384 ,385 ,386 ,387 ,390 ,
    391 ,393 ,395 ,399 ,406 ,407 ,408 ,409 ,410 ,411 ,
    414 ,415 ,424 ,425 ,426 ,427 ,428 ,429 ,430 ,431 ,
    444 ,445 ,446 ,447 ,488 ,489 ,490 ,491 ,494 ,495 ,
    510 ,828 ,829 ,831 ,854 ,855 ,856 ,857 ,858 ,859 ,
    862 ,863 ,872 ,873 ,874 ,875 ,876 ,877 ,878 ,879 ,
    892 ,893 ,894 ,960 ,961 ,963 ,965 ,966 ,967 ,975 ,
    980 ,981 ,982 ,983 ,984 ,985 ,987 ,988 ,989 ,990 ,
    1020 ,1632 ,1633 ,1634 ,1635 ,1638 ,1639 ,1641 ,1643 ,1647 ,
    1650 ,1651 ,1654 ,1656 ,1657 ,1658 ,1659 ,1662 ,1680 ,1681 ,
    1683 ,1686 ,1687 ,1695 ,1712 ,1713 ,1714 ,1715 ,1716 ,1717 ,
    1718 ,1719 ,1721 ,1725 ,1776 ,1777 ,1778 ,1782 ,1785 ,1910 ,
    1912 ,1913 ,1914 ,1918 ,1968 ,1969 ,1972 ,1973 ,1974 ,1980 ,
    2016 ,2017 ,2018 ,2019 ,2022 ,2025 ,2032 ,2033 ,2034 ,2040 ,
    4080 ,5736 ,5737 ,5738 ,5739 ,5742 ,5758 ,5761 ,5763 ,5766 ,
    5767 ,5769 ,5771 ,5774 ,5782 ,5783 ,5784 ,5785 ,5786 ,5787 ,
    5790 ,5801 ,5804 ,5805 ,5820 ,5865 ,6014 ,6030 ,6038 ,6040 ,
    6042 ,6060 ,6120 ,6375 ,6625 ,6627 ,6630 ,7128 ,7140 ,7905 ,
    15555 ,27030 
};

static unsigned canon_truth3 [] = {
    0, 1, 3, 6, 7, 15, 22, 23, 24, 25, 27, 30, 60, 105 
};
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mig_Obj_t * Graph_ManAddVar( Mig_Man_t * p, Mig_ObjType_t type, unsigned uTruth )
{
    Mig_Obj_t * pNew = Mig_ObjAlloc(type);
    pNew->Id = p->vObjs->nSize;
    pNew->uTruth = uTruth;
    pNew->Level  = 0;
//    pNew->Volume = 0;
    pNew->fPhase = 1;
    pNew->pFanin0= NULL;
    pNew->pFanin1= NULL;
    pNew->pFanin2= NULL;
    pNew->pNext  = NULL;
    Vec_PtrPush( p->vObjs, pNew );
    return pNew;
}

Mig_Man_t * Graph_ManStart()
{
    Mig_Man_t * p;
    p = ABC_ALLOC( Mig_Man_t, 1 );
    memset( p, 0, sizeof(Mig_Man_t) );
    // allocate table
    p->nTableSize = Abc_PrimeCudd( 10000 );               // roughly selected now
    p->pTable = ABC_ALLOC( Mig_Obj_t *, p->nTableSize );
    // allocate forest
    p->vObjs = Vec_PtrAlloc( 100 );
    p->vPos  = Vec_PtrAlloc( 100 );
    Graph_ManAddVar( p, MIG_OBJ_CONST1, 0xFFFF ); // const1
    Graph_ManAddVar( p, MIG_OBJ_PI, 0xAAAA );     // var A
    Graph_ManAddVar( p, MIG_OBJ_PI, 0xCCCC );     // var B
    Graph_ManAddVar( p, MIG_OBJ_PI, 0xF0F0 );     // var C
    Graph_ManAddVar( p, MIG_OBJ_PI, 0xFF00 );     // var D
    return p;
}

Mig_Man_t * Graph_ManStart3()
{
    Mig_Man_t * p;
    p = ABC_ALLOC( Mig_Man_t, 1 );
    memset( p, 0, sizeof(Mig_Man_t) );
    // allocate table
    p->nTableSize = Abc_PrimeCudd( 10000 );               // roughly selected now
    p->pTable = ABC_ALLOC( Mig_Obj_t *, p->nTableSize );
    // allocate forest
    p->vObjs = Vec_PtrAlloc( 100 );
    p->vPos  = Vec_PtrAlloc( 100 );
    Graph_ManAddVar( p, MIG_OBJ_CONST1, 0x00FF ); // const1
    Graph_ManAddVar( p, MIG_OBJ_PI, 0x00AA );     // var A
    Graph_ManAddVar( p, MIG_OBJ_PI, 0x00CC );     // var B
    Graph_ManAddVar( p, MIG_OBJ_PI, 0x00F0 );     // var C
    return p;
}

void Graph_GenerateGraphs( FILE* pFile, Mig_Man_t * p, unsigned TRUTH )
{
    unsigned numGraph;
    unsigned numNode;
    char c0[6], c1[6], c2[6], out[6];
    unsigned id0, id1, id2;
    unsigned uTruth0, uTruth1, uTruth2;
    unsigned compl0, compl1, compl2;
    Mig_Obj_t * pFanin0, *pFanin1, *pFanin2, *pMaj;
    int i, k;

    printf("Generating graphs for truth = %d\n", TRUTH);
    fscanf( pFile, "%u", &numGraph );
    for( i = 0; i < numGraph; i++ )
    {
        fscanf( pFile, "%d", &numNode );
        Vec_Ptr_t * vNodes = Vec_PtrAlloc( numNode );
        for( k = 0; k < 5; k++ ) Vec_PtrPush( vNodes, Vec_PtrEntry( p->vObjs, k ) );
        for( k = 0; k < numNode; k++ )
        {
            fscanf( pFile, "%u%s%u%s%u%s", &id0, c0, &id1, c1, &id2, c2 );
            //printf("%d %d %d\n", id0, id1, id2 );
            compl0 = (strcmp("true",c0)==0);
            compl1 = (strcmp("true",c1)==0);
            compl2 = (strcmp("true",c2)==0);
            if( id0 == 0 ) compl0 = !compl0;  // in z3, id = 0 means const0 while in MIG, it means const1 in our implementation
            pFanin0 = Vec_PtrEntry( vNodes, id0 );  assert( !Mig_IsComplement(pFanin0) );
            pFanin1 = Vec_PtrEntry( vNodes, id1 );  assert( !Mig_IsComplement(pFanin1) );
            pFanin2 = Vec_PtrEntry( vNodes, id2 );  assert( !Mig_IsComplement(pFanin2) );
            pFanin0 = Mig_NotCond( pFanin0, compl0);
            pFanin1 = Mig_NotCond( pFanin1, compl1);
            pFanin2 = Mig_NotCond( pFanin2, compl2);
            /*
            if(compl0) pFanin0 = Mig_Not(pFanin0);
            if(compl1) pFanin1 = Mig_Not(pFanin1);
            if(compl2) pFanin2 = Mig_Not(pFanin2);
            */
            pMaj = Mig_MigMaj( p, pFanin0, pFanin1, pFanin2 ); // in0,in1,in2,next,level
            assert( !Mig_IsComplement(pMaj) );
            if( pMaj->uTruth == 0 ) {
                assert( pMaj->Id == p->vObjs->nSize-1 );
                uTruth0 = Mig_Regular(pFanin0)->uTruth;  if(compl0) uTruth0 = ~uTruth0;
                uTruth1 = Mig_Regular(pFanin1)->uTruth;  if(compl1) uTruth1 = ~uTruth1;
                uTruth2 = Mig_Regular(pFanin2)->uTruth;  if(compl2) uTruth2 = ~uTruth2;
                pMaj->uTruth =  ((uTruth0 & uTruth1) | (uTruth0 & uTruth2) | (uTruth1 & uTruth2)) & 0xFFFF;   
            }
            Vec_PtrPush( vNodes, pMaj );
        }
        pMaj = Vec_PtrEntry( vNodes, numNode + 4 );
        pMaj->TravId = 1;  // marked as PO
        assert( pMaj->uTruth == TRUTH );
        Vec_PtrFree(vNodes);
        fscanf( pFile, "%s", out );
    }
}

void Mig_GenerateAllGraphs()
{
    unsigned i;
    unsigned digit;
    Mig_Man_t * pMan = Graph_ManStart();
    /*FILE* pFile = fopen( "1.net", "r" );
    Graph_GenerateGraphs( pFile, pMan, 1 );
    fclose(pFile);
    pFile = fopen( "2.net", "r" );
    Graph_GenerateGraphs( pFile, pMan, 3 );
    fclose(pFile);
    pFile = fopen( "3.net", "r" );
    Graph_GenerateGraphs( pFile, pMan, 6 );
    fclose(pFile);
    pFile = fopen( "4.net", "r" );
    Graph_GenerateGraphs( pFile, pMan, 7 );
    fclose(pFile);
    pFile = fopen( "5.net", "r" );
    Graph_GenerateGraphs( pFile, pMan, 15 );
    fclose(pFile);
    pFile = fopen( "6.net", "r" );
    Graph_GenerateGraphs( pFile, pMan,22 );
    fclose(pFile);
    pFile = fopen( "7.net", "r" );
    Graph_GenerateGraphs( pFile, pMan, 23 );
    fclose(pFile);
    pFile = fopen( "8.net", "r" );
    Graph_GenerateGraphs( pFile, pMan, 24 );
    fclose(pFile);
    pFile = fopen( "9.net", "r" );
    Graph_GenerateGraphs( pFile, pMan, 25 );
    fclose(pFile);
    for ( i = 10; i < 222; i++) {
        if( i <= 9 ) digit = 1;
        else if ( i <= 99 ) digit = 2;
        else digit = 3;
        char fName[digit+4];
        sprintf( fName, "%u", i );
        fName[digit  ] = '.';
        fName[digit+1] = 'n';
        fName[digit+2] = 'e'; 
        fName[digit+3] = 't';
        FILE* pFile = fopen( fName, "r" );
        if(!pFile){ printf("%d\n",i); continue;  }
        Graph_GenerateGraphs( pFile, pMan, canon_truth[i] );
        fclose(pFile);
    }*/
    FILE* pFile = fopen( "221.net", "r" );
    Graph_GenerateGraphs( pFile, pMan, 27030 );
    fclose(pFile);
    Mig_WriteToArray( pMan );
    //Mig_PrintHashTable( pMan );
    //Mig_Print(pMan);
} 

void Mig_WriteToArray( Mig_Man_t * p )
{
    FILE* pFile;
    Mig_Obj_t * pNode;
    int i;
    unsigned Entry0, Entry1, Entry2;
    pFile = fopen( "npn4_mig_array.txt", "w" );
    fprintf( pFile, "static unsigned short s_MigSubgraphs[] = \n{" );

    Mig_ManForEachObj( p, pNode, i )
    {
        //  Id0  |compl| output
        //     Id1     | compl
        //     Id2     | compl
        if( i < 5 ) continue;  // const | var
        if( i % 5 == 0 ) fprintf( pFile, "\n    " );
        Entry0 = (Mig_Regular(pNode->pFanin0)->Id << 1 ) | Mig_IsComplement(pNode->pFanin0);
        Entry1 = (Mig_Regular(pNode->pFanin1)->Id << 1 ) | Mig_IsComplement(pNode->pFanin1);
        Entry2 = (Mig_Regular(pNode->pFanin2)->Id << 1 ) | Mig_IsComplement(pNode->pFanin2);
        assert( pNode->TravId == 0 || pNode->TravId == 1 );
        Entry0 = (Entry0 << 1 ) | pNode->TravId;
        Extra_PrintHex( pFile, &Entry0, 5 ); fprintf( pFile, "," );
        Extra_PrintHex( pFile, &Entry1, 5 ); fprintf( pFile, "," );
        Extra_PrintHex( pFile, &Entry2, 5 ); fprintf( pFile, ", " );
    }
    if( i % 5 == 0 ) fprintf( pFile, "\n    " );
    Entry0 = 0;
    Extra_PrintHex( pFile, &Entry0, 5 ); fprintf( pFile, "," );
    Extra_PrintHex( pFile, &Entry0, 5 ); fprintf( pFile, "," );
    Extra_PrintHex( pFile, &Entry0, 5 ); fprintf( pFile, "\n};\n" );
    fclose( pFile );
    printf( "The number of nodes saved = %d.   \n", p->nEntries );

    /*
    // print output node ID
    pFile = fopen( "mig_subgraph_output.txt", "w" );
    fprintf( pFile, "static unsigned int s_MigSubgraphOutputs[] = \n{    " );
    Mig_ManForEachObj( p, pNode, i )
    {
        if( pNode->TravId == 0 ) continue;
        fprintf( pFile, "%d, ", pNode->Id );
        Entry0++;
        if( Entry0 % 5 == 0 ) fprintf( pFile, "\n    " );
    }
    fprintf( pFile, "0\n};\n" );
    fclose( pFile );
    printf( "The number of subgraphs = %d.   \n", Entry0 );*/
}

void Mig_WriteToArray3( Mig_Man_t * p )
{
    FILE* pFile;
    Mig_Obj_t * pNode;
    int i;
    unsigned Entry0, Entry1, Entry2;
    pFile = fopen( "npn4_var3_mig_array.txt", "w" );
    fprintf( pFile, "static unsigned short s_MigSubgraphsVar3[] = \n{" );

    Mig_ManForEachObj( p, pNode, i )
    {
        //  Id0  |compl| output
        //     Id1     | compl
        //     Id2     | compl
        if( i < 4 ) continue;  // const | var
        if( i % 5 == 0 ) fprintf( pFile, "\n    " );
        Entry0 = (Mig_Regular(pNode->pFanin0)->Id << 1 ) | Mig_IsComplement(pNode->pFanin0);
        Entry1 = (Mig_Regular(pNode->pFanin1)->Id << 1 ) | Mig_IsComplement(pNode->pFanin1);
        Entry2 = (Mig_Regular(pNode->pFanin2)->Id << 1 ) | Mig_IsComplement(pNode->pFanin2);
        assert( pNode->TravId == 0 || pNode->TravId == 1 );
        Entry0 = (Entry0 << 1 ) | pNode->TravId;
        Extra_PrintHex( pFile, &Entry0, 5 ); fprintf( pFile, "," );
        Extra_PrintHex( pFile, &Entry1, 5 ); fprintf( pFile, "," );
        Extra_PrintHex( pFile, &Entry2, 5 ); fprintf( pFile, ", " );
    }
    if( i % 5 == 0 ) fprintf( pFile, "\n    " );
    Entry0 = 0;
    Extra_PrintHex( pFile, &Entry0, 5 ); fprintf( pFile, "," );
    Extra_PrintHex( pFile, &Entry0, 5 ); fprintf( pFile, "," );
    Extra_PrintHex( pFile, &Entry0, 5 ); fprintf( pFile, "\n};\n" );
    fclose( pFile );
    printf( "The number of nodes saved = %d.   \n", p->nEntries );
}

void Graph_GenerateGraphs3( FILE* pFile, Mig_Man_t * p, unsigned TRUTH )
{
    unsigned numGraph;
    unsigned numNode;
    char c0[6], c1[6], c2[6], out[6];
    unsigned id0, id1, id2;
    unsigned uTruth0, uTruth1, uTruth2;
    unsigned compl0, compl1, compl2;
    Mig_Obj_t * pFanin0, *pFanin1, *pFanin2, *pMaj;
    int i, k;

    printf("Generating graphs for truth = %d\n", TRUTH);
    fscanf( pFile, "%u", &numGraph );
    for( i = 0; i < numGraph; i++ )
    {
        fscanf( pFile, "%d", &numNode );
        Vec_Ptr_t * vNodes = Vec_PtrAlloc( numNode );
        for( k = 0; k < 4; k++ ) Vec_PtrPush( vNodes, Vec_PtrEntry( p->vObjs, k ) );
        for( k = 0; k < numNode; k++ )
        {
            fscanf( pFile, "%u%s%u%s%u%s", &id0, c0, &id1, c1, &id2, c2 );
            //printf("%d %d %d\n", id0, id1, id2 );
            compl0 = (strcmp("true",c0)==0);
            compl1 = (strcmp("true",c1)==0);
            compl2 = (strcmp("true",c2)==0);
            if( id0 == 0 ) compl0 = !compl0;  // in z3, id = 0 means const0 while in MIG, it means const1 in our implementation
            pFanin0 = Vec_PtrEntry( vNodes, id0 );  assert( !Mig_IsComplement(pFanin0) );
            pFanin1 = Vec_PtrEntry( vNodes, id1 );  assert( !Mig_IsComplement(pFanin1) );
            pFanin2 = Vec_PtrEntry( vNodes, id2 );  assert( !Mig_IsComplement(pFanin2) );
            pFanin0 = Mig_NotCond( pFanin0, compl0);
            pFanin1 = Mig_NotCond( pFanin1, compl1);
            pFanin2 = Mig_NotCond( pFanin2, compl2);
            /*
            if(compl0) pFanin0 = Mig_Not(pFanin0);
            if(compl1) pFanin1 = Mig_Not(pFanin1);
            if(compl2) pFanin2 = Mig_Not(pFanin2);
            */
            pMaj = Mig_MigMaj( p, pFanin0, pFanin1, pFanin2 ); // in0,in1,in2,next,level
            assert( !Mig_IsComplement(pMaj) );
            if( pMaj->uTruth == 0 ) {
                assert( pMaj->Id == p->vObjs->nSize-1 );
                uTruth0 = Mig_Regular(pFanin0)->uTruth;  if(compl0) uTruth0 = ~uTruth0;
                uTruth1 = Mig_Regular(pFanin1)->uTruth;  if(compl1) uTruth1 = ~uTruth1;
                uTruth2 = Mig_Regular(pFanin2)->uTruth;  if(compl2) uTruth2 = ~uTruth2;
                pMaj->uTruth =  ((uTruth0 & uTruth1) | (uTruth0 & uTruth2) | (uTruth1 & uTruth2)) & 0x00FF;   
            }
            Vec_PtrPush( vNodes, pMaj );
        }
        pMaj = Vec_PtrEntry( vNodes, numNode + 3 );
        pMaj->TravId = 1;  // marked as PO
        assert( pMaj->uTruth == TRUTH );
        Vec_PtrFree(vNodes);
        fscanf( pFile, "%s", out );
    }
}

void Mig_GenerateAllGraphs3()
{
    unsigned i;
    unsigned digit;
    Mig_Man_t * pMan = Graph_ManStart3();
    FILE* pFile = fopen( "1.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan, 1 );
    fclose(pFile);
    pFile = fopen( "2.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan, 3 );
    fclose(pFile);
    pFile = fopen( "3.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan, 6 );
    fclose(pFile);
    pFile = fopen( "4.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan, 7 );
    fclose(pFile);
//    pFile = fopen( "5.net", "r" );
//    Graph_GenerateGraphs3( pFile, pMan, 15 );
//    fclose(pFile);
    pFile = fopen( "6.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan,22 );
    fclose(pFile);
    pFile = fopen( "7.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan, 23 );
    fclose(pFile);
    pFile = fopen( "8.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan, 24 );
    fclose(pFile);
    pFile = fopen( "9.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan, 25 );
    fclose(pFile);
    pFile = fopen( "10.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan, 27 );
    fclose(pFile);
    pFile = fopen( "11.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan, 30 );
    fclose(pFile);
    pFile = fopen( "12.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan, 60 );
    fclose(pFile);
    pFile = fopen( "13.net", "r" );
    Graph_GenerateGraphs3( pFile, pMan, 105 );
    fclose(pFile);
    
    Mig_WriteToArray3( pMan );
}
////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

