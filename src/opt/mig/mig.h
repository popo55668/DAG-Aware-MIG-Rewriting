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
 
#ifndef ABC__MIG_h
#define ABC__MIG_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/abc/abc.h"
#include "opt/cut/cut.h"
#include "opt/cut/cutInt.h"
#include "opt/cut/cutList.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_HEADER_START

typedef enum {
    MIG_OBJ_NONE = 0,  // 0
    MIG_OBJ_CONST1,    // 1
    MIG_OBJ_PI,        // 2
    MIG_OBJ_PO,        // 3
    MIG_OBJ_NODE,      // 4
    MIG_OBJ_NUMBER     // 5
} Mig_ObjType_t;

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Mig_Obj_t_ Mig_Obj_t;
typedef struct Mig_Man_t_ Mig_Man_t;

struct Mig_Obj_t_
{
    Mig_Obj_t *       pNext;
    Mig_Obj_t *       pFanin0;
    Mig_Obj_t *       pFanin1;
    Mig_Obj_t *       pFanin2;
    int               Id;
    unsigned          Type     :   3;
    unsigned          fPhase   :   1;
    unsigned          fMarkA   :   1;  // both fMarkA, fMarkB are used in Mig_Replace
    unsigned          fMarkB   :   1;  // they can not be used as flag
    unsigned          isFix    :   1;
    unsigned          target   :   1;
    unsigned          Level    :  24;
    unsigned          simValue;
    Vec_Int_t         vFanouts;
    int               TravId;
    union { void *    pData;
            unsigned  uTruth; };  // truth table manipulation
};

struct Mig_Man_t_
{
    Vec_Ptr_t *       vObjs;      // CONST | PI | NODE | PO
    Vec_Ptr_t *       vPis;
    Vec_Ptr_t *       vPos;
    Vec_Vec_t *       pSimTable;          // FEC table
    Mig_Obj_t *       pConst1;
    Mig_Obj_t **      pTable;
    // temporary storage
    Vec_Ptr_t *       vNodes;             // temporary array
    Vec_Ptr_t *       vStackReplaceOld;   // the nodes to be replaced
    Vec_Ptr_t *       vStackReplaceNew;   // the nodes to be used for replacement
    Vec_Vec_t *       vLevels;            // the nodes to be updated for level
    Vec_Vec_t *       vLevelsR;           // the nodes to be updated for level 
    // rewriting
    Vec_Vec_t *       vChains;            // carry chain
    Vec_Ptr_t *       vTarget;            // target carry-out node to recover
    Vec_Int_t *       vLevelsReverse;     // the reversed level
    int               nObjCounts[MIG_OBJ_NUMBER];
    int               nTableSize;
    int               nEntries;   // the total number of entries in the table
    int               nTravIds;
    int               LevelMax;
};

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////
static inline Mig_Obj_t * Mig_Regular( Mig_Obj_t * p ) { return (Mig_Obj_t*)((ABC_PTRUINT_T)(p) & ~01); }
static inline Mig_Obj_t * Mig_Not( Mig_Obj_t * p ) { return (Mig_Obj_t*)((ABC_PTRUINT_T)(p) ^ 01); }
static inline Mig_Obj_t * Mig_NotCond( Mig_Obj_t * p, int c ) { return (Mig_Obj_t*)((ABC_PTRUINT_T)(p) ^ (c)); }
static inline int Mig_IsComplement( Mig_Obj_t * p ) { return (int)((ABC_PTRUINT_T)(p) & 01); }

static inline Mig_Obj_t * Mig_ManObj( Mig_Man_t * pMig, int i ) { return (Mig_Obj_t *)Vec_PtrEntry( pMig->vObjs, i ); }
static inline Mig_Obj_t * Mig_ManPi( Mig_Man_t * pMig, int i ) { return (Mig_Obj_t *)Vec_PtrEntry( pMig->vPis, i); }
static inline Mig_Obj_t * Mig_ManPo( Mig_Man_t * pMig, int i ) { return (Mig_Obj_t *)Vec_PtrEntry( pMig->vPos, i); }

// Mig_Obj_t
static inline int Mig_ManObjNum( Mig_Man_t * pMig ) { return Vec_PtrSize(pMig->vObjs); }
static inline int Mig_ManPiNum( Mig_Man_t * pMig ) { return Vec_PtrSize(pMig->vPis); }
static inline int Mig_ManPoNum( Mig_Man_t * pMig ) { return Vec_PtrSize(pMig->vPos); }

static inline unsigned Mig_ObjId( Mig_Obj_t * pObj ) { return pObj->Id; }

