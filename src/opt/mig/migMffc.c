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
void Mig_ManIncTravId( Mig_Man_t * p )
{
    if ( p->nTravIds++ < 0x8FFFFFFF ) {
//        printf("HIHIHI %d\n", p->nTravIds );
        return;
    }

    Mig_Obj_t * pNode;
    int i;
    Mig_ManForEachObj( p, pNode, i )
        pNode->TravId = 0;
    p->nTravIds = 1;
}

int Mig_NodeRefDeref( Mig_Man_t * pMan, Mig_Obj_t * pNode, int fReference, int fLabel )
{
    Mig_Obj_t * pNode0, *pNode1, *pNode2;
    int Counter;

    // label visited nodes
    if( fLabel ) pNode->TravId = pMan->nTravIds;
    // skip the PO
    if( Mig_ObjIsPi(pNode) || Mig_ObjIsConst(pNode) ) return 0;
    // process the internal node
    pNode0 = Mig_Regular(pNode->pFanin0);
    pNode1 = Mig_Regular(pNode->pFanin1);
    pNode2 = Mig_Regular(pNode->pFanin2);
    Counter = 1;
    if( fReference )
    {
        if( pNode0->vFanouts.nSize++ == 0 )  // recover
            Counter += Mig_NodeRefDeref( pMan, pNode0, fReference, fLabel );
        if( pNode1->vFanouts.nSize++ == 0 )  // recover
            Counter += Mig_NodeRefDeref( pMan, pNode1, fReference, fLabel );
        if( pNode2->vFanouts.nSize++ == 0 )  // recover
            Counter += Mig_NodeRefDeref( pMan, pNode2, fReference, fLabel );
    }
    else
    {
        assert( pNode0->vFanouts.nSize > 0 );
        assert( pNode1->vFanouts.nSize > 0 );
        assert( pNode2->vFanouts.nSize > 0 );
        if( --pNode0->vFanouts.nSize == 0 ) // fanout free node
            Counter += Mig_NodeRefDeref( pMan, pNode0, fReference, fLabel );
        if( --pNode1->vFanouts.nSize == 0 ) // fanout free node
            Counter += Mig_NodeRefDeref( pMan, pNode1, fReference, fLabel );
        if( --pNode2->vFanouts.nSize == 0 ) // fanout free node
            Counter += Mig_NodeRefDeref( pMan, pNode2, fReference, fLabel );
    }
    return Counter;
}

int Mig_NodeMffcLabelMig( Mig_Man_t * pMan, Mig_Obj_t * pNode )
{
    int nSize1, nSize2;
    assert(!Mig_IsComplement(pNode));
    assert(Mig_ObjIsNode(pNode));
    if( pNode->pFanin0 == 0 ) return 0;  // floating node
    
    nSize1 = Mig_NodeRefDeref( pMan, pNode, 0, 1 );
    nSize2 = Mig_NodeRefDeref( pMan, pNode, 1, 0 );
    assert( nSize1 == nSize2 );
    assert( nSize1 > 0 );
    return nSize1;
}

// NOTE: leaves won't be collected
// nodes will be collected in dfs order
void Mig_CollectNodes( Mig_Man_t * pMan, Mig_Obj_t * pNode, Vec_Ptr_t * vNodes )
{
    Mig_ObjSetTravIdCurrent( pMan, pNode );
    if( Mig_ObjIsPi(pNode) || Mig_ObjIsConst(pNode) ) 
    {
        Vec_PtrPush( vNodes, pNode );
        return;
    }
    assert( Mig_ObjIsNode(pNode) );
    if( !Mig_ObjIsTravIdCurrent( pMan, Mig_Regular(pNode->pFanin0) ) ) Mig_CollectNodes( pMan, Mig_Regular(pNode->pFanin0), vNodes );
    if( !Mig_ObjIsTravIdCurrent( pMan, Mig_Regular(pNode->pFanin1) ) ) Mig_CollectNodes( pMan, Mig_Regular(pNode->pFanin1), vNodes );
    if( !Mig_ObjIsTravIdCurrent( pMan, Mig_Regular(pNode->pFanin2) ) ) Mig_CollectNodes( pMan, Mig_Regular(pNode->pFanin2), vNodes );
    Vec_PtrPush( vNodes, pNode );
}

////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

