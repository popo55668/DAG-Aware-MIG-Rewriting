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

static void Mig_ObjDeleteFanin( Mig_Obj_t * pObj, Mig_Obj_t * pFanin );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    []

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Mig_Obj_t * Mig_ObjAlloc( Mig_ObjType_t type)
{
    Mig_Obj_t * pObj = (Mig_Obj_t*) ABC_ALLOC( Mig_Obj_t, 1 );
    memset( pObj, 0, sizeof(Mig_Obj_t) );
    pObj->Type = type;
    pObj->Id = -1;
    pObj->pFanin0 = 0;
    pObj->pFanin1 = 0;
    pObj->pFanin2 = 0;
    pObj->TravId = 0;
    pObj->fMarkA = 0;
    pObj->fMarkB = 0;
    pObj->isFix  = 0;
    pObj->simValue = 0;
    return pObj;
}

void Mig_ObjRemoveFanins( Mig_Obj_t * pObj )
{
    Mig_Obj_t * pFanin;

    pFanin = Mig_Regular(pObj->pFanin0);  Mig_ObjDeleteFanin( pObj, pFanin );
    pFanin = Mig_Regular(pObj->pFanin1);  Mig_ObjDeleteFanin( pObj, pFanin );
    pFanin = Mig_Regular(pObj->pFanin2);  Mig_ObjDeleteFanin( pObj, pFanin );
}

void Mig_ObjDeleteFanin( Mig_Obj_t * pObj, Mig_Obj_t * pFanin )
{
    assert( !Mig_IsComplement(pObj) );
    assert( !Mig_IsComplement(pFanin) );
    assert( pObj->Id >= 0 && pFanin->Id >= 0 );
    
    if( !Vec_IntRemove( &pFanin->vFanouts, pObj->Id ) ) assert(0);
}

////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