// Obj Type
static inline unsigned Mig_ObjType( Mig_Obj_t * pObj ) { return pObj->Type; }
static inline unsigned Mig_ObjIsConst( Mig_Obj_t * pObj ) { return pObj->Type == MIG_OBJ_CONST1; }
static inline unsigned Mig_ObjIsPi( Mig_Obj_t * pObj ) { return pObj->Type == MIG_OBJ_PI; }
static inline unsigned Mig_ObjIsPo( Mig_Obj_t * pObj ) { return pObj->Type == MIG_OBJ_PO; }
static inline unsigned Mig_ObjIsNode( Mig_Obj_t * pObj ) { return pObj->Type == MIG_OBJ_NODE; }

// Obj Fanin
static inline Mig_Obj_t * Mig_ObjFanin0( Mig_Obj_t * pObj ) { return Mig_Regular(pObj->pFanin0); }
static inline Mig_Obj_t * Mig_ObjFanin1( Mig_Obj_t * pObj ) { return Mig_Regular(pObj->pFanin1); }
static inline Mig_Obj_t * Mig_ObjFanin2( Mig_Obj_t * pObj ) { return Mig_Regular(pObj->pFanin2); }
static inline int Mig_ObjFaninC0( Mig_Obj_t * pObj ) { return Mig_IsComplement(pObj->pFanin0); }
static inline int Mig_ObjFaninC1( Mig_Obj_t * pObj ) { return Mig_IsComplement(pObj->pFanin1); }
static inline int Mig_ObjFaninC2( Mig_Obj_t * pObj ) { return Mig_IsComplement(pObj->pFanin2); }

// Obj Fanout
static inline unsigned Mig_ObjFanoutNum( Mig_Obj_t * pObj ) { return Vec_IntSize(&pObj->vFanouts); }
static inline Mig_Obj_t * Mig_ObjFanout( Mig_Man_t * p, Mig_Obj_t * pObj, int i ) { return Mig_ManObj( p, Vec_IntEntry(&pObj->vFanouts, i ) ); }

// TravId
static inline void Mig_ObjSetTravIdCurrent( Mig_Man_t * p, Mig_Obj_t * pObj ) { pObj->TravId = p->nTravIds; }
static inline int Mig_ObjIsTravIdCurrent( Mig_Man_t * p, Mig_Obj_t * pObj ) { return (int)(pObj->TravId == p->nTravIds); }

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////
/*=== migObj.c ============================================================*/
extern Mig_Obj_t* Mig_ObjAlloc( Mig_ObjType_t);
extern void Mig_ObjRemoveFanins( Mig_Obj_t * pObj );

/*=== migTran.c ===========================================================*/
extern Mig_Man_t * Mig_MigStart( Abc_Ntk_t * pNtk);
extern Mig_Man_t * Mig_AigToMig( Abc_Ntk_t * pNtk);
extern Mig_Man_t * Mig_ReadMig( char * file, Abc_Ntk_t * pNtk );
extern void Mig_ManReassignIds( Mig_Man_t * pMan);
extern int Mig_MigCleanup( Mig_Man_t * pMan );

/*=== migTable.c ===========================================================*/
extern void Mig_HashNode( Mig_Man_t * pMan, Mig_Obj_t * pObj, Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2 );
extern Mig_Obj_t * Mig_MigMaj( Mig_Man_t * pMan,  Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2 );
extern Mig_Obj_t * Mig_MigMajCreate( Mig_Man_t * pMan,  Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2 );
extern Mig_Obj_t * Mig_MigMajCreateFrom( Mig_Man_t * pMan,  Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2, Mig_Obj_t * pFrom );
extern Mig_Obj_t * Mig_MigMajCreateWithoutId( Mig_Man_t * pMan,  Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2 );
extern Mig_Obj_t * Mig_MigMajLookup( Mig_Man_t * pMan, Mig_Obj_t * p0, Mig_Obj_t * p1, Mig_Obj_t * p2 );
extern void Mig_MigRehash( Mig_Man_t * pMan );
extern void Mig_MigDeleteNode( Mig_Man_t * pMan, Mig_Obj_t * pNode ); // recursive delete fanin cone from table
extern void Mig_MigDelete( Mig_Man_t * pMan, Mig_Obj_t * pNode );     // delete node in the table
extern void Mig_MigStrash( Mig_Man_t * pMan );

/*=== migRule.c ============================================================*/
extern void Mig_InvertCanonical( Mig_Man_t * pMan );

