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
#include "base/io/ioAbc.h"

ABC_NAMESPACE_IMPL_START

//////////////////////////////////////////////////////////////////////////////////
//                          MIG functions                                       //
//////////////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [ initialize a MIG Manager ]

  Description [ const/pi/po will be created ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mig_Man_t* Mig_MigStart( Abc_Ntk_t * pNtk)
{
    Mig_Man_t* p;
    Abc_Obj_t* pObj;
    int i;

    p = ABC_ALLOC( Mig_Man_t, 1 );
    memset( p, 0, sizeof(Mig_Man_t) );
    p->vObjs            = Vec_PtrAlloc( 100 );
    p->vPis             = Vec_PtrAlloc( Abc_NtkCiNum(pNtk) );
    p->vPos             = Vec_PtrAlloc( Abc_NtkCoNum(pNtk) );
    p->vStackReplaceOld = Vec_PtrAlloc( 100 );
    p->vStackReplaceNew = Vec_PtrAlloc( 100 );
    p->vNodes           = Vec_PtrAlloc( 100 );
    p->vTarget          = Vec_PtrAlloc( 100 );
    p->vChains          = Vec_VecAlloc( 100 );
    p->vLevels          = Vec_VecAlloc( 100 );
    p->vLevelsR         = Vec_VecAlloc( 100 );

    // const/pi/po will be created
    p->pConst1 = Mig_ObjAlloc( MIG_OBJ_CONST1 );
    p->pConst1->Id = 0;  p->pConst1->fPhase = 1;
    Vec_PtrPush( p->vObjs, p->pConst1 );
    p->nObjCounts[MIG_OBJ_CONST1]++;
    assert( Vec_PtrSize(p->vObjs) == 1);

    Abc_NtkForEachCi( pNtk, pObj, i )
    {
        Mig_Obj_t* m = Mig_ObjAlloc( MIG_OBJ_PI );
        m->Id = pObj->Id;
        m->Level = 0;
        Vec_PtrPush( p->vPis, m );
        p->nObjCounts[MIG_OBJ_PI]++;
    }
    assert( p->nObjCounts[MIG_OBJ_PI] == Abc_NtkCiNum(pNtk) );
    Abc_NtkForEachCo( pNtk, pObj, i )
    {
        Mig_Obj_t * m = Mig_ObjAlloc( MIG_OBJ_PO );
        m->Id = pObj->Id;
        Vec_PtrPush( p->vPos, m );
        p->nObjCounts[MIG_OBJ_PO]++;
    }
    assert( p->nObjCounts[MIG_OBJ_PO] == Abc_NtkCoNum(pNtk) );

    p->nTableSize = Abc_PrimeCudd( Abc_NtkNodeNum(pNtk) + 100 );
    p->pTable = ABC_ALLOC( Mig_Obj_t*, p->nTableSize );
    memset( p->pTable, 0, sizeof(Mig_Obj_t*) * p->nTableSize );
    p->nEntries = 0;
    p->nTravIds = 0;
    p->LevelMax = 0;

    // fec table    
    p->pSimTable = Vec_VecStart( p->nTableSize );

    return p;
}

void Mig_GenOrderList_rec( Mig_Man_t * pMan, Vec_Ptr_t * vNodes, Mig_Obj_t * pObj )
{
    assert(pObj->pFanin0);
    assert(pObj->pFanin1);
    assert(pObj->pFanin2);
    assert( Mig_ObjIsNode(pObj) );
    assert( !Mig_IsComplement(pObj) );
    Mig_ObjSetTravIdCurrent( pMan, pObj );
    pMan->nObjCounts[MIG_OBJ_NODE]++;
    if( !Mig_ObjIsTravIdCurrent( pMan, Mig_Regular(pObj->pFanin0) ) ) Mig_GenOrderList_rec( pMan, vNodes, Mig_Regular(pObj->pFanin0) );
    if( !Mig_ObjIsTravIdCurrent( pMan, Mig_Regular(pObj->pFanin1) ) ) Mig_GenOrderList_rec( pMan, vNodes, Mig_Regular(pObj->pFanin1) );
    if( !Mig_ObjIsTravIdCurrent( pMan, Mig_Regular(pObj->pFanin2) ) ) Mig_GenOrderList_rec( pMan, vNodes, Mig_Regular(pObj->pFanin2) );
    Vec_PtrPush( vNodes, pObj );
}

void Mig_ManReassignIds( Mig_Man_t * pMan )  // also reallocate vFanouts
{
    Vec_Ptr_t * vObjsNew;
    Mig_Obj_t * pObj;
    int i;

    Mig_ManForEachObj( pMan, pObj, i )
        Vec_IntClear( &pObj->vFanouts );

    // const->pi->obj->po
    vObjsNew = Vec_PtrAlloc( Mig_ManObjNum(pMan) );

    Mig_ManIncTravId( pMan );
    Vec_PtrPush( vObjsNew, pMan->pConst1 );  Mig_ObjSetTravIdCurrent( pMan, pMan->pConst1 );
    Mig_ManForEachPi( pMan, pObj, i )
    {
        Vec_PtrPush( vObjsNew, pObj );
        Mig_ObjSetTravIdCurrent( pMan, pObj );
        pObj->Level = 0;
    }
    Mig_ManForEachPo( pMan, pObj, i )
    {
        if( !Mig_ObjIsTravIdCurrent( pMan, Mig_Regular(pObj->pFanin0) ) )
            Mig_GenOrderList_rec( pMan, vObjsNew, Mig_Regular(pObj->pFanin0) );
    }
    Mig_ManForEachPo( pMan, pObj, i ) Vec_PtrPush( vObjsNew, pObj );
    Vec_PtrFree( pMan->vObjs );
    pMan->vObjs = vObjsNew;

    // reassigned IDs
    Mig_ManForEachObj( pMan, pObj, i )  pObj->Id = i;
    
    // fanout info
    Mig_ManForEachObj( pMan, pObj, i )
    {
        if( Mig_ObjIsConst(pObj) || Mig_ObjIsPi(pObj) ) continue;
        Mig_Obj_t * pFanin = Mig_ObjFanin0(pObj);
        Vec_IntPush( &pFanin->vFanouts, Mig_ObjId(pObj) );
        if( Mig_ObjIsPo(pObj) ) continue;
        pFanin = Mig_ObjFanin1(pObj);
        Vec_IntPush( &pFanin->vFanouts, Mig_ObjId(pObj) );
        pFanin = Mig_ObjFanin2(pObj);
        Vec_IntPush( &pFanin->vFanouts, Mig_ObjId(pObj) );
    }
    // update level
    Mig_ManForEachNode( pMan, pObj, i )
    {
        pObj->Level = 1 + Abc_MaxInt( Mig_Regular(pObj->pFanin0)->Level, Abc_MaxInt(Mig_Regular(pObj->pFanin1)->Level, Mig_Regular(pObj->pFanin2)->Level) );
    }
    pMan->LevelMax = 0;
    Mig_ManForEachPo( pMan, pObj, i )
    {
        pObj->Level = 1 + Mig_Regular(pObj->pFanin0)->Level;
        if( pObj->Level > pMan->LevelMax )
            pMan->LevelMax = pObj->Level;
    }
    // rehash
    Mig_MigRehash(pMan);
}

void AigToMig( Abc_Ntk_t * pNtk, Mig_Man_t * m )
{
    int i;
    Abc_Obj_t * pNode;
    Vec_Ptr_t * vTemp = Vec_PtrAlloc( Abc_NtkObjNum(pNtk) );
    Vec_PtrFill( vTemp, Abc_NtkObjNum(pNtk), 0 );
    Vec_PtrWriteEntry( vTemp, 0,  m->pConst1 );
    Abc_NtkForEachCi( pNtk, pNode, i )
        Vec_PtrWriteEntry( vTemp, pNode->Id , Vec_PtrEntry( m->vPis, i) );
    Abc_NtkForEachCo( pNtk, pNode, i )
        Vec_PtrWriteEntry( vTemp, pNode->Id , Vec_PtrEntry( m->vPos, i) );
    

    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Mig_Obj_t * pMaj;
            Mig_Obj_t * pFanin0 = Mig_Not(m->pConst1);
            Mig_Obj_t * pFanin1 = Vec_PtrEntry( vTemp, Abc_ObjFaninId0(pNode) );
            Mig_Obj_t * pFanin2 = Vec_PtrEntry( vTemp, Abc_ObjFaninId1(pNode) );
            if( Abc_ObjFaninC0(pNode) ) pFanin1 = Mig_Not(pFanin1);
            if( Abc_ObjFaninC1(pNode) ) pFanin2 = Mig_Not(pFanin2);
            assert(pFanin0); assert(pFanin1); assert(pFanin2);
            if( (pMaj = Mig_MigMajLookup(m, pFanin0, pFanin1, pFanin2) ) == NULL )
                pMaj = Mig_MigMajCreateWithoutId( m, pFanin0, pFanin1, pFanin2 );
        if( pMaj->Id == -1 ) pMaj->Id = pNode->Id;  // this is a new node
        assert( Vec_PtrEntry( vTemp, pNode->Id ) == 0);
        Vec_PtrWriteEntry( vTemp, pNode->Id, pMaj );
    }

    Abc_NtkForEachCo( pNtk, pNode, i )
    {
        Abc_Obj_t * fanin = Abc_ObjFanin0( pNode );
        Mig_Obj_t * po = Mig_ManPo( m, i );
        po->pFanin0 = Mig_NotCond( Vec_PtrEntry( vTemp, fanin->Id ), Abc_ObjFaninC0(pNode) );
        po->Level = 1 + Mig_Regular(po->pFanin0)->Level;
    }

    // 1. clear fanout 2. genDfs 3. update ID 4, update fanout 5. rehash  6.compute level
    Mig_ManReassignIds( m );

    Vec_PtrFree( vTemp );

    /*Mig_ManForEachObj( m, mNode, i )
        printf("Id = %d, numFanout = %d.\n", Mig_ObjId(mNode), Mig_ObjFanoutNum(mNode) );*/
}

