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

  Description [ The fanin0 must not be negated and checking the hash table ]
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Mig_InvertCanonical( Mig_Man_t * pMan )
{
    Mig_Obj_t * pNode, * pMaj, * pFanin, *pFanout;
    Mig_Obj_t * pFanin0, *pFanin1, *pFanin2;
    Mig_Obj_t ** pTableNew;
    int i, k;
    int fCompl;

    Mig_ManForEachNode( pMan, pNode, i )
    {
        if( ! Mig_IsComplement(pNode->pFanin0) ) continue;
        // negate fanin
        pNode->pFanin0 = Mig_Not(pNode->pFanin0);
        pNode->pFanin1 = Mig_Not(pNode->pFanin1);
        pNode->pFanin2 = Mig_Not(pNode->pFanin2);
        // negate fanout
        Mig_ObjForEachFanout( pMan, pNode, pFanout, k )  
        {
            assert( !Mig_IsComplement(pFanout) );
            if( Mig_ObjIsPo(pFanout) ) 
            {
                assert( Mig_Regular(pFanout->pFanin0) == pNode );
                pFanout->pFanin0 = Mig_Not(pFanout->pFanin0);
                continue;
            }
            if( Mig_Regular(pFanout->pFanin0) == pNode ) {
                pFanout->pFanin0 = Mig_Not(pFanout->pFanin0);
                continue;
            }
            else if( Mig_Regular(pFanout->pFanin1) == pNode ) {
                pFanout->pFanin1 = Mig_Not(pFanout->pFanin1);
                continue;
            }
            else if( Mig_Regular(pFanout->pFanin2) == pNode ) {
                pFanout->pFanin2 = Mig_Not(pFanout->pFanin2);
                continue;
            }
            else 
                assert(0);
        }
    }
    // update hash table and check the nodes with same fanin
    pTableNew = ABC_ALLOC( Mig_Obj_t *, pMan->nTableSize );
    memset( pTableNew, 0, sizeof(Mig_Obj_t*) * pMan->nTableSize );
    ABC_FREE( pMan->pTable );
    pMan->pTable = pTableNew;
    pMan->nEntries = 0;

    Mig_ManForEachNode( pMan, pNode, i )  // topological order
    {
//printf("%d\n", pNode->Id );
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
        else  // new kind of fanins -> hash into the table
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

