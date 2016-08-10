#include "base/abc/abc.h"
#include "graph.h"

ABC_NAMESPACE_IMPL_START

unsigned Gra_GraphDeriveTruth( Gra_Graph_t * pGraph )
{
    unsigned uTruth[5] = { 0xFFFF, 0xAAAA, 0xCCCC, 0xF0F0, 0xFF00 };
    unsigned truth = 0;
    unsigned uTruth0, uTruth1, uTruth2;
    Gra_Node_t * pNode;
    int i;

    assert( Gra_GraphLeaveNum(pGraph) >= 0 );
    assert( Gra_GraphLeaveNum(pGraph) <= 5 );

    if( Gra_GraphIsConst(pGraph) )
        return Gra_GraphIsComplement(pGraph)? 0 : ~((unsigned)0);
    if( Gra_GraphIsVar(pGraph) ) 
        return Gra_GraphIsComplement(pGraph)? ~uTruth[Gra_GraphVarInt(pGraph)]:uTruth[Gra_GraphVarInt(pGraph)];

    Gra_GraphForEachLeaf( pGraph, pNode, i )
        pNode->pFunc = (void*)(ABC_PTRUINT_T)uTruth[i];

    Gra_GraphForEachNode( pGraph, pNode, i )
    {
        uTruth0 = (unsigned)(ABC_PTRUINT_T)Gra_GraphNode(pGraph, pNode->eEdge0.Node)->pFunc;
        uTruth1 = (unsigned)(ABC_PTRUINT_T)Gra_GraphNode(pGraph, pNode->eEdge1.Node)->pFunc;
        uTruth2 = (unsigned)(ABC_PTRUINT_T)Gra_GraphNode(pGraph, pNode->eEdge2.Node)->pFunc;
        uTruth0 = pNode->eEdge0.fCompl? ~uTruth0 : uTruth0;
        uTruth1 = pNode->eEdge1.fCompl? ~uTruth1 : uTruth1;
        uTruth2 = pNode->eEdge2.fCompl? ~uTruth2 : uTruth2;

        truth = (uTruth0&uTruth1)|(uTruth0&uTruth2)|(uTruth1&uTruth2);
        pNode->pFunc = (void*)(ABC_PTRUINT_T)truth;
    }
    return Gra_GraphIsComplement(pGraph)? ~truth : truth;
}

ABC_NAMESPACE_IMPL_END
