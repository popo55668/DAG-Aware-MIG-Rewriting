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

#define NUMBER1 3716960521u
#define NUMBER2 2174103536u

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
unsigned my_random(int fReset )
{
    static unsigned int m_z = NUMBER1;
    static unsigned int m_w = NUMBER2;
    if( fReset )
    {
        m_z = NUMBER1;
        m_w = NUMBER2;
    }
    m_z = 36969 * (m_z & 65535 ) + (m_z >> 16);
    m_w = 18000 * (m_w & 65535 ) + (m_w >> 16);
    return (m_z << 16) + m_w;
}

unsigned Mig_SimKey(unsigned simValue, int TableSize)
{
    unsigned key = simValue;
    return key % TableSize;
}

void simulate( Mig_Man_t * pMan ) 
{
    Mig_Obj_t * pNode;
    int i;
    unsigned s0, s1, s2, key;

    Vec_VecClear( pMan->pSimTable );

    pMan->pConst1->simValue = 0xFFFFFFFF;
    Mig_ManForEachPi( pMan, pNode, i )
        pNode->simValue = my_random(0);

    Mig_ManForEachNode( pMan, pNode, i ) 
    {
        s0 = Mig_Regular(pNode->pFanin0)->simValue;
        if( Mig_IsComplement(pNode->pFanin0) ) s0 = ~s0;
        s1 = Mig_Regular(pNode->pFanin1)->simValue;
        if( Mig_IsComplement(pNode->pFanin1) ) s1 = ~s1;
        s2 = Mig_Regular(pNode->pFanin2)->simValue;
        if( Mig_IsComplement(pNode->pFanin2) ) s2 = ~s2;

        pNode->simValue = ( s0 & s1 ) | ( s0 & s2 ) | ( s1 & s2 );
        key = Mig_SimKey( pNode->simValue, pMan->nTableSize );
        Vec_VecPush( pMan->pSimTable, key, pNode );
    }
    /* print simTable
    Vec_Ptr_t * vVec;
    int j;
    Vec_VecForEachLevel( pMan->pSimTable, vVec, i )
    {
        Vec_PtrForEachEntry( Mig_Obj_t *, vVec, pNode, j ) 
            printf("%d ", pNode->Id);
        if(Vec_PtrSize(vVec))printf("\n");
    }*/
}

////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

