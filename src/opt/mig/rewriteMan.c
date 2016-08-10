/**CFile****************************************************************

  FileName    [rwrMan.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [DAG-aware AIG rewriting package.]

  Synopsis    [Rewriting manager.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: rwrMan.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "rewrite.h"
#include "base/main/main.h"
#include "bool/dec/dec.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////
static void Truth3VarNPN( unsigned short ** puCanons, char ** puPhases, char ** puPerms, unsigned char ** puMap );
////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Starts rewriting manager.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Rewrite_ListAddToTail( Rwr_Obj_t ** ppList, Rwr_Obj_t * pObj )
{
    Rwr_Obj_t * pTemp;
    // find the last one
    for ( pTemp = *ppList; pTemp; pTemp = pTemp->pNext )
        ppList = &pTemp->pNext;
    *ppList = pObj;
}

Rwr_Obj_t * Rewrite_ManAddVar( Rewrite_Man_t * p, unsigned uTruth )
{
    Rwr_Obj_t * pNew;
    pNew = ABC_ALLOC( Rwr_Obj_t, 1 );
    memset( pNew, 0, sizeof(Rwr_Obj_t) );
    pNew->Id     = p->vForest->nSize;
    pNew->TravId = 0;
    pNew->uTruth = uTruth;
    pNew->Level  = 0;
    pNew->Volume = 0;
    pNew->fUsed  = 1;
    pNew->fOut   = 0;
    pNew->p0     = NULL;
    pNew->p1     = NULL;
    pNew->p2     = NULL;
    pNew->pNext  = NULL;
    Vec_PtrPush( p->vForest, pNew );
    return pNew;
}

Rwr_Obj_t * Rewrite_ManAddNode( Rewrite_Man_t * p, Rwr_Obj_t * p0, Rwr_Obj_t * p1, Rwr_Obj_t * p2, int fOut, int Level, int Volume, unsigned FFFF )
{
    Rwr_Obj_t * pNew;
    unsigned uTruth, t0, t1, t2;
    
    t0 = Rwr_IsComplement(p0) ? ~Rwr_Regular(p0)->uTruth : Rwr_Regular(p0)->uTruth;
    t1 = Rwr_IsComplement(p1) ? ~Rwr_Regular(p1)->uTruth : Rwr_Regular(p1)->uTruth;
    t2 = Rwr_IsComplement(p2) ? ~Rwr_Regular(p2)->uTruth : Rwr_Regular(p2)->uTruth;

    uTruth = ( (t0&t1) | (t0&t2) | (t1&t2) ) & FFFF;

    pNew = ABC_ALLOC( Rwr_Obj_t, 1 );
    memset( pNew, 0, sizeof(Rwr_Obj_t) );
    pNew->Id     = p->vForest->nSize;
    pNew->TravId = 0;
    pNew->uTruth = uTruth;
    pNew->Level  = Level;
    pNew->Volume = Volume;
    pNew->fUsed  = 0;
    pNew->fOut   = fOut;
    pNew->p0     = p0;
    pNew->p1     = p1;
    pNew->p2     = p2;
    pNew->pNext  = NULL;
    Vec_PtrPush( p->vForest, pNew );

    if( fOut == 0 ) return pNew;
    //printf("%d %d\n",  uTruth, p->puCanons[uTruth] );
    assert( uTruth == p->puCanons[uTruth] );

    if( p->pTable[uTruth] == NULL ) p->nClasses++;
    Rewrite_ListAddToTail( p->pTable + uTruth, pNew );
    return pNew;
}

Rewrite_Man_t * Rewrite_ManStartFor4Var(int isHighEffort )
{
    Dec_Man_t * pManDec;
    Rewrite_Man_t * p;
    int i;

    p = ABC_ALLOC( Rewrite_Man_t, 1 );
    memset( p, 0, sizeof(Rewrite_Man_t) );

    p->nFuncs = (1<<16);
    pManDec   = (Dec_Man_t *)Abc_FrameReadManDec();
    p->puCanons = pManDec->puCanons; 
    p->pPhases  = pManDec->pPhases; 
    p->pPerms   = pManDec->pPerms; 
    p->pMap     = pManDec->pMap; 


    unsigned tmp = 0x0000;
    printf("0x163F canon truth:%X\n",p->puCanons[tmp]);
    printf("0x163F phases: %d\n", p->pPhases[tmp]);
    printf("0x163F perms: %d\n", p->pPerms[tmp]);
    printf("0x163F class: %d\n", p->pMap[tmp]);
    tmp++;

    printf("\n0x163F canon truth:%X\n",p->puCanons[tmp]);
    printf("0x163F phases: %d\n", p->pPhases[tmp]);
    printf("0x163F perms: %d\n", p->pPerms[tmp]);
    printf("0x163F class: %d\n", p->pMap[tmp]);
    tmp++;

    printf("\n0x163F canon truth:%X\n",p->puCanons[tmp]);
    printf("0x163F phases: %d\n", p->pPhases[tmp]);
    printf("0x163F perms: %d\n", p->pPerms[tmp]);
    printf("0x163F class: %d\n", p->pMap[tmp]);
    // create the table
    p->pTable = ABC_ALLOC( Rwr_Obj_t *, p->nFuncs );
    memset( p->pTable, 0, sizeof(Rwr_Obj_t *) * p->nFuncs );

    // create the elementary nodes, note that var node won't be added to pTable
    p->vForest  = Vec_PtrAlloc( 100 );
    Rewrite_ManAddVar( p, 0xFFFF ); // constant 1
    Rewrite_ManAddVar( p, 0xAAAA ); // var A
    Rewrite_ManAddVar( p, 0xCCCC ); // var B
    Rewrite_ManAddVar( p, 0xF0F0 ); // var C
    Rewrite_ManAddVar( p, 0xFF00 ); // var D
    p->nClasses = 2;

    for ( i = 0; i < 222; i++ ) p->nScores[i] = 0;

    // other stuff
    p->nTravIds   = 1;
    p->pPerms4    = Extra_Permutations( 4 );
    p->vFanins    = Vec_PtrAlloc( 50 );
    p->vFaninsCur = Vec_PtrAlloc( 50 );
    p->vNodesTemp = Vec_PtrAlloc( 50 );

    // load saved subgraphs
    Rewrite_ManLoadFrom4VarArray( p, isHighEffort );
    //Rwr_ManPrint( p );
    //Rwr_ManPreprocess( p );
    
//p->timeStart = Abc_Clock() - clk;
    return p;
}

void Rewrite_ManIncTravId( Rewrite_Man_t * p )
{
    Rwr_Obj_t * pNode;
    int i;
    if ( p->nTravIds++ < 0x8FFFFFFF ) return;

    Vec_PtrForEachEntry( Rwr_Obj_t *, p->vForest, pNode, i )
        pNode->TravId = 0;
    p->nTravIds = 1;
}

Rewrite_Man_t * Rewrite_ManStartFor3Var( )
{
    Dec_Man_t * pManDec;
    Rewrite_Man_t * p;

    p = ABC_ALLOC( Rewrite_Man_t, 1 );
    memset( p, 0, sizeof(Rewrite_Man_t) );

    p->nFuncs = (1<<8);
    pManDec   = (Dec_Man_t *)Abc_FrameReadManDec();
    Truth3VarNPN( &p->puCanons, &p->pPhases, &p->pPerms, &p->pMap );

    // create the table
    p->pTable = ABC_ALLOC( Rwr_Obj_t *, p->nFuncs );
    memset( p->pTable, 0, sizeof(Rwr_Obj_t *) * p->nFuncs );

    // create the elementary nodes, note that var node won't be added to pTable
    p->vForest  = Vec_PtrAlloc( 100 );
    Rewrite_ManAddVar( p, 0x00FF ); // constant 1
    Rewrite_ManAddVar( p, 0x00AA ); // var A
    Rewrite_ManAddVar( p, 0x00CC ); // var B
    Rewrite_ManAddVar( p, 0x00F0 ); // var C
    p->nClasses = 2;

    // other stuff
    p->nTravIds   = 1;
    p->pPerms4    = Extra_Permutations( 3 );
    p->vFanins    = Vec_PtrAlloc( 50 );
    p->vFaninsCur = Vec_PtrAlloc( 50 );
    p->vNodesTemp = Vec_PtrAlloc( 50 );

    // load saved subgraphs
    Rewrite_ManLoadFrom3VarArray( p );
    //Rwr_ManPrint( p );
    //Rwr_ManPreprocess( p );
    
//p->timeStart = Abc_Clock() - clk;
    return p;
}

void Truth3VarNPN( unsigned short ** puCanons, char ** puPhases, char ** puPerms, unsigned char ** puMap )
{
    unsigned short * uCanons;
    unsigned char * uMap;
    unsigned uTruth, uPhase, uPerm;
    char ** pPerms3, * uPhases, * uPerms;
    int nFuncs, nClasses;
    int i, k;

    nFuncs  = (1 << 8);
    uCanons = ABC_ALLOC( unsigned short, nFuncs );
    uPhases = ABC_ALLOC( char, nFuncs );
    uPerms  = ABC_ALLOC( char, nFuncs );
    uMap    = ABC_ALLOC( unsigned char, nFuncs );
    memset( uCanons, 0, sizeof(unsigned short) * nFuncs );
    memset( uPhases, 0, sizeof(char) * nFuncs );
    memset( uPerms,  0, sizeof(char) * nFuncs );
    memset( uMap,    0, sizeof(unsigned char) * nFuncs );
    pPerms3 = Extra_Permutations( 3 );

    nClasses = 1;
    nFuncs = (1 << 7);
    for ( uTruth = 1; uTruth < (unsigned)nFuncs; uTruth++ )
    {
        // skip already assigned
        if ( uCanons[uTruth] )
        {
            assert( uTruth > uCanons[uTruth] );
            uMap[~uTruth & 0x00FF] = uMap[uTruth] = uMap[uCanons[uTruth]];
            continue;
        }
        uMap[uTruth] = nClasses++;
        for ( i = 0; i < 8; i++ )
        {
            uPhase = Extra_TruthPolarize( uTruth, i, 3 );
            for ( k = 0; k < 6; k++ )
            {
                uPerm = Extra_TruthPermute( uPhase, pPerms3[k], 4, 0 );
                if ( uCanons[uPerm] == 0 )
                {
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i;
                    uPerms[uPerm]  = k;
                    uMap[uPerm] = uMap[uTruth];

                    uPerm = ~uPerm & 0x00FF;
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i | 8;
                    uPerms[uPerm]  = k;
                    uMap[uPerm] = uMap[uTruth];
                }
                else
                    assert( uCanons[uPerm] == uTruth );
            }
            uPhase = Extra_TruthPolarize( ~uTruth & 0x00FF, i, 3 ); 
            for ( k = 0; k < 6; k++ )
            {
                uPerm = Extra_TruthPermute( uPhase, pPerms3[k], 3, 0 );
                if ( uCanons[uPerm] == 0 )
                {
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i;
                    uPerms[uPerm]  = k;
                    uMap[uPerm] = uMap[uTruth];

                    uPerm = ~uPerm & 0x00FF;
                    uCanons[uPerm] = uTruth;
                    uPhases[uPerm] = i | 8;
                    uPerms[uPerm]  = k;
                    uMap[uPerm] = uMap[uTruth];
                }
                else
                    assert( uCanons[uPerm] == uTruth );
            }
        }
    }
    for( uTruth = 1; uTruth < 0x00FF; uTruth++) assert( uMap[uTruth] != 0 );
    uPhases[(1<<8)-1] = 8;
    assert( nClasses == 14 );
    ABC_FREE( pPerms3 );
    if ( puCanons ) 
        *puCanons = uCanons;
    else
        ABC_FREE( uCanons );
    if ( puPhases ) 
        *puPhases = uPhases;
    else
        ABC_FREE( uPhases );
    if ( puPerms ) 
        *puPerms = uPerms;
    else
        ABC_FREE( uPerms );
    if ( puMap ) 
        *puMap = uMap;
    else
        ABC_FREE( uMap );
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

