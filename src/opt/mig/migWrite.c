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
#include "base/io/ioAbc.h"

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
void Mig_Write( Abc_Ntk_t * pNtk, Mig_Man_t* pMig, char * pFileName )
{
    int i, j = 0;
    Abc_Obj_t * pAbc;
    Mig_Obj_t * pObj;

    FILE * pFile = fopen( pFileName, "w" );
    fprintf( pFile, "module top ( " );

    Abc_NtkForEachPi( pNtk, pAbc, i )
        fprintf( pFile, "%s, ", Abc_ObjName(pAbc) );
    Abc_NtkForEachPo( pNtk, pAbc, i )
        fprintf(pFile, "%s%s ", Abc_ObjName(pAbc), (i==Abc_NtkPoNum(pNtk)-1)?");":", "); 

    fprintf( pFile, "\ninput " );
    Abc_NtkForEachPi( pNtk, pAbc, i )
        fprintf( pFile, "%s%s ", Abc_ObjName(pAbc), (i==Abc_NtkPiNum(pNtk)-1)?";":", " );
    fprintf( pFile, "\noutput " );
    Abc_NtkForEachPo( pNtk, pAbc, i )
        fprintf(pFile, "%s%s ", Abc_ObjName(pAbc), (i==Abc_NtkPoNum(pNtk)-1)?";":", "); 
    fprintf( pFile, "\nwire " );
    Mig_ManForEachObj( pMig, pObj, i ) 
    {
        if( Mig_ObjIsNode(pObj) )
        {
            j++;  
            fprintf( pFile, "w%d%s", Mig_ObjId(pObj), (j==((Mig_ManObjNum(pMig)-Mig_ManPiNum(pMig)-Mig_ManPoNum(pMig)-1))?";\n":", "));
        }
    }
    Mig_ManForEachObj( pMig, pObj, i )
    {
        if( !Mig_ObjIsNode(pObj) ) continue;
        fprintf( pFile, "assign w%d = ", Mig_ObjId(pObj) );
        if( Mig_Regular(pObj->pFanin0)->Id == 0 )
        {
            Mig_Obj_t * in = pObj->pFanin1; 
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d ", in->Id );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1 " );

            in = pObj->pFanin2; 
            fprintf( pFile, "%s ",  Mig_IsComplement(pObj->pFanin0) ?"&":"|" );
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );  
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d;\n", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "%s;\n", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1;\n" );
        }
        else
        {
            Mig_Obj_t * in = pObj->pFanin0; 
            fprintf(pFile,"(");
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d ", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1" );

            in = pObj->pFanin1;
            fprintf(pFile, "& " );
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );  
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "%s", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1" );

            in = pObj->pFanin0; 
            fprintf(pFile,") | (");
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d ", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1 " );

            in = pObj->pFanin2;
            fprintf(pFile, "& " );
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );  
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "%s", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1" );
   
            in = pObj->pFanin1; 
            fprintf(pFile,") | (");
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d ", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1 " );

            in = pObj->pFanin2;
            fprintf(pFile, "& " );
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );  
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "%s", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1" );
            fprintf(pFile,");\n");
        }
    }
    Mig_ManForEachPo( pMig, pObj, i )
    {
        Mig_Obj_t * in = pObj->pFanin0;
        fprintf( pFile, "assign %s = ", Abc_ObjName(Abc_NtkPo(pNtk, i )) );
        if( Mig_IsComplement(in) ) fprintf( pFile, "~" );  
        in = Mig_Regular(in);
        if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d", Mig_ObjId(in) );
        else if( Mig_ObjIsPi(in) ) fprintf( pFile, "%s", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
        else fprintf( pFile, "1'b1" );

        fprintf(pFile, ";// level ");
        fprintf(pFile, "%d\n", pObj->Level );
    }
    fprintf(pFile,"endmodule");
    fclose(pFile); 
}