/*=== migCut.c ============================================================*/
extern Cut_Man_t * Mig_MigStartCutManForRewrite( Mig_Man_t * pMan );
extern Cut_Cut_t * Mig_NodeGetCutsRecursive( Cut_Man_t * p, Mig_Obj_t * pObj );
extern Cut_Cut_t * Mig_NodeGetCuts( Cut_Man_t * p, Mig_Obj_t * pObj );

/*=== migGraph.c ==========================================================*/
extern void Mig_GenerateAllGraphs();
extern void Mig_GenerateAllGraphs3();
extern void Mig_WriteToArray( Mig_Man_t * p );

/*=== migWrite.c ==========================================================*/
extern void Mig_Write(Abc_Ntk_t * pNtk, Mig_Man_t * pMig, char * name );
extern void Mig_WriteWithFadd(Abc_Ntk_t * pNtk, Mig_Man_t * pMig, char * name );

/*=== migMffc.c ===========================================================*/
extern void Mig_ManIncTravId( Mig_Man_t * p );
extern int Mig_NodeMffcLabelMig( Mig_Man_t * p, Mig_Obj_t * pNode );
extern void Mig_CollectNodes( Mig_Man_t * p, Mig_Obj_t * pNode, Vec_Ptr_t * vNodes );

/*=== migReplace.c ===========================================================*/
extern void Mig_MigReplace( Mig_Man_t * pMan, Mig_Obj_t * pOld, Mig_Obj_t * pNew, int fUpdateLevel );
extern int Mig_ObjRequiredLevel( Mig_Man_t * pMan, Mig_Obj_t * pObj );
extern int Mig_ObjReverseLevel( Mig_Man_t * pMan, Mig_Obj_t * pObj );
extern void Mig_ManStartReverseLevels( Mig_Man_t * pMan, int nMaxLevelIncrease );
extern void Mig_ManRemoveFromLevelStructure( Vec_Vec_t * vStruct, Mig_Obj_t * pNode );
extern void Mig_ManRemoveFromLevelStructureR( Vec_Vec_t * vStruct, Mig_Man_t * pMan, Mig_Obj_t * pNode );

/*=== migUtil.c ==========================================================*/
extern void Mig_Print( Mig_Man_t * p);
extern void Mig_PrintHashTable( Mig_Man_t * p );
extern void Mig_PrintCuts( Cut_Man_t * p, Mig_Man_t * pMan );

/*=== migFadd.c ==========================================================*/
extern Mig_Man_t * Mig_AigToMigWithAdderDetection( Abc_Ntk_t * pNtk );

/*=== migSim.c ==========================================================*/
extern void simulate( Mig_Man_t * pMan );

////////////////////////////////////////////////////////////////////////
///                          ITERATORs                               ///
////////////////////////////////////////////////////////////////////////
#define Mig_ManForEachObj( pMig, pObj, i )                                                        \
    for( i = 0; (i < Mig_ManObjNum(pMig)) && (((pObj)=Mig_ManObj(pMig,i)),1); i++)                \
        if( (pObj) == NULL ) { } else

#define Mig_ManForEachPi( pMig, pPi, i )                                                          \
    for( i = 0; (i < Mig_ManPiNum(pMig)) && (((pPi)=Mig_ManPi(pMig,i)),1); i++)

#define Mig_ManForEachPo( pMig, pPo, i )                                                          \
    for( i = 0; (i < Mig_ManPoNum(pMig)) && (((pPo)=Mig_ManPo(pMig,i)),1); i++)

#define Mig_ManForEachNode( pMig, pObj, i )                                                       \
    for( i = 0; (i < Mig_ManObjNum(pMig)) && (((pObj)=Mig_ManObj(pMig,i)),1);i++)                 \
        if( pObj == NULL || !Mig_ObjIsNode(pObj) ) {} else

#define Mig_ObjForEachFanout( pMig, pObj, pFanout, i )                                            \
    for( i = 0; (i < Mig_ObjFanoutNum(pObj))&&(((pFanout)=Mig_ObjFanout(pMig,pObj,i)),1); i++ )

#define Mig_MigBinForEachEntry( pBin, pEnt )                                                      \
    for( pEnt = pBin; pEnt; pEnt = pEnt->pNext )                                                  \

#define Mig_MigBinForEachEntrySafe( pBin, pEnt, pEnt2 )                                           \
    for( pEnt = pBin, pEnt2 = pEnt ? pEnt->pNext:NULL;                                            \
         pEnt;                                                                                    \
         pEnt = pEnt2, pEnt2 = pEnt? pEnt->pNext:NULL )                                           \

ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

