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

// hashing the node
static unsigned Mig_HashKey( Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2, int TableSize )
{
    unsigned Key = 0;
    Key ^= Mig_Regular(p0)->Id * 23813;
    Key ^= Mig_Regular(p1)->Id * 7937;
    Key ^= Mig_Regular(p2)->Id * 2971;
    Key ^= Mig_IsComplement(p0) * 911;
    Key ^= Mig_IsComplement(p1) * 353;
    Key ^= Mig_IsComplement(p2) * 113;
    return Key % TableSize;
}

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mig_Obj_t * Mig_MigMajLookup( Mig_Man_t * pMan, Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2 )
{
    Mig_Obj_t * pMaj;
    unsigned key;
    if( p0 == p1 ) return p0;
    if( p0 == p2 ) return p0;
    if( p1 == p2 ) return p1;
    if( p0 == Mig_Not(p1) ) return p2;
    if( p0 == Mig_Not(p2) ) return p1;
    if( p1 == Mig_Not(p2) ) return p0;

    if( Mig_Regular(p0)->Id > Mig_Regular(p1)->Id )
        pMaj = p0, p0 = p1, p1 = pMaj;
    if( Mig_Regular(p1)->Id > Mig_Regular(p2)->Id )
        pMaj = p1, p1 = p2, p2 = pMaj;
    if( Mig_Regular(p0)->Id > Mig_Regular(p1)->Id )
        pMaj = p0, p0 = p1, p1 = pMaj;
    assert( Mig_Regular(p0)->Id < Mig_Regular(p1)->Id );
    assert( Mig_Regular(p1)->Id < Mig_Regular(p2)->Id );

    key = Mig_HashKey( p0, p1, p2, pMan->nTableSize );
    Mig_MigBinForEachEntry( pMan->pTable[key], pMaj )
        if( p0 == pMaj->pFanin0 && p1 == pMaj->pFanin1 && p2 == pMaj->pFanin2 )
            return pMaj;
    return NULL;
}

/**Function*************************************************************

  Synopsis    [Insert a node into the table]

  Description [Should make sure the fanin combination is not in the table]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mig_HashNode( Mig_Man_t * pMan, Mig_Obj_t * pNode, Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2 )
{
    Mig_Obj_t * pMaj;
    unsigned key;

    if( Mig_Regular(p0)->Id > Mig_Regular(p1)->Id )
        pMaj = p0, p0 = p1, p1 = pMaj;
    if( Mig_Regular(p1)->Id > Mig_Regular(p2)->Id )
        pMaj = p1, p1 = p2, p2 = pMaj;
    if( Mig_Regular(p0)->Id > Mig_Regular(p1)->Id )
        pMaj = p0, p0 = p1, p1 = pMaj;

    assert( Mig_MigMajLookup( pMan, p0, p1, p2 ) == NULL );

    key = Mig_HashKey( p0, p1, p2 , pMan->nTableSize );
    pNode->pNext = pMan->pTable[key];
    pMan->pTable[key] = pNode;
    pMan->nEntries++;
}

/**Function*************************************************************

  Synopsis    [Create a node and insert into table]

  Description [1. not pushed to vObjs 2.forcely insert into table]
               
  SideEffects [ p0, p1, p2 will be sorted ]

  SeeAlso     []

***********************************************************************/
Mig_Obj_t * Mig_MigMajCreateWithoutId( Mig_Man_t * pMan,  Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2 ) 
{
    Mig_Obj_t * pMaj;
    unsigned key;
    
    if( Mig_Regular(p0)->Id > Mig_Regular(p1)->Id )
        pMaj = p0, p0 = p1, p1 = pMaj;
    if( Mig_Regular(p1)->Id > Mig_Regular(p2)->Id )
        pMaj = p1, p1 = p2, p2 = pMaj;
    if( Mig_Regular(p0)->Id > Mig_Regular(p1)->Id )
        pMaj = p0, p0 = p1, p1 = pMaj;
    // Note: pMaj->Id = -1, Therefore, fanout is not added 
    pMaj = Mig_ObjAlloc( MIG_OBJ_NODE );
    pMaj->pFanin0 = p0;
    pMaj->pFanin1 = p1;
    pMaj->pFanin2 = p2;
    // level
    pMaj->Level = 1 + Abc_MaxInt( Mig_Regular(p0)->Level, Abc_MaxInt(Mig_Regular(p1)->Level, Mig_Regular(p2)->Level) );
    // hash table
    key = Mig_HashKey( p0, p1, p2 , pMan->nTableSize );
    pMaj->pNext = pMan->pTable[key];
    pMan->pTable[key] = pMaj;
    pMan->nEntries++;
    return pMaj;
}


