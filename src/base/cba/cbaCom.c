/**CFile****************************************************************

  FileName    [cbaCom.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Hierarchical word-level netlist.]

  Synopsis    [Command handlers.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - November 29, 2014.]

  Revision    [$Id: cbaCom.c,v 1.00 2014/11/29 00:00:00 alanmi Exp $]

***********************************************************************/

#include "cba.h"
#include "proof/cec/cec.h"
#include "base/main/mainInt.h"

ABC_NAMESPACE_IMPL_START

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static int  Cba_CommandRead     ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Cba_CommandWrite    ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Cba_CommandPs       ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Cba_CommandPut      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Cba_CommandGet      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Cba_CommandClp      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Cba_CommandCec      ( Abc_Frame_t * pAbc, int argc, char ** argv );
static int  Cba_CommandTest     ( Abc_Frame_t * pAbc, int argc, char ** argv );

static inline Cba_Man_t * Cba_AbcGetMan( Abc_Frame_t * pAbc )                   { return (Cba_Man_t *)pAbc->pAbcCba;                        }
static inline void        Cba_AbcFreeMan( Abc_Frame_t * pAbc )                  { if ( pAbc->pAbcCba ) Cba_ManFree(Cba_AbcGetMan(pAbc));    }
static inline void        Cba_AbcUpdateMan( Abc_Frame_t * pAbc, Cba_Man_t * p ) { Cba_AbcFreeMan(pAbc); pAbc->pAbcCba = p;                  }

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Cba_Init( Abc_Frame_t * pAbc )
{
    Cmd_CommandAdd( pAbc, "New word level", "@read",       Cba_CommandRead,      0 );
    Cmd_CommandAdd( pAbc, "New word level", "@write",      Cba_CommandWrite,     0 );
    Cmd_CommandAdd( pAbc, "New word level", "@ps",         Cba_CommandPs,        0 );
    Cmd_CommandAdd( pAbc, "New word level", "@put",        Cba_CommandPut,       0 );
    Cmd_CommandAdd( pAbc, "New word level", "@get",        Cba_CommandGet,       0 );
    Cmd_CommandAdd( pAbc, "New word level", "@clp",        Cba_CommandClp,       0 );
    Cmd_CommandAdd( pAbc, "New word level", "@cec",        Cba_CommandCec,       0 );
    Cmd_CommandAdd( pAbc, "New word level", "@test",       Cba_CommandTest,      0 );
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
void Cba_End( Abc_Frame_t * pAbc )
{
    Cba_AbcFreeMan( pAbc );
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Cba_CommandRead( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    FILE * pFile;
    Cba_Man_t * p = NULL;
    char * pFileName = NULL;
    int c, fUseAbc = 0, fUsePtr = 0, fVerbose  =    0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "apvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'a':
            fUseAbc ^= 1;
            break;
        case 'p':
            fUsePtr ^= 1;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( argc != globalUtilOptind + 1 )
    {
        printf( "Cba_CommandRead(): Input file name should be given on the command line.\n" );
        return 0;
    }
    // get the file name
    pFileName = argv[globalUtilOptind];
    if ( (pFile = fopen( pFileName, "r" )) == NULL )
    {
        Abc_Print( 1, "Cannot open input file \"%s\". ", pFileName );
        if ( (pFileName = Extra_FileGetSimilarName( pFileName, ".v", ".blif", ".smt", ".cba", NULL )) )
            Abc_Print( 1, "Did you mean \"%s\"?", pFileName );
        Abc_Print( 1, "\n" );
        return 0;
    }
    fclose( pFile );
/*
    // perform reading
    if ( fUseAbc || fUsePtr )
    {
        extern Vec_Ptr_t * Ptr_AbcDeriveDes( Abc_Ntk_t * pNtk );
        Abc_Ntk_t * pAbcNtk = Io_ReadNetlist( pFileName, Io_ReadFileType(pFileName), 0 );
        Vec_Ptr_t * vDes = Ptr_AbcDeriveDes( pAbcNtk );
        p = Cba_PtrTransformToCba( vDes );
        Cba_PtrFree( vDes ); // points to names in pAbcNtk
        if ( p )
        {
            ABC_FREE( p->pSpec );
            p->pSpec = Abc_UtilStrsav( pAbcNtk->pSpec );
        }
        Abc_NtkDelete( pAbcNtk );
    }
    else 
*/
    if ( !strcmp( Extra_FileNameExtension(pFileName), "blif" )  )
        p = Cba_ManReadBlif( pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "v" )  )
        p = Cba_ManReadVerilog( pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "cba" )  )
        p = Cba_ManReadCba( pFileName );
    else 
    {
        printf( "Unrecognized input file extension.\n" );
        return 0;
    }
    Cba_AbcUpdateMan( pAbc, p );
    return 0;
usage:
    Abc_Print( -2, "usage: @read [-apvh] <file_name>\n" );
    Abc_Print( -2, "\t         reads hierarchical design\n" );
//    Abc_Print( -2, "\t-a     : toggle using old ABC parser [default = %s]\n", fUseAbc? "yes": "no" );
//    Abc_Print( -2, "\t-p     : toggle using Ptr construction [default = %s]\n", fUsePtr? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Cba_CommandWrite( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Cba_Man_t * p = Cba_AbcGetMan(pAbc);
    char * pFileName = NULL;
    int fInclineCats =    1;
    int c, fVerbose  =    0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "cvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'c':
            fInclineCats ^= 1;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( p == NULL )
    {
        Abc_Print( 1, "Cba_CommandWrite(): There is no current design.\n" );
        return 0;
    }

    if ( argc == globalUtilOptind + 1 )
        pFileName = argv[globalUtilOptind];
    else if ( argc == globalUtilOptind && p )
        pFileName = Extra_FileNameGenericAppend( Cba_ManName(p), "_out.v" );
    else 
    {
        printf( "Output file name should be given on the command line.\n" );
        return 0;
    }
    // perform writing
    if ( !strcmp( Extra_FileNameExtension(pFileName), "blif" )  )
        Cba_ManWriteBlif( pFileName, p );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "v" )  )
        Cba_ManWriteVerilog( pFileName, p, fInclineCats );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "cba" )  )
        Cba_ManWriteCba( pFileName, p );
    else 
    {
        printf( "Unrecognized output file extension.\n" );
        return 0;
    }
    return 0;