void genDfsWithXor( Mig_Man_t * pMan, Mig_Obj_t * pNode, Vec_Ptr_t * dfsList )
{
    Mig_ObjSetTravIdCurrent( pMan, pNode );
    
    if( Mig_ObjIsPi(pNode) || Mig_ObjIsConst(pNode) ) 
    {
        Vec_PtrPush( dfsList, pNode );
        return;
    }
    if( Mig_ObjIsPo(pNode) )
    {
        if( !Mig_ObjIsTravIdCurrent( pMan, Mig_Regular(pNode->pFanin0) ) ) genDfsWithXor( pMan, Mig_Regular(pNode->pFanin0), dfsList );
        Vec_PtrPush( dfsList, pNode );
        return;
    }
    assert( Mig_ObjIsNode(pNode) );
    Cut_Cut_t * pCut;
    if( (pCut = pNode->pData) ){
        assert(pCut->nLeaves==3);
        Mig_Obj_t * pFanin = Mig_ManObj( pMan, pCut->pLeaves[0] );
        if( !Mig_ObjIsTravIdCurrent( pMan, pFanin ) ) genDfsWithXor( pMan, pFanin, dfsList );
        pFanin = Mig_ManObj( pMan, pCut->pLeaves[1] );
        if( !Mig_ObjIsTravIdCurrent( pMan, pFanin ) ) genDfsWithXor( pMan, pFanin, dfsList );
        pFanin = Mig_ManObj( pMan, pCut->pLeaves[2] );
        if( !Mig_ObjIsTravIdCurrent( pMan, pFanin ) ) genDfsWithXor( pMan, pFanin, dfsList );
    }
    else
    {
        if( !Mig_ObjIsTravIdCurrent( pMan, Mig_Regular(pNode->pFanin0) ) ) genDfsWithXor( pMan, Mig_Regular(pNode->pFanin0), dfsList );
        if( !Mig_ObjIsTravIdCurrent( pMan, Mig_Regular(pNode->pFanin1) ) ) genDfsWithXor( pMan, Mig_Regular(pNode->pFanin1), dfsList );
        if( !Mig_ObjIsTravIdCurrent( pMan, Mig_Regular(pNode->pFanin2) ) ) genDfsWithXor( pMan, Mig_Regular(pNode->pFanin2), dfsList );
    }
    Vec_PtrPush( dfsList, pNode );
}