/**Function*************************************************************

  Synopsis    [ Create a new node and push to vObjs ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mig_Obj_t * Mig_MigMajCreate( Mig_Man_t * pMan,  Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2 ) 
{
    Mig_Obj_t * pMaj;
    unsigned key;
    
    if( Mig_Regular(p0)->Id > Mig_Regular(p1)->Id )
        pMaj = p0, p0 = p1, p1 = pMaj;
    if( Mig_Regular(p1)->Id > Mig_Regular(p2)->Id )
        pMaj = p1, p1 = p2, p2 = pMaj;
    if( Mig_Regular(p0)->Id > Mig_Regular(p1)->Id )
        pMaj = p0, p0 = p1, p1 = pMaj;
    pMaj = Mig_ObjAlloc( MIG_OBJ_NODE );
    pMaj->Id = pMan->vObjs->nSize;
    Vec_PtrPush( pMan->vObjs, pMaj );
    pMaj->pFanin0 = p0;  pMaj->pFanin1 = p1;  pMaj->pFanin2 = p2;
    Vec_IntPush( &Mig_Regular(p0)->vFanouts, pMaj->Id );
    Vec_IntPush( &Mig_Regular(p1)->vFanouts, pMaj->Id );
    Vec_IntPush( &Mig_Regular(p2)->vFanouts, pMaj->Id );
    // level
    pMaj->Level = 1 + Abc_MaxInt( Mig_Regular(p0)->Level, Abc_MaxInt(Mig_Regular(p1)->Level, Mig_Regular(p2)->Level) );
    // hash table
    key = Mig_HashKey( p0, p1, p2 , pMan->nTableSize );
    pMaj->pNext = pMan->pTable[key];
    pMan->pTable[key] = pMaj;
    pMan->nEntries++;
    return pMaj;
}

/**Function*************************************************************

  Synopsis    [ Update fanin combination and lookup table ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mig_Obj_t * Mig_MigMajCreateFrom( Mig_Man_t * pMan,  Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2 , Mig_Obj_t* pMaj) 
{
    Mig_Obj_t * pTemp;
    unsigned key;
    if( Mig_Regular(p0)->Id > Mig_Regular(p1)->Id )
        pTemp = p0, p0 = p1, p1 = pTemp;
    if( Mig_Regular(p1)->Id > Mig_Regular(p2)->Id )
        pTemp = p1, p1 = p2, p2 = pTemp;
    if( Mig_Regular(p0)->Id > Mig_Regular(p1)->Id )
        pTemp = p0, p0 = p1, p1 = pTemp;
//    pMaj->Id = pMan->vObjs->nSize;
//    Vec_PtrPush( pMan->vObjs, pMaj );
    pMaj->pFanin0 = p0;  pMaj->pFanin1 = p1;  pMaj->pFanin2 = p2;
    Vec_IntPush( &Mig_Regular(p0)->vFanouts, pMaj->Id );
    Vec_IntPush( &Mig_Regular(p1)->vFanouts, pMaj->Id );
    Vec_IntPush( &Mig_Regular(p2)->vFanouts, pMaj->Id );
    // level
    pMaj->Level = 1 + Abc_MaxInt( Mig_Regular(p0)->Level, Abc_MaxInt(Mig_Regular(p1)->Level, Mig_Regular(p2)->Level) );
    // hash table
    key = Mig_HashKey( p0, p1, p2 , pMan->nTableSize );
    pMaj->pNext = pMan->pTable[key];
    pMan->pTable[key] = pMaj;
    pMan->nEntries++;
    return pMaj;
}

/**Function*************************************************************

  Synopsis    [ Delete a node and it fanout-free cone ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mig_MigDeleteNode( Mig_Man_t * pMan, Mig_Obj_t * pNode ) 
{
    Mig_Obj_t * pTemp, *pFanin0, *pFanin1, *pFanin2 ;
    int i, k;

    assert(!Mig_IsComplement(pNode) );
    assert( Mig_ObjIsNode(pNode) );
    assert( Mig_ObjFanoutNum(pNode) == 0 ); // dangling

    // when deleting and old node scheduled for replacement, remove it from the stack
    Vec_PtrForEachEntry( Mig_Obj_t *, pMan->vStackReplaceOld, pTemp, i )
        if ( pNode == pTemp )
        {
            for ( k = i; k < pMan->vStackReplaceOld->nSize - 1; k++ )
            {
                pMan->vStackReplaceOld->pArray[k] = pMan->vStackReplaceOld->pArray[k+1];
                pMan->vStackReplaceNew->pArray[k] = pMan->vStackReplaceNew->pArray[k+1];
            }
            pMan->vStackReplaceOld->nSize--;
            pMan->vStackReplaceNew->nSize--;
        }
    // when deleting a new node that should replace another node, do not delte
    Vec_PtrForEachEntry( Mig_Obj_t *, pMan->vStackReplaceNew, pTemp, i  )
        if ( pNode == Mig_Regular(pTemp) ) return;

    Mig_MigDelete( pMan, pNode );  // remove from hash table
    
    // remove from level structure
    if( pNode->fMarkA )
        Mig_ManRemoveFromLevelStructure( pMan->vLevels, pNode );
    if( pNode->fMarkB )
        Mig_ManRemoveFromLevelStructureR( pMan->vLevelsR, pMan, pNode );

    // call recursively for fanin cone
    pFanin0 = Mig_Regular(pNode->pFanin0);
    pFanin1 = Mig_Regular(pNode->pFanin1);
    pFanin2 = Mig_Regular(pNode->pFanin2);
    if( !Vec_IntRemove( &pFanin0->vFanouts, pNode->Id ) ) assert(0);
    if( !Vec_IntRemove( &pFanin1->vFanouts, pNode->Id ) ) assert(0);
    if( !Vec_IntRemove( &pFanin2->vFanouts, pNode->Id ) ) assert(0);

    // remove the node from the man, memory is not recycled
    Vec_PtrWriteEntry( pMan->vObjs, pNode->Id, NULL );

    if( Mig_ObjIsNode(pFanin0) && Mig_ObjFanoutNum(pFanin0) == 0 && Mig_ManObj( pMan, pFanin0->Id ) )
        Mig_MigDeleteNode( pMan, pFanin0 );
    if( Mig_ObjIsNode(pFanin1) && Mig_ObjFanoutNum(pFanin1) == 0 && Mig_ManObj( pMan, pFanin1->Id ) )
        Mig_MigDeleteNode( pMan, pFanin1 );
    if( Mig_ObjIsNode(pFanin2) && Mig_ObjFanoutNum(pFanin2) == 0 && Mig_ManObj( pMan, pFanin2->Id ) )
        Mig_MigDeleteNode( pMan, pFanin2 );
}

/**Function*************************************************************

  Synopsis    [ Delete a node from the lookup table ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mig_MigDelete( Mig_Man_t * pMan, Mig_Obj_t * pNode ) 
{
    Mig_Obj_t * pMaj, *pFanin0, *pFanin1, *pFanin2, **ppPlace;
    unsigned key;
    assert(!Mig_IsComplement(pNode) );
    assert( Mig_ObjIsNode(pNode) );

    pFanin0 = pNode->pFanin0;
    pFanin1 = pNode->pFanin1;
    pFanin2 = pNode->pFanin2;
    key = Mig_HashKey( pFanin0, pFanin1, pFanin2, pMan->nTableSize );
    // find the matching node in the table
    ppPlace = pMan->pTable + key;
    Mig_MigBinForEachEntry( pMan->pTable[key], pMaj )
    {
        if ( pMaj != pNode )
        {
            ppPlace = &pMaj->pNext;
            continue;
        }
        *ppPlace = pMaj->pNext;
        break;
    }
    //if( pMaj == pNode ) pMan->nEntries--;
    //else printf("Warning: deleted node is not in hash table\n");

    assert( pMaj == pNode );
    pMan->nEntries--;
}

/**Function*************************************************************

  Synopsis    [ Either newly created or lookuped ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mig_Obj_t * Mig_MigMaj( Mig_Man_t * pMan,  Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2 )
{
    Mig_Obj_t * pMaj;
    if( (pMaj = Mig_MigMajLookup( pMan, p0, p1, p2 )) ) return pMaj;
    return Mig_MigMajCreate( pMan, p0, p1, p2 );
}

/**Function*************************************************************

  Synopsis    [ Rehash the lookup table ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mig_MigRehash( Mig_Man_t * pMan )  
{
    Mig_Obj_t ** pTableNew;
    Mig_Obj_t * pEnt, * pEnt2;
    unsigned key;
    int counter, i;

    pTableNew = ABC_ALLOC( Mig_Obj_t *, pMan->nTableSize );
    memset( pTableNew, 0, sizeof(Mig_Obj_t*) * pMan->nTableSize );

    counter = 0;
    for ( i = 0; i < pMan->nTableSize; i++ )
        Mig_MigBinForEachEntrySafe( pMan->pTable[i], pEnt, pEnt2 )
        {
            key = Mig_HashKey( pEnt->pFanin0, pEnt->pFanin1, pEnt->pFanin2, pMan->nTableSize );
            pEnt->pNext = pTableNew[key];
            pTableNew[key] = pEnt;
            counter++;
        }
    assert( counter == pMan->nEntries );
    ABC_FREE( pMan->pTable );
    pMan->pTable = pTableNew;
}

/**Function*************************************************************

  Synopsis    [ structure hashing ]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mig_MigStrash( Mig_Man_t * pMan ) 
{
    int i, k, fCompl;
    Mig_Obj_t * pNode, * pMaj, * pFanout;
    Mig_Obj_t * pFanin0, * pFanin1, * pFanin2, * pFanin;

    // update hash table and check the nodes with same fanin
    Mig_Obj_t ** pTableNew = ABC_ALLOC( Mig_Obj_t *, pMan->nTableSize );
    memset( pTableNew, 0, sizeof(Mig_Obj_t*) * pMan->nTableSize );
    ABC_FREE( pMan->pTable );
    pMan->pTable = pTableNew;
    pMan->nEntries = 0;

    Mig_ManForEachNode( pMan, pNode, i )
    {
        if( (pMaj = Mig_MigMajLookup( pMan, pNode->pFanin0, pNode->pFanin1, pNode->pFanin2 )) )
        {
            // replace pNode with pMaj
            // NOTE: do not check the new fanin combination since it will be checked later
            Vec_PtrClear( pMan->vNodes );
            Mig_ObjForEachFanout( pMan, pNode, pFanout, k )
            {
                assert( pFanout );
                Vec_PtrPush( pMan->vNodes, pFanout );
            }
            Vec_PtrForEachEntry( Mig_Obj_t * , pMan->vNodes, pFanout, k )
            {
                assert( pFanout );
                assert( !Mig_IsComplement(pFanout) );
                pFanin = Mig_Regular(pMaj);
                if( !Vec_IntRemove(&pNode->vFanouts, pFanout->Id) ) assert(0); // pNode->nFanout--
                Vec_IntPush( &pFanin->vFanouts, pFanout->Id );

                if( Mig_ObjIsPo(pFanout) ) 
                {
                    fCompl = Mig_IsComplement(pFanout->pFanin0) ^ Mig_IsComplement(pMaj);
                    pFanout->pFanin0 = Mig_NotCond( pFanin, fCompl );
                    continue;
                }
                if( Mig_Regular(pFanout->pFanin0) == pNode )
                {
                    fCompl = Mig_IsComplement(pFanout->pFanin0) ^ Mig_IsComplement(pMaj);
                    pFanout->pFanin0 = Mig_NotCond( pFanin, fCompl );
                }
                else if( Mig_Regular(pFanout->pFanin1) == pNode )
                {
                    fCompl = Mig_IsComplement(pFanout->pFanin1) ^ Mig_IsComplement(pMaj);
                    pFanout->pFanin1 = Mig_NotCond( pFanin, fCompl );
                }
                else if( Mig_Regular(pFanout->pFanin2) == pNode )
                {
                    fCompl = Mig_IsComplement(pFanout->pFanin2) ^ Mig_IsComplement(pMaj);
                    pFanout->pFanin2 = Mig_NotCond( pFanin, fCompl );
                }
                else
                    assert(0);
            }
            assert( Mig_ObjFanoutNum(pNode) == 0 );
            pFanin0 = Mig_Regular(pNode->pFanin0);
            pFanin1 = Mig_Regular(pNode->pFanin1);
            pFanin2 = Mig_Regular(pNode->pFanin2);
            
            // delete pNode
            if( !Vec_IntRemove( &pFanin0->vFanouts, pNode->Id ) ) assert(0);
            if( !Vec_IntRemove( &pFanin1->vFanouts, pNode->Id ) ) assert(0);
            if( !Vec_IntRemove( &pFanin2->vFanouts, pNode->Id ) ) assert(0);
            // remove the node from the man
            Vec_PtrWriteEntry( pMan->vObjs, pNode->Id, NULL );
            // call recursively for fanin cone
            // NOTE: fanin node must in the hash table
            if( Mig_ObjIsNode(pFanin0) && Mig_ObjFanoutNum(pFanin0) == 0 && Mig_ManObj( pMan, pFanin0->Id ) )
                Mig_MigDeleteNode( pMan, pFanin0 );
            if( Mig_ObjIsNode(pFanin1) && Mig_ObjFanoutNum(pFanin1) == 0 && Mig_ManObj( pMan, pFanin1->Id ) )
                Mig_MigDeleteNode( pMan, pFanin1 );
            if( Mig_ObjIsNode(pFanin2) && Mig_ObjFanoutNum(pFanin2) == 0 && Mig_ManObj( pMan, pFanin2->Id ) )
                Mig_MigDeleteNode( pMan, pFanin2 );
        }
        else
        {
            Mig_HashNode( pMan, pNode, pNode->pFanin0, pNode->pFanin1, pNode->pFanin2 );
        }
    }
    Mig_ManReassignIds( pMan );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