usage:
    Abc_Print( -2, "usage: @write [-cvh]\n" );
    Abc_Print( -2, "\t         writes the design into a file in BLIF or Verilog\n" );
    Abc_Print( -2, "\t-c     : toggle inlining input concatenations [default = %s]\n",  fInclineCats? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n",  fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Cba_CommandPs( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Cba_Man_t * p = Cba_AbcGetMan(pAbc);
    int c, nModules = 0, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "Mvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'M':
            if ( globalUtilOptind >= argc )
            {
                Abc_Print( -1, "Command line switch \"-M\" should be followed by an integer.\n" );
                goto usage;
            }
            nModules = atoi(argv[globalUtilOptind]);
            globalUtilOptind++;
            if ( nModules < 0 )
                goto usage;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( p == NULL )
    {
        Abc_Print( 1, "Cba_CommandPs(): There is no current design.\n" );
        return 0;
    }
    Cba_ManPrintStats( p, nModules, fVerbose );
    return 0;
usage:
    Abc_Print( -2, "usage: @ps [-M num] [-vh]\n" );
    Abc_Print( -2, "\t         prints statistics\n" );
    Abc_Print( -2, "\t-M num : the number of first modules to report [default = %d]\n", nModules );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Cba_CommandPut( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Cba_Man_t * p = Cba_AbcGetMan(pAbc);
    Gia_Man_t * pGia = NULL;
    int c, fBarBufs = 1, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "bvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'b':
            fBarBufs ^= 1;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( p == NULL )
    {
        Abc_Print( 1, "Cba_CommandPut(): There is no current design.\n" );
        return 0;
    }
    pGia = Cba_ManBlast( p, fBarBufs, fVerbose );
    if ( pGia == NULL )
    {
        Abc_Print( 1, "Cba_CommandPut(): Conversion to AIG has failed.\n" );
        return 0;
    }
    Abc_FrameUpdateGia( pAbc, pGia );
    return 0;
usage:
    Abc_Print( -2, "usage: @put [-bvh]\n" );
    Abc_Print( -2, "\t         extracts AIG from the hierarchical design\n" );
    Abc_Print( -2, "\t-b     : toggle using barrier buffers [default = %s]\n", fBarBufs? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Cba_CommandGet( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Cba_Man_t * pNew = NULL, * p = Cba_AbcGetMan(pAbc);
    int c, fMapped = 0, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "mvh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'm':
            fMapped ^= 1;
            break;
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( p == NULL )
    {
        Abc_Print( 1, "Cba_CommandGet(): There is no current design.\n" );
        return 0;
    }

    if ( fMapped )
    {
        if ( pAbc->pNtkCur == NULL )
        {
            Abc_Print( 1, "Cba_CommandGet(): There is no current mapped design.\n" );
            return 0;
        }
        pNew = Cba_ManInsertAbc( p, pAbc->pNtkCur );
    }
    else
    {
        if ( pAbc->pGia == NULL )
        {
            Abc_Print( 1, "Cba_CommandGet(): There is no current AIG.\n" );
            return 0;
        }
        pNew = Cba_ManInsertGia( p, pAbc->pGia );
    }
    Cba_AbcUpdateMan( pAbc, pNew );
    return 0;
usage:
    Abc_Print( -2, "usage: @get [-mvh]\n" );
    Abc_Print( -2, "\t         extracts AIG or mapped network into the hierarchical design\n" );
    Abc_Print( -2, "\t-m     : toggle using mapped network from main-space [default = %s]\n", fMapped? "yes": "no" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Cba_CommandClp( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Cba_Man_t * pNew = NULL, * p = Cba_AbcGetMan(pAbc);
    int c, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( p == NULL )
    {
        Abc_Print( 1, "Cba_CommandGet(): There is no current design.\n" );
        return 0;
    }
    pNew = Cba_ManCollapse( p, CBA_BOX_BUF );
    Cba_AbcUpdateMan( pAbc, pNew );
    return 0;
usage:
    Abc_Print( -2, "usage: @clp [-vh]\n" );
    Abc_Print( -2, "\t         collapses the current hierarchical design\n" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Cba_CommandCec( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    Cba_Man_t * p = Cba_AbcGetMan(pAbc), * pTemp;
    Gia_Man_t * pFirst, * pSecond, * pMiter;
    Cec_ParCec_t ParsCec, * pPars = &ParsCec;
    char * pFileName, * pStr, ** pArgvNew;
    int c, nArgcNew, fDumpMiter = 0;
    FILE * pFile;
    Cec_ManCecSetDefaultParams( pPars );
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'v':
            pPars->fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
    if ( p == NULL )
    {
        Abc_Print( 1, "Cba_CommandCec(): There is no current design.\n" );
        return 0;
    }

    pArgvNew = argv + globalUtilOptind;
    nArgcNew = argc - globalUtilOptind;
    if ( nArgcNew != 1 )
    {
        if ( p->pSpec == NULL )
        {
            Abc_Print( -1, "File name is not given on the command line.\n" );
            return 1;
        }
        pFileName = p->pSpec;
    }
    else
        pFileName = pArgvNew[0];
    // fix the wrong symbol
    for ( pStr = pFileName; *pStr; pStr++ )
        if ( *pStr == '>' )
            *pStr = '\\';
    if ( (pFile = fopen( pFileName, "r" )) == NULL )
    {
        Abc_Print( -1, "Cannot open input file \"%s\". ", pFileName );
        if ( (pFileName = Extra_FileGetSimilarName( pFileName, ".v", ".blif", NULL, NULL, NULL )) )
            Abc_Print( 1, "Did you mean \"%s\"?", pFileName );
        Abc_Print( 1, "\n" );
        return 1;
    }
    fclose( pFile );

    // extract AIG from the current design
    pFirst = Cba_ManBlast( p, 0, 0 );
    if ( pFirst == NULL )
    {
        Abc_Print( -1, "Extracting AIG from the current design has failed.\n" );
        return 0;
    }
    // extract AIG from the second design

    if ( !strcmp( Extra_FileNameExtension(pFileName), "blif" )  )
        pTemp = Cba_ManReadBlif( pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "v" )  )
        pTemp = Cba_ManReadVerilog( pFileName );
    else if ( !strcmp( Extra_FileNameExtension(pFileName), "cba" )  )
        pTemp = Cba_ManReadCba( pFileName );
    else assert( 0 );
    pSecond = Cba_ManBlast( pTemp, 0, 0 );
    Cba_ManFree( pTemp );
    if ( pSecond == NULL )
    {
        Gia_ManStop( pFirst );
        Abc_Print( -1, "Extracting AIG from the original design has failed.\n" );
        return 0;
    }
    // compute the miter
    pMiter = Gia_ManMiter( pFirst, pSecond, 0, 1, 0, 0, pPars->fVerbose );
    if ( pMiter )
    {
        if ( fDumpMiter )
        {
            Abc_Print( 0, "The verification miter is written into file \"%s\".\n", "cec_miter.aig" );
            Gia_AigerWrite( pMiter, "cec_miter.aig", 0, 0 );
        }
        pAbc->Status = Cec_ManVerify( pMiter, pPars );
        //Abc_FrameReplaceCex( pAbc, &pAbc->pGia->pCexComb );
        Gia_ManStop( pMiter );
    }
    Gia_ManStop( pFirst );
    Gia_ManStop( pSecond );
    return 0;
usage:
    Abc_Print( -2, "usage: @cec [-vh]\n" );
    Abc_Print( -2, "\t         combinational equivalence checking\n" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", pPars->fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects []

  SeeAlso     []

******************************************************************************/
int Cba_CommandTest( Abc_Frame_t * pAbc, int argc, char ** argv )
{
    extern void Prs_ManReadVerilogTest();
    //Cba_Man_t * p = Cba_AbcGetMan(pAbc);
    int c, fVerbose  = 0;
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {
        case 'v':
            fVerbose ^= 1;
            break;
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }
/*
    if ( p == NULL )
    {
        Abc_Print( 1, "Cba_CommandTest(): There is no current design.\n" );
        return 0;
    }
*/
    Prs_ManReadVerilogTest();
    return 0;
usage:
    Abc_Print( -2, "usage: @test [-vh]\n" );
    Abc_Print( -2, "\t         experiments with word-level networks\n" );
    Abc_Print( -2, "\t-v     : toggle printing verbose information [default = %s]\n", fVerbose? "yes": "no" );
    Abc_Print( -2, "\t-h     : print the command usage\n");
    return 1;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

