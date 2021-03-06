/**CFile****************************************************************

  FileName    [giaQbf.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Scalable AIG package.]

  Synopsis    [QBF solver.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - October 18, 2014.]

  Revision    [$Id: giaQbf.c,v 1.00 2014/10/18 00:00:00 alanmi Exp $]

***********************************************************************/

#include "gia.h"
#include "sat/cnf/cnf.h"
#include "sat/bsat/satStore.h"
#include "misc/extra/extra.h"

ABC_NAMESPACE_IMPL_START


////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

typedef struct Qbf_Man_t_ Qbf_Man_t; 
struct Qbf_Man_t_
{
    Gia_Man_t *     pGia;           // original miter
    int             nPars;          // parameter variables
    int             nVars;          // functional variables
    int             fVerbose;       // verbose flag
    // internal variables
    int             iParVarBeg;     // SAT var ID of the first par variable in the ver solver
    sat_solver *    pSatVer;        // verification instance
    sat_solver *    pSatSyn;        // synthesis instance
    Vec_Int_t *     vValues;        // variable values
    Vec_Int_t *     vParMap;        // parameter mapping
    Vec_Int_t *     vLits;          // literals for the SAT solver
    abctime         clkStart;       // global timeout
    abctime         clkSat;         // SAT solver time
};

