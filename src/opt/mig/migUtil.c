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
void Mig_PrintNtkInfo( Mig_Man_t * pMan )
{

}
void Mig_Print( Mig_Man_t * m)
{
    int i , k;
    Mig_Obj_t * mNode, * fanout;
    Mig_ManForEachObj( m, mNode, i )
    {
        printf("Node Id: %d -> ", mNode->Id );
        if( Mig_ObjIsPi(mNode) || Mig_ObjIsConst(mNode) ) { printf("\n"); continue; }
        if(Mig_IsComplement(mNode->pFanin0)) printf("~");
        printf("%d ", Mig_Regular(mNode->pFanin0)->Id );
        if( !Mig_ObjIsNode(mNode) ) { printf("\n"); continue; }
        if(Mig_IsComplement(mNode->pFanin1)) printf("~");
        printf("%d ", Mig_Regular(mNode->pFanin1)->Id );
        if(Mig_IsComplement(mNode->pFanin2)) printf("~");
        printf("%d ", Mig_Regular(mNode->pFanin2)->Id );

        printf(" Fanout: ");
        Mig_ObjForEachFanout( m, mNode, fanout, k )
            printf("%d ", fanout->Id);
        printf("\n");
    }
}

void Mig_PrintHashTable( Mig_Man_t * m )
{
    int i;
    Mig_Obj_t * mNode;
    for ( i = 0; i < m->nTableSize; i++ )
    {
      Mig_MigBinForEachEntry( m->pTable[i], mNode )
      {
          printf("%d ", mNode->Id);
      }
      printf("\n");
    }
}

void Mig_PrintCuts( Cut_Man_t * pCutMan, Mig_Man_t * pMan )
{
    Cut_Cut_t * pList;
    Mig_Obj_t * pObj;
    int i;
    printf( "Cuts of the network:\n" );
    Mig_ManForEachObj( pMan, pObj, i )
    {
        pList = Cut_NodeReadCutsNew( pCutMan, Mig_ObjId(pObj) );
        printf("Node %d:\n", Mig_ObjId(pObj) );
        Cut_CutPrintList(pList, 0);
    }

}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