/**Function*************************************************************

  Synopsis    [returns # of dangling nodes removed]

  Description [Delete dangling nodes (and it fanin cone ) in hash table]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Mig_MigCleanup( Mig_Man_t * pMan )
{
    Vec_Ptr_t * vDangles;
    Mig_Obj_t * pMaj;
    int i, nNodesOld;

    nNodesOld = pMan->nEntries;
    vDangles = Vec_PtrAlloc( 100 );
    for ( i = 0; i < pMan->nTableSize; i++ )
        Mig_MigBinForEachEntry( pMan->pTable[i], pMaj )
            if( Mig_ObjFanoutNum(pMaj) == 0 )
                Vec_PtrPush( vDangles, pMaj );
    Vec_PtrForEachEntry( Mig_Obj_t *, vDangles, pMaj, i )
        Mig_MigDeleteNode( pMan, pMaj );
    Vec_PtrFree( vDangles );
    return nNodesOld - pMan->nEntries;
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mig_Man_t * Mig_AigToMig( Abc_Ntk_t * pNtk)
{
    Mig_Man_t * pMig = Mig_MigStart(pNtk);
    AigToMig( pNtk, pMig );
    Mig_MigCleanup( pMig );  // remove node not in dfs but in table
    return pMig;
}

Mig_Obj_t * getObjByName( char * name, Abc_Ntk_t * pNtk, Mig_Man_t * pMan, Vec_Ptr_t * vObjs, int del )
{
    if( name[0] == 'w' ){
        name = name + 1;
        return Vec_PtrEntry( vObjs, atoi(name) );
    }
    name[ strlen(name)-del ] = '\0';
    if(strcmp(name,"one") == 0){
        return pMan->pConst1;
    }
    else{
        int i;
        Abc_Obj_t * pObj;
        Abc_NtkForEachPi( pNtk, pObj, i )
        {
            if( strcmp( name, Abc_ObjName(pObj) ) == 0 )
                return Mig_ManPi( pMan, i );
        }
    }
    assert(0);
    return 0;
}

Mig_Obj_t * getFaninByName( char * buf, Abc_Ntk_t * pNtk, Mig_Man_t * pMan, Vec_Ptr_t * vObjs, int del )
{
    int compl;
    Mig_Obj_t * pObj;

    compl = (buf[0] == '~');
    if( compl ) buf++;
    pObj = getObjByName( buf, pNtk, pMan, vObjs, del );
    return Mig_NotCond(pObj, compl);
}


/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mig_Man_t * Mig_ReadMig( char * file, Abc_Ntk_t * pNtk )
{
    Mig_Man_t * pMan;
    Mig_Obj_t * pObj, * p0, * p1, * p2;
    Abc_Obj_t * pAbcObj;
    FILE * pFile;
    Vec_Ptr_t * vObjs;
    char buf[1024];
    char * pTemp;
    int i = 0,  size, k;

    vObjs = Vec_PtrAlloc( 100 );
    pFile = fopen( file, "r" );
    pMan = Mig_MigStart( pNtk );

    while( strcmp( buf, "wire" ) != 0 ) fscanf( pFile, "%s", buf );
    fscanf( pFile, "%s", buf );  assert( strcmp(buf,"one,") == 0 );  // wire one, ...
    do {
        i++;
        fscanf( pFile, "%s", buf );
        pObj = Mig_ObjAlloc( MIG_OBJ_NODE );
        pObj->Id = Vec_PtrSize(vObjs);
        Vec_PtrPush( vObjs, pObj );
    } while( buf[strlen(buf)-1] != ';' );

    for( i = 0, size = Vec_PtrSize(vObjs); i < size; ++i )
    {
        pObj = Vec_PtrEntry( vObjs, i );

        fscanf( pFile, "%s", buf ); assert( strcmp(buf, "assign") == 0 );
        fscanf( pFile, "%s", buf ); // target wire
        fscanf( pFile, "%s", buf ); assert( strcmp(buf, "=" ) == 0 );
        fscanf( pFile, "%s", buf ); 
        if( buf[0] == '(' )  // MAJ type  (~w20 & w31) | (~w20 & w15) | (w31 & w15);
        {
            pTemp = &buf[1];
            p0 = getFaninByName( pTemp, pNtk, pMan, vObjs, 0 );
            fscanf( pFile, "%s", buf );  fscanf( pFile, "%s", buf );
            p1 = getFaninByName( buf, pNtk, pMan, vObjs, 1 );
            fscanf( pFile, "%s", buf );  fscanf( pFile, "%s", buf );
            fscanf( pFile, "%s", buf );  fscanf( pFile, "%s", buf );
            p2 = getFaninByName( buf, pNtk, pMan, vObjs, 1 );
            fscanf( pFile, "%s", buf );  fscanf( pFile, "%s", buf );
            fscanf( pFile, "%s", buf );  fscanf( pFile, "%s", buf );
        }
        else   // AND type
        {
            p1 = getFaninByName( buf, pNtk, pMan, vObjs, 0 );  // ~w20 & w31;
            fscanf( pFile, "%s", buf );
            p0 = (buf[0] == '&') ? Mig_Not(pMan->pConst1) : pMan->pConst1;
            fscanf( pFile, "%s", buf );
            p2 = getFaninByName( buf, pNtk, pMan, vObjs, 1 );
        }
        pObj->pFanin0 = p0; pObj->pFanin1 = p1; pObj->pFanin2 = p2;
    }  // end for each node

    // reading "assign one = 1;"
    fscanf( pFile, "%s", buf ); fscanf( pFile, "%s", buf ); fscanf( pFile, "%s", buf );  fscanf( pFile, "%s", buf );

    Abc_NtkForEachPo( pNtk, pAbcObj, i )  // assign po01 = ~w20;// level 20
    {
        pObj = Mig_ManPo( pMan, i );
        fscanf( pFile, "%s", buf ); fscanf( pFile, "%s", buf ); 
        assert( strcmp( buf, Abc_ObjName(pAbcObj) ) == 0 );
        fscanf( pFile, "%s", buf ); fscanf( pFile, "%s", buf ); 
        pObj->pFanin0 = getFaninByName( buf, pNtk, pMan, vObjs, 3 );
        fscanf( pFile, "%s", buf ); fscanf( pFile, "%s", buf );
        //printf("%d\n", Mig_Regular(pObj->pFanin0)->Id );
    } 
    Mig_ManReassignIds( pMan );

//    printf("%d\n", pMan->nObjCounts[MIG_OBJ_NODE] );
    
    Mig_ManForEachNode( pMan, pObj, i )
    {
        if( Mig_Regular(pObj->pFanin0)->Id > Mig_Regular(pObj->pFanin1)->Id )
            p0 = pObj->pFanin0, pObj->pFanin0 = pObj->pFanin1, pObj->pFanin1 = p0;
        if( Mig_Regular(pObj->pFanin1)->Id > Mig_Regular(pObj->pFanin2)->Id )
            p0 = pObj->pFanin1, pObj->pFanin1 = pObj->pFanin2, pObj->pFanin2 = p0;
        if( Mig_Regular(pObj->pFanin0)->Id > Mig_Regular(pObj->pFanin1)->Id )
            p0 = pObj->pFanin0, pObj->pFanin0 = pObj->pFanin1, pObj->pFanin1 = p0;

        // p0: will replace pObj, p1: pObj's fanout, p2 = Mig_Regular(p0)
        if( (p0 = Mig_MigMajLookup( pMan, pObj->pFanin0, pObj->pFanin1, pObj->pFanin2 )) )
        {
            // replace pObj with p0
            // NOTE: do not check the new fanin combination since it will be checked later
            Vec_PtrClear( pMan->vNodes );
            Mig_ObjForEachFanout( pMan, pObj, p1, k )
            {
                assert( p1 );
                Vec_PtrPush( pMan->vNodes, p1 );
            }
            Vec_PtrForEachEntry( Mig_Obj_t * , pMan->vNodes, p1, k )
            {
                assert( p1 );
                assert( !Mig_IsComplement(p1) );
                p2 = Mig_Regular(p0);
                if( !Vec_IntRemove(&pObj->vFanouts, p1->Id) ) assert(0);
                Vec_IntPush( &p2->vFanouts, p1->Id );

                if( Mig_ObjIsPo(p1) )
                {
                    int fCompl = Mig_IsComplement(p1->pFanin0) ^ Mig_IsComplement(p0);
                    p1->pFanin0 = Mig_NotCond( p2, fCompl );
                    continue;
                }
                if( Mig_Regular(p1->pFanin0) == pObj )
                {
                    int fCompl = Mig_IsComplement(p1->pFanin0) ^ Mig_IsComplement(p0);
                    p1->pFanin0 = Mig_NotCond( p2, fCompl );
                }
                else if( Mig_Regular(p1->pFanin1) == pObj )
                {
                    int fCompl = Mig_IsComplement(p1->pFanin1) ^ Mig_IsComplement(p0);
                    p1->pFanin1 = Mig_NotCond( p2, fCompl );
                }
                else if( Mig_Regular(p1->pFanin2) == pObj )
                {
                    int fCompl = Mig_IsComplement(p1->pFanin2) ^ Mig_IsComplement(p0);
                    p1->pFanin2 = Mig_NotCond( p2, fCompl );
                }
                else 
                    assert(0);
            }
            assert( Mig_ObjFanoutNum(pObj) == 0 );
            p0 = Mig_Regular(pObj->pFanin0);
            p1 = Mig_Regular(pObj->pFanin1);
            p2 = Mig_Regular(pObj->pFanin2);

            // delete the node from its fanin's fanout
            if( !Vec_IntRemove( &p0->vFanouts, pObj->Id ) ) assert(0);
            if( !Vec_IntRemove( &p1->vFanouts, pObj->Id ) ) assert(0);
            if( !Vec_IntRemove( &p2->vFanouts, pObj->Id ) ) assert(0);
            // remove the node from manager
            Vec_PtrWriteEntry( pMan->vObjs, pObj->Id, NULL );
            if( Mig_ObjIsNode(p0) && Mig_ObjFanoutNum(p0) == 0 ) Mig_MigDeleteNode( pMan, p0 );
            if( Mig_ObjIsNode(p1) && Mig_ObjFanoutNum(p1) == 0 ) Mig_MigDeleteNode( pMan, p1 );
            if( Mig_ObjIsNode(p2) && Mig_ObjFanoutNum(p2) == 0 ) Mig_MigDeleteNode( pMan, p2 );
        }
        else
        {
            Mig_HashNode( pMan, pObj, pObj->pFanin0, pObj->pFanin1, pObj->pFanin2 );
        }
    }
    Mig_ManReassignIds( pMan );
//    Mig_Write( pNtk, pMan, "rewrite.v" );
    return pMan;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