void Mig_WriteWithFadd(Abc_Ntk_t * pNtk, Mig_Man_t * pMig, char * pFileName )
{
    int i, j = 0;
    unsigned uTruth;
    Abc_Obj_t * pAbc;
    Mig_Obj_t * pObj;

    Cut_Man_t * pCutMan;
    Cut_Cut_t * pCut, * pTargetCut;
    Vec_Ptr_t * dfsList = Vec_PtrAlloc( Mig_ManObjNum(pMig) );

    pCutMan = Mig_MigStartCutManForRewrite( pMig );

    
    FILE * pFile = fopen( pFileName, "w" );
    fprintf( pFile, "module top ( " );

    Abc_NtkForEachPi( pNtk, pAbc, i )
        fprintf( pFile, "\\%s , ", Abc_ObjName(pAbc) );
    Abc_NtkForEachPo( pNtk, pAbc, i )
        fprintf( pFile, "\\%s%s ", Abc_ObjName(pAbc), (i==Abc_NtkPoNum(pNtk)-1)?" );":" , "); 

    fprintf( pFile, "\ninput " );
    Abc_NtkForEachPi( pNtk, pAbc, i )
        fprintf( pFile, "\\%s%s ", Abc_ObjName(pAbc), (i==Abc_NtkPiNum(pNtk)-1)?" ;":" , " );
    fprintf( pFile, "\noutput " );
    Abc_NtkForEachPo( pNtk, pAbc, i )
        fprintf(pFile, "\\%s%s ", Abc_ObjName(pAbc), (i==Abc_NtkPoNum(pNtk)-1)?" ;":" , ");
  
    // collect node with XOR3 cut 
    Mig_ManForEachNode( pMig, pObj, i )
    {
        if( pObj->isFix == 0 ) continue;
        if( Mig_Regular(pObj->pFanin0)->Id == 0 ) continue; 
        pCut = Mig_NodeGetCutsRecursive( pCutMan, pObj );
        pTargetCut = 0;
        for( pCut = pCut->pNext; pCut; pCut = pCut->pNext ) 
        {
            if ( pCut->nLeaves != 3 ) continue;
            uTruth = 0x00FF & *Cut_CutReadTruth(pCut);
            if ( uTruth != 0x69 && uTruth != 0x96 ) continue;
            if(pTargetCut) pTargetCut = 0;
            else pTargetCut = pCut;

        }
        if( pTargetCut == 0 ) {
            pObj->pData = 0;
            continue;
        }
        else
            pObj->pData = pTargetCut;
    }
    Mig_ManIncTravId( pMig );
    Mig_ManForEachPo( pMig, pObj, i )
    {
        genDfsWithXor( pMig, pObj, dfsList );
    }
    // write "wire w1, w2 ..."
    fprintf( pFile, "\nwire " );
    j = 0;
    Vec_PtrForEachEntry( Mig_Obj_t *, dfsList, pObj, i )
    {
        if( Mig_ObjIsNode(pObj) )
        {
            j++;
            fprintf( pFile, "w%d%s", Mig_ObjId(pObj), (j==((Vec_PtrSize(dfsList)-Mig_ManPiNum(pMig)-Mig_ManPoNum(pMig)-1))?";\n":", "));
        }
    }
    
    Vec_PtrForEachEntry( Mig_Obj_t *, dfsList, pObj, i )
    {
      if( !Mig_ObjIsNode(pObj) ) continue;
      if( (pCut = pObj->pData) ){
          uTruth = 0x00FF & *Cut_CutReadTruth(pCut);
          if( uTruth == 0x69 ) fprintf( pFile, "xnor ( w%d, ", Mig_ObjId(pObj) );
          else fprintf( pFile, "xor( w%d, ", Mig_ObjId(pObj) );
          
          Mig_Obj_t * in = Mig_ManObj( pMig, pCut->pLeaves[0] );
          if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d, ", in->Id );
          else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s , ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
          else fprintf( pFile, "1'b1," );

          in = Mig_ManObj( pMig, pCut->pLeaves[1] );
          if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d, ", in->Id );
          else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s , ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
          else fprintf( pFile, "1'b1, " );

          in = Mig_ManObj( pMig, pCut->pLeaves[2] );
          if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d );\n", in->Id );
          else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s );\n", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
          else fprintf( pFile, "1'b1 );\n" );
          /* assign type
          fprintf( pFile, "assign w%d = ", Mig_ObjId(pObj) );
          Mig_Obj_t * in = Mig_ManObj( pMig, pCut->pLeaves[0] );
          if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d ", in->Id );
          else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
          else fprintf( pFile, "1'b1 " );
          fprintf(pFile, " ^ " );

          in = Mig_ManObj( pMig, pCut->pLeaves[1] );
          if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d ", in->Id );
          else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
          else fprintf( pFile, "1'b1 " );
          fprintf(pFile, " ^ " );
          
          uTruth = 0x00FF & *Cut_CutReadTruth(pCut);
          assert( uTruth == 0x69 || uTruth == 0x96 );
          if( uTruth == 0x69 ) fprintf( pFile, "~" );
          
          in = Mig_ManObj( pMig, pCut->pLeaves[2] );
          if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d ", in->Id );
          else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
          else fprintf( pFile, "1'b1 " );
          
          fprintf(pFile, " ;\n" );*/
      }
      else {
        fprintf( pFile, "assign w%d = ", Mig_ObjId(pObj) );
        if( Mig_Regular(pObj->pFanin0)->Id == 0 )
        {
            Mig_Obj_t * in = pObj->pFanin1; 
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d ", in->Id );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1 " );

            in = pObj->pFanin2; 
            fprintf( pFile, "%s ",  Mig_IsComplement(pObj->pFanin0) ?"&":"|" );
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );  
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d;\n", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s ;\n", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1;\n" );
        }
        else
        {
            Mig_Obj_t * in = pObj->pFanin0; 
            fprintf(pFile,"(");
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d ", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1" );

            in = pObj->pFanin1;
            fprintf(pFile, "& " );
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );  
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1" );

            in = pObj->pFanin0; 
            fprintf(pFile,") | (");
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d ", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1 " );

            in = pObj->pFanin2;
            fprintf(pFile, "& " );
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );  
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1" );
   
            in = pObj->pFanin1; 
            fprintf(pFile,") | (");
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d ", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1 " );

            in = pObj->pFanin2;
            fprintf(pFile, "& " );
            if( Mig_IsComplement(in) ) fprintf( pFile, "~" );  
            in = Mig_Regular(in);
            if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d", Mig_ObjId(in) );
            else if( Mig_ObjIsPi(in) ) fprintf( pFile, "\\%s ", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
            else fprintf( pFile, "1'b1" );
            fprintf(pFile,");\n");
        }
      }
    }
   
    Mig_ManForEachPo( pMig, pObj, i )
    {
        Mig_Obj_t * in = pObj->pFanin0;
        fprintf( pFile, "assign \\%s = ", Abc_ObjName(Abc_NtkPo(pNtk, i )) );
        if( Mig_IsComplement(in) ) fprintf( pFile, "~" );  
        in = Mig_Regular(in);
        if( Mig_ObjIsNode(in) ) fprintf( pFile, "w%d", Mig_ObjId(in) );
        else if( Mig_ObjIsPi(in) ) fprintf( pFile, " \\%s", Abc_ObjName(Abc_NtkPi(pNtk,  Mig_ObjId(in)-1 )) );
        else fprintf( pFile, "1'b1" );

        fprintf(pFile, ";// level ");
        fprintf(pFile, "%d\n", pObj->Level );
    }
    fprintf(pFile,"endmodule");
    fclose(pFile); 
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


ABC_NAMESPACE_IMPL_END

