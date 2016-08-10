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
 
#ifndef ABC__MIG_RWR_h
#define ABC__MIG_RWR_h


////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

#include "base/abc/abc.h"
#include "opt/mig/mig.h"
#include "opt/cut/cut.h"
#include "opt/cut/cutInt.h"
#include "opt/cut/cutList.h"

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_START

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////
typedef struct Rwr_Obj_t_     Rwr_Obj_t;
typedef struct Rewrite_Man_t_ Rewrite_Man_t;

struct Rwr_Obj_t_
{
    int             Id;
    unsigned        TravId;
    unsigned        uTruth : 16;
    unsigned        Volume :  9;  
    unsigned        Level  :  5;
    unsigned        fUsed  :  1;
    unsigned        fOut   :  1;  // marked as output node
    Rwr_Obj_t *     p0;
    Rwr_Obj_t *     p1;
    Rwr_Obj_t *     p2;
    Rwr_Obj_t *     pNext;
};

struct Rewrite_Man_t_
{
    int               nFuncs;
    unsigned short *  puCanons;      // canonical forms
    char *            pPhases;       // canonical phases
    char *            pPerms;        // canonical permutations
    unsigned char *   pMap;          // [0-65535] mapping from functions to class
    unsigned short *  pMapInv;       // mapping from class to truth
    char **           pPerms4;       // four-var permutations
    // node space
    Vec_Ptr_t *       vForest;       // all the nodes
    Rwr_Obj_t **      pTable;        // [0-65535] hash table 
    Vec_Vec_t *       vClasses;      // [0-221] list
    //
    int               nTravIds;      // traversal purpose 
    int               nClasses;
    // temporary
    int               fCompl;        // indicated if the output should be compl
    void *            pGraph;        // the decomposition graph
    Vec_Ptr_t *       vFanins;       // the fanin array
    Vec_Ptr_t *       vFaninsCur;    // the fanin array
    Vec_Ptr_t *       vNodesTemp;    // the nodes in MFFC
    // statistics
    int               nScores[222];
};

static inline Rwr_Obj_t * Rwr_Regular( Rwr_Obj_t * p ) { return (Rwr_Obj_t*)((ABC_PTRUINT_T)(p) & ~01); }
static inline Rwr_Obj_t * Rwr_Not( Rwr_Obj_t * p ) { return (Rwr_Obj_t*)((ABC_PTRUINT_T)(p) ^ 01); }
static inline Rwr_Obj_t * Rwr_NotCond( Rwr_Obj_t * p, int c ) { return (Rwr_Obj_t*)((ABC_PTRUINT_T)(p) ^ (c)); }
static inline int Rwr_IsComplement( Rwr_Obj_t * p ) { return (int)((ABC_PTRUINT_T)(p) & 01); }

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

/*=== rewriteMan.c =================================================*/
extern Rewrite_Man_t * Rewrite_ManStartFor4Var(int isHighEffort);
extern Rewrite_Man_t * Rewrite_ManStartFor3Var();
extern Rwr_Obj_t * Rewrite_ManAddVar( Rewrite_Man_t * p, unsigned uTruth );
extern Rwr_Obj_t * Rewrite_ManAddNode( Rewrite_Man_t * p, Rwr_Obj_t * p0, Rwr_Obj_t * p1, Rwr_Obj_t * p2, int fOut, int Level, int Volume, unsigned FFFF );
extern void Rewrite_ManIncTravId( Rewrite_Man_t * p );

/*=== rewriteLoad.c ================================================*/
extern void Rewrite_ManLoadFrom4VarArray( Rewrite_Man_t * p, int isHighEffort );
extern void Rewrite_ManLoadFrom3VarArray( Rewrite_Man_t * p );

/*=== rewriteEva.c ================================================*/
extern int Rewrite_NodeRewrite( Mig_Man_t * pMan, Rewrite_Man_t * p, Cut_Man_t * pCutMan, Mig_Obj_t * pNode, Cut_Cut_t * pCut, int fUpdateLevel );
extern int Rewrite_NodeRewriteFor3Var( Mig_Man_t * pMan, Rewrite_Man_t * p, Cut_Man_t * pCutMan, Mig_Obj_t * pNode, Cut_Cut_t * pCut );
extern int Rewrite_NodeRewriteForSimCandidate( Mig_Man_t * pMig, Rewrite_Man_t * p, Cut_Man_t * pCutMan, Mig_Obj_t * pNode, Mig_Obj_t * pCandidate, Cut_Cut_t * pCut );


////////////////////////////////////////////////////////////////////////
///                          ITERATORs                               ///
////////////////////////////////////////////////////////////////////////

ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