extern Cnf_Dat_t * Mf_ManGenerateCnf( Gia_Man_t * pGia, int nLutSize, int fCnfObjIds, int fAddOrCla, int fVerbose );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Generating QBF miter to solve the induction problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Vec_Int_t * Gia_GenCollectFlopIndexes( char * pStr, int nLutNum, int nLutSize, int nFlops )
{
    int nDups;
    char * pTemp;
    Vec_Int_t * vFlops;
    assert( nLutSize * nLutNum <= nFlops );
    if ( pStr == NULL )
        return Vec_IntStartNatural( nLutNum * nLutSize );
    vFlops = Vec_IntAlloc( nLutNum * nLutSize );
    pTemp = strtok( pStr, ", " );
    while ( pTemp )
    {
        int iFlop = atoi(pTemp);
        if ( iFlop >= nFlops )
        {
            printf( "Flop index (%d) exceeds the number of flops (%d).\n", iFlop, nFlops );
            break;
        }
        Vec_IntPush( vFlops, iFlop );
        pTemp = strtok( NULL, ", " );
    }
    if ( Vec_IntSize(vFlops) != nLutNum * nLutSize )
    {
        printf( "Gia_GenCollectFlopIndexes: Expecting %d flop indexes (instead of %d).\n", nLutNum * nLutSize, Vec_IntSize(vFlops) );
        Vec_IntFree( vFlops );
        return NULL;
    }
    nDups = Vec_IntCountDuplicates(vFlops);
    if ( nDups )
    {
        printf( "Gia_GenCollectFlopIndexes: There are %d duplicated flops in the list.\n", nDups );
        Vec_IntFree( vFlops );
        return NULL;
    }
    return vFlops;
}
int Gia_GenCreateMux_rec( Gia_Man_t * pNew, int * pCtrl, int nCtrl, Vec_Int_t * vData, int Shift )
{
    int iLit0, iLit1;
    if ( nCtrl == 0 )
        return Vec_IntEntry( vData, Shift );
    iLit0 = Gia_GenCreateMux_rec( pNew, pCtrl, nCtrl-1, vData, Shift );
    iLit1 = Gia_GenCreateMux_rec( pNew, pCtrl, nCtrl-1, vData, Shift + (1<<(nCtrl-1)) );
    return Gia_ManHashMux( pNew, pCtrl[nCtrl-1], iLit1, iLit0 );
}
Vec_Int_t * Gia_GenCreateMuxes( Gia_Man_t * p, Gia_Man_t * pNew, Vec_Int_t * vFlops, int nLutNum, int nLutSize, Vec_Int_t * vParLits, int fUseRi )
{
    Vec_Int_t * vLits = Vec_IntAlloc( nLutNum );
    int i, k, iMux, iFlop, pCtrl[16];
    // add MUXes for each group of flops
    assert( Vec_IntSize(vFlops) == nLutNum * nLutSize );
    for ( i = 0; i < nLutNum; i++ )
    {
        for ( k = 0; k < nLutSize; k++ )
        {
            iFlop = Vec_IntEntry(vFlops, i * nLutSize + k);
            assert( iFlop >= 0 && iFlop < Gia_ManRegNum(p) );
            if ( fUseRi )
                pCtrl[k] = Gia_ManRi(p, iFlop)->Value;
            else
                pCtrl[k] = Gia_ManRo(p, iFlop)->Value;
        }
        iMux = Gia_GenCreateMux_rec( pNew, pCtrl, nLutSize, vParLits, i * (1 << nLutSize) );
        Vec_IntPush( vLits, iMux );
    }
    return vLits;
}
Gia_Man_t * Gia_GenQbfMiter( Gia_Man_t * p, int nFrames, int nLutNum, int nLutSize, char * pStr, int fVerbose )
{
    Gia_Obj_t * pObj; 
    Gia_Man_t * pTemp, * pNew;
    int i, iMiter, nPars = nLutNum * (1 << nLutSize);
    Vec_Int_t * vLits0, * vLits1, * vParLits;
    Vec_Int_t * vFlops = Gia_GenCollectFlopIndexes( pStr, nLutNum, nLutSize, Gia_ManRegNum(p) );
    // collect parameter literals (data vars)
    vParLits = Vec_IntAlloc( nPars );
    for ( i = 0; i < nPars; i++ )
        Vec_IntPush( vParLits, i ? Abc_Var2Lit(i+1, 0) : 1 );
    // create new manager
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    pNew->pSpec = Abc_UtilStrsav( p->pSpec );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    for ( i = 0; i < nPars; i++ )
        Gia_ManAppendCi( pNew );
    Gia_ManForEachCi( p, pObj, i )
        pObj->Value = Gia_ManAppendCi( pNew );
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ObjFanin0Copy(pObj);
    vLits0 = Gia_GenCreateMuxes( p, pNew, vFlops, nLutNum, nLutSize, vParLits, 0 );
    vLits1 = Gia_GenCreateMuxes( p, pNew, vFlops, nLutNum, nLutSize, vParLits, 1 );
    // create miter output
    iMiter = Gia_ManHashAnd( pNew, Vec_IntEntry(vLits0, 0), Abc_LitNot(Vec_IntEntry(vLits1, 0)) );
    iMiter = Gia_ManHashAnd( pNew, Abc_LitNot(iMiter), Abc_Var2Lit(1, 0) );
    Gia_ManAppendCo( pNew, iMiter );
    // cleanup
    Vec_IntFree( vLits0 );
    Vec_IntFree( vLits1 );
    Vec_IntFree( vFlops );
    Vec_IntFree( vParLits );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Naive way to enumerate SAT assignments.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_ManSatEnum( Gia_Man_t * pGia, int nConfLimit, int nTimeOut, int fVerbose )
{
    Cnf_Dat_t * pCnf;
    sat_solver * pSat;
    Vec_Int_t * vLits;
    int i, iLit, iParVarBeg, Iter;
    int nSolutions = 0, RetValue = 0;
    abctime clkStart = Abc_Clock();
    pCnf = Mf_ManGenerateCnf( pGia, 8, 0, 1, 0 );
    pSat = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    iParVarBeg = pCnf->nVars - Gia_ManPiNum(pGia);// - 1;
    Cnf_DataFree( pCnf );
    // iterate through the SAT assignment
    vLits = Vec_IntAlloc( Gia_ManPiNum(pGia) );
    for ( Iter = 1 ; ; Iter++ )
    {
        int status = sat_solver_solve( pSat, NULL, NULL, (ABC_INT64_T)nConfLimit, 0, 0, 0 );
        if ( status == l_False ) { RetValue =  1; break; }
        if ( status == l_Undef ) { RetValue =  0; break; }
        nSolutions++;
        // extract SAT assignment
        Vec_IntClear( vLits );
        for ( i = 0; i < Gia_ManPiNum(pGia); i++ )
            Vec_IntPush( vLits, Abc_Var2Lit(iParVarBeg+i, sat_solver_var_value(pSat, iParVarBeg+i)) );
        if ( fVerbose )
        {
            printf( "%5d : ", Iter );
            Vec_IntForEachEntry( vLits, iLit, i )
                printf( "%d", !Abc_LitIsCompl(iLit) );
            printf( "\n" );
        }
        // add clause
        if ( !sat_solver_addclause( pSat, Vec_IntArray(vLits), Vec_IntArray(vLits) + Vec_IntSize(vLits) ) )
            { RetValue =  1; break; }
        if ( nTimeOut && (Abc_Clock() - clkStart)/CLOCKS_PER_SEC >= nTimeOut ) { RetValue = 0; break; }
    }
    sat_solver_delete( pSat );
    Vec_IntFree( vLits );
    if ( nTimeOut && (Abc_Clock() - clkStart)/CLOCKS_PER_SEC >= nTimeOut )
        printf( "Enumerated %d assignments when timeout (%d sec) was reached.  ", nSolutions, nTimeOut );
    else if ( nConfLimit && !RetValue )
        printf( "Enumerated %d assignments when conflict limit (%d) was reached.  ", nSolutions, nConfLimit );
    else 
        printf( "Enumerated the complete set of %d assignments.  ", nSolutions );
    Abc_PrintTime( 1, "Time", Abc_Clock() - clkStart );
    return RetValue;
}

/**Function*************************************************************

  Synopsis    [Dumps the original problem in QDIMACS format.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_QbfDumpFile( Gia_Man_t * pGia, int nPars )
{
    // original problem: \exists p \forall x \exists y.  M(p,x,y)
    // negated problem:  \forall p \exists x \exists y. !M(p,x,y)
    Cnf_Dat_t * pCnf = Mf_ManGenerateCnf( pGia, 8, 0, 1, 0 );
    Vec_Int_t * vVarMap, * vForAlls, * vExists;
    Gia_Obj_t * pObj;
    char * pFileName;
    int i, Entry;
    // create var map
    vVarMap = Vec_IntStart( pCnf->nVars );
    Gia_ManForEachCi( pGia, pObj, i )
        if ( i < nPars )
            Vec_IntWriteEntry( vVarMap, pCnf->pVarNums[Gia_ManCiIdToId(pGia, i)], 1 );
    // create various maps
    vForAlls = Vec_IntAlloc( nPars );
    vExists = Vec_IntAlloc( Gia_ManCiNum(pGia) - nPars );
    Vec_IntForEachEntry( vVarMap, Entry, i )
        if ( Entry )
            Vec_IntPush( vForAlls, i );
        else
            Vec_IntPush( vExists, i );
    // generate CNF
    pFileName = Extra_FileNameGenericAppend( pGia->pSpec, ".qdimacs" );
    Cnf_DataWriteIntoFile( pCnf, pFileName, 0, vForAlls, vExists );
    Cnf_DataFree( pCnf );
    Vec_IntFree( vForAlls );
    Vec_IntFree( vExists );
    Vec_IntFree( vVarMap );
    printf( "The 2QBF formula was written into file \"%s\".\n", pFileName );
}

/**Function*************************************************************

  Synopsis    [Generate one SAT assignment of the problem.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Qbf_Man_t * Gia_QbfAlloc( Gia_Man_t * pGia, int nPars, int fVerbose )
{
    Qbf_Man_t * p;
    Cnf_Dat_t * pCnf;
    Gia_ObjFlipFaninC0( Gia_ManPo(pGia, 0) );
    pCnf = Mf_ManGenerateCnf( pGia, 8, 0, 1, 0 );
    Gia_ObjFlipFaninC0( Gia_ManPo(pGia, 0) );
    p = ABC_CALLOC( Qbf_Man_t, 1 );
    p->clkStart   = Abc_Clock();
    p->pGia       = pGia;
    p->nPars      = nPars;
    p->nVars      = Gia_ManPiNum(pGia) - nPars;
    p->fVerbose   = fVerbose;
    p->iParVarBeg = pCnf->nVars - Gia_ManPiNum(pGia);// - 1;
    p->pSatVer    = (sat_solver *)Cnf_DataWriteIntoSolver( pCnf, 1, 0 );
    p->pSatSyn    = sat_solver_new();
    p->vValues    = Vec_IntAlloc( Gia_ManPiNum(pGia) );
    p->vParMap    = Vec_IntStartFull( nPars );
    p->vLits      = Vec_IntAlloc( nPars );
    sat_solver_setnvars( p->pSatSyn, nPars );
    Cnf_DataFree( pCnf );
    return p;
}
void Gia_QbfFree( Qbf_Man_t * p )
{
    sat_solver_delete( p->pSatVer );
    sat_solver_delete( p->pSatSyn );
    Vec_IntFree( p->vLits );
    Vec_IntFree( p->vValues );
    Vec_IntFree( p->vParMap );
    ABC_FREE( p );
}

/**Function*************************************************************

  Synopsis    [Create and add one cofactor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_QbfQuantify( Gia_Man_t * p, int nPars )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj; 
    int i, m, nMints = (1 << (Gia_ManPiNum(p) - nPars));
    assert( Gia_ManPoNum(p) == 1 );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    for ( i = 0; i < nPars; i++ )
        Gia_ManAppendCi(pNew);
    for ( m = 0; m < nMints; m++ )
    {
        Gia_ManForEachPi( p, pObj, i )
            pObj->Value = (i < nPars) ? Gia_Obj2Lit(pNew, Gia_ManPi(pNew, i)) : ((m >> (i - nPars)) & 1);
        Gia_ManForEachAnd( p, pObj, i )
            pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
        Gia_ManForEachCo( p, pObj, i )
            pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    }
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    assert( Gia_ManPiNum(pNew) == nPars );
    assert( Gia_ManPoNum(pNew) == nMints );
    return pNew;
}

/**Function*************************************************************

  Synopsis    [Create and add one cofactor.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
Gia_Man_t * Gia_QbfCofactor( Gia_Man_t * p, int nPars, Vec_Int_t * vValues, Vec_Int_t * vParMap )
{
    Gia_Man_t * pNew, * pTemp;
    Gia_Obj_t * pObj; int i;
    assert( Gia_ManPiNum(p) == nPars + Vec_IntSize(vValues) );
    pNew = Gia_ManStart( Gia_ManObjNum(p) );
    pNew->pName = Abc_UtilStrsav( p->pName );
    Gia_ManHashAlloc( pNew );
    Gia_ManConst0(p)->Value = 0;
    Gia_ManForEachPi( p, pObj, i )
        if ( i < nPars )
        {
            pObj->Value = Gia_ManAppendCi(pNew);
            if ( Vec_IntEntry(vParMap, i) != -1 )
                pObj->Value = Vec_IntEntry(vParMap, i);
        }
        else
            pObj->Value = Vec_IntEntry(vValues, i - nPars);
    Gia_ManForEachAnd( p, pObj, i )
        pObj->Value = Gia_ManHashAnd( pNew, Gia_ObjFanin0Copy(pObj), Gia_ObjFanin1Copy(pObj) );
    Gia_ManForEachCo( p, pObj, i )
        pObj->Value = Gia_ManAppendCo( pNew, Gia_ObjFanin0Copy(pObj) );
    pNew = Gia_ManCleanup( pTemp = pNew );
    Gia_ManStop( pTemp );
    assert( Gia_ManPiNum(pNew) == nPars );
    return pNew;
}
int Gia_QbfAddCofactor( Qbf_Man_t * p, Gia_Man_t * pCof )
{
    Cnf_Dat_t * pCnf = Mf_ManGenerateCnf( pCof, 8, 0, 1, 0 );
    int i, iFirstVar = sat_solver_nvars(p->pSatSyn) + pCnf->nVars - Gia_ManPiNum(pCof);// - 1;
    pCnf->pMan = NULL;
    Cnf_DataLift( pCnf, sat_solver_nvars(p->pSatSyn) );
    for ( i = 0; i < pCnf->nClauses; i++ )
        if ( !sat_solver_addclause( p->pSatSyn, pCnf->pClauses[i], pCnf->pClauses[i+1] ) )
        {
            Cnf_DataFree( pCnf );
            return 0;
        }
    Cnf_DataFree( pCnf );
    // add connection clauses
    for ( i = 0; i < Gia_ManPiNum(p->pGia); i++ )
        if ( !sat_solver_add_buffer( p->pSatSyn, i, iFirstVar+i, 0 ) )
            return 0;
    return 1;
}

/**Function*************************************************************

  Synopsis    [Extract SAT assignment.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_QbfOnePattern( Qbf_Man_t * p, Vec_Int_t * vValues )
{
    int i;
    Vec_IntClear( vValues );
    for ( i = 0; i < p->nPars; i++ )
        Vec_IntPush( vValues, sat_solver_var_value(p->pSatSyn, i) );
}
void Gia_QbfPrint( Qbf_Man_t * p, Vec_Int_t * vValues, int Iter )
{
    printf( "%5d : ", Iter );
    assert( Vec_IntSize(vValues) == p->nVars );
    Vec_IntPrintBinary( vValues ); printf( "  " );
    printf( "Var = %6d  ", sat_solver_nvars(p->pSatSyn) );
    printf( "Cla = %6d  ", sat_solver_nclauses(p->pSatSyn) );
    printf( "Conf = %6d  ", sat_solver_nconflicts(p->pSatSyn) );
    Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
}

/**Function*************************************************************

  Synopsis    [Generate one SAT assignment in terms of functional vars.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_QbfVerify( Qbf_Man_t * p, Vec_Int_t * vValues )
{
    int i, Entry, RetValue;
    assert( Vec_IntSize(vValues) == p->nPars );
    Vec_IntClear( p->vLits );
    Vec_IntForEachEntry( vValues, Entry, i )
        Vec_IntPush( p->vLits, Abc_Var2Lit(p->iParVarBeg+i, !Entry) );
    RetValue = sat_solver_solve( p->pSatVer, Vec_IntArray(p->vLits), Vec_IntLimit(p->vLits), 0, 0, 0, 0 );
    if ( RetValue == l_True )
    {
        Vec_IntClear( vValues );
        for ( i = 0; i < p->nVars; i++ )
            Vec_IntPush( vValues, sat_solver_var_value(p->pSatVer, p->iParVarBeg+p->nPars+i) );
    }
    return RetValue == l_True ? 1 : 0;
}

/**Function*************************************************************

  Synopsis    [Constraint learning.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Gia_QbfAddSpecialConstr( Qbf_Man_t * p )
{
    int i, status, Lits[2];
    for ( i = 0; i < 4*11; i++ )
        if ( i % 4 == 0 )
        {
            assert( Vec_IntEntry(p->vParMap, i) == -1 );
            Vec_IntWriteEntry( p->vParMap, i, (i % 4) == 3 );
            Lits[0] = Abc_Var2Lit( i, (i % 4) != 3 );
            status = sat_solver_addclause( p->pSatSyn, Lits, Lits+1 );
            assert( status );
        }
}
void Gia_QbfLearnConstraint( Qbf_Man_t * p, Vec_Int_t * vValues )
{
    int i, status, Entry, Lits[2];
    assert( Vec_IntSize(vValues) == p->nPars );
    printf( "  Pattern   " );
    Vec_IntPrintBinary( vValues );
    printf( "\n" );
    Vec_IntForEachEntry( vValues, Entry, i )
    {
        Lits[0] = Abc_Var2Lit( i, Entry );
        status = sat_solver_solve( p->pSatSyn, Lits, Lits+1, 0, 0, 0, 0 );
        printf( "  Var =%4d ", i );
        if ( status != l_True )
        {
            printf( "UNSAT\n" );
            Lits[0] = Abc_LitNot(Lits[0]);
            status = sat_solver_addclause( p->pSatSyn, Lits, Lits+1 );
            assert( status );
            continue;
        }
        Gia_QbfOnePattern( p, p->vLits );
        Vec_IntPrintBinary( p->vLits );
        printf( "\n" );
    }
    assert( Vec_IntSize(vValues) == p->nPars );
}

/**Function*************************************************************

  Synopsis    [Performs QBF solving using an improved algorithm.]

  Description []
               
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Gia_QbfSolve( Gia_Man_t * pGia, int nPars, int nIterLimit, int nConfLimit, int nTimeOut, int fVerbose )
{
    Qbf_Man_t * p = Gia_QbfAlloc( pGia, nPars, fVerbose );
    Gia_Man_t * pCof;
    int i, status, RetValue = 0;
    abctime clk;
//    Gia_QbfAddSpecialConstr( p );
    if ( fVerbose )
        printf( "Solving QBF for \"%s\" with %d parameters, %d variables and %d AIG nodes.\n", 
            Gia_ManName(pGia), p->nPars, p->nVars, Gia_ManAndNum(pGia) );
    assert( Gia_ManRegNum(pGia) == 0 );
    Vec_IntFill( p->vValues, nPars, 0 );
    for ( i = 0; Gia_QbfVerify(p, p->vValues); i++ )
    {
        // generate next constraint
        assert( Vec_IntSize(p->vValues) == p->nVars );
        pCof = Gia_QbfCofactor( pGia, nPars, p->vValues, p->vParMap );
        status = Gia_QbfAddCofactor( p, pCof );
        Gia_ManStop( pCof );
        if ( status == 0 )       { RetValue =  1; break; }
        // synthesize next assignment
        clk = Abc_Clock();
        status = sat_solver_solve( p->pSatSyn, NULL, NULL, (ABC_INT64_T)nConfLimit, 0, 0, 0 );
        p->clkSat += Abc_Clock() - clk;
        if ( fVerbose )
            Gia_QbfPrint( p, p->vValues, i );
        if ( status == l_False ) { RetValue =  1; break; }
        if ( status == l_Undef ) { RetValue = -1; break; }
        // extract SAT assignment
        Gia_QbfOnePattern( p, p->vValues );
        assert( Vec_IntSize(p->vValues) == p->nPars );
        // examine variables
//        Gia_QbfLearnConstraint( p, p->vValues );
//        Vec_IntPrintBinary( p->vValues ); printf( "\n" );
        if ( nIterLimit && i+1 == nIterLimit ) { RetValue = -1; break; }
        if ( nTimeOut && (Abc_Clock() - p->clkStart)/CLOCKS_PER_SEC >= nTimeOut ) { RetValue = -1; break; }
    }
    if ( RetValue == 0 )
    {
        int nZeros = Vec_IntCountZero( p->vValues );
        printf( "Parameters: " );
        assert( Vec_IntSize(p->vValues) == nPars );
        Vec_IntPrintBinary( p->vValues );
        printf( "  Statistics: 0=%d 1=%d\n", nZeros, Vec_IntSize(p->vValues) - nZeros );
    }
    if ( RetValue == -1 && nTimeOut && (Abc_Clock() - p->clkStart)/CLOCKS_PER_SEC >= nTimeOut )
        printf( "The problem timed out after %d sec.  ", nTimeOut );
    else if ( RetValue == -1 && nConfLimit )
        printf( "The problem aborted after %d conflicts.  ", nConfLimit );
    else if ( RetValue == -1 && nIterLimit )
        printf( "The problem aborted after %d iterations.  ", nIterLimit );
    else if ( RetValue == 1 )
        printf( "The problem is UNSAT after %d iterations.  ", i );
    else 
        printf( "The problem is SAT after %d iterations.  ", i );
    if ( fVerbose )
    {
        printf( "\n" );
        Abc_PrintTime( 1, "SAT  ", p->clkSat );
        Abc_PrintTime( 1, "Other", Abc_Clock() - p->clkStart - p->clkSat );
        Abc_PrintTime( 1, "TOTAL", Abc_Clock() - p->clkStart );
    }
    else
        Abc_PrintTime( 1, "Time", Abc_Clock() - p->clkStart );
    Gia_QbfFree( p );
    return RetValue;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

