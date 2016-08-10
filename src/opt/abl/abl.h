/**CFile****************************************************************

  FileName    [.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName []

  Synopsis    [External declarations.]

  Author      [Li-Wei Wang]
  
  Affiliation [NTU]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: .h,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__ABL_h
#define ABC__ABL_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/abc/abc.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_HEADER_START

typedef enum {
    ABL_OBJ_NONE = 0,  // 0
    ABL_OBJ_CONST1,    // 1
    ABL_OBJ_PI,        // 2
    ABL_OBJ_PO,        // 3
    ABL_OBJ_MIG,       // 4
    ABL_OBJ_XOR,       // 5
    ABL_OBJ_NUMBER     // 6
} Abl_ObjType_t;

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Abl_Obj_t_ Abl_Obj_t;
typedef struct Abl_Man_t_ Abl_Man_t;

struct Abl_Obj_t_
{
    Abl_Obj_t *       pNext;
    int               Id;
    unsigned          Type   :   3;
    unsigned          fCompl0:   1;
    unsigned          fCompl1:   1;
    unsigned          fCompl2:   1;
    unsigned          fPhase :   1;  // the flag to mark the phase of eq node
    unsigned          Level  :  25;
    Vec_Int_t         vFanins;
};

struct Abl_Man_t_
{
    Vec_Ptr_t *       vObjs;
    Vec_Ptr_t *       vPis;
    Vec_Ptr_t *       vPos;
    Abl_Obj_t *       pConst1;
    Abl_Obj_t **      xorBins;
    Abl_Obj_t **      majBins;
    int               nXorBins;
    int               nMajBins;
    int               nEntries;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////
static inline Abl_Obj_t * Abl_ManObj( Abl_Man_t * pAbl, int i ) { return (Abl_Obj_t *)Vec_PtrEntry( pAbl->vObjs, i ); }
static inline Abl_Obj_t * Abl_ManPi( Abl_Man_t * pAbl, int i ) { return (Abl_Obj_t *)Vec_PtrEntry( pAbl->vPis, i); }
static inline Abl_Obj_t * Abl_ManPo( Abl_Man_t * pAbl, int i ) { return (Abl_Obj_t *)Vec_PtrEntry( pAbl->vPos, i); }

static inline int Abl_ObjFaninNum( Abl_Obj_t * pObj ) { return pObj->vFanins.nSize; }
static inline int Abl_ObjFaninId( Abl_Obj_t * pObj, int i ) { return pObj->vFanins.pArray[i]; }

static inline int Abl_ManObjNum( Abl_Man_t * pAbl ) { return Vec_PtrSize(pAbl->vObjs); }
static inline int Abl_ManPiNum( Abl_Man_t * pAbl ) { return Vec_PtrSize(pAbl->vPis); }
static inline int Abl_ManPoNum( Abl_Man_t * pAbl ) { return Vec_PtrSize(pAbl->vPos); }


static inline unsigned Abl_ObjType( Abl_Obj_t * pObj ) { return pObj->Type; }
static inline unsigned Abl_ObjId( Abl_Obj_t * pObj ) { return pObj->Id; }

static inline unsigned Abl_ObjIsPi( Abl_Obj_t * pObj ) { return pObj->Type == ABL_OBJ_PI; }
static inline unsigned Abl_ObjIsPo( Abl_Obj_t * pObj ) { return pObj->Type == ABL_OBJ_PO; }
static inline unsigned Abl_ObjIsMaj( Abl_Obj_t * pObj ) { return pObj->Type == ABL_OBJ_MAJ; }
static inline unsigned Abl_ObjIsXor( Abl_Obj_t * pObj ) { return pObj->Type == ABL_OBJ_XOR; }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                          ITERATORs                               ///
////////////////////////////////////////////////////////////////////////
#define Abl_ManForEachObj( pAbl, pObj, i )                             \
    for( i = 0; (i < Abl_ManObjNum(pAbl)) && (((pObj)=Abl_ManObj(pAbl,i)),1); i++) \
        if( (pObj) == NULL ) { } else
#define Abl_ManForEachPi( pAbl, pPi, i )                               \
    for( i = 0; (i < Abl_ManPiNum(pAbl)) && (((pPi)=Abl_ManPi(pAbl,i)),1); i++)
#define Abl_ManForEachPo( pAbl, pPo, i )                               \
    for( i = 0; (i < Abl_ManPoNum(pAbl)) && (((pPo)=Abl_ManPo(pAbl,i)),1); i++)


ABC_NAMESPACE_HEADER_END

#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

