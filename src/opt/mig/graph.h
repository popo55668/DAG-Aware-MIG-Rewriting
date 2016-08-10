#ifndef ABC_mig_graph_h
#define ABC_mig_graph_h

ABC_NAMESPACE_HEADER_START

typedef struct Gra_Edge_t_ Gra_Edge_t;
struct Gra_Edge_t_
{
    unsigned        fCompl  :  1;
    unsigned        Node    : 30;
};

typedef struct Gra_Node_t_ Gra_Node_t;
struct Gra_Node_t_
{
    Gra_Edge_t           eEdge0;
    Gra_Edge_t           eEdge1;
    Gra_Edge_t           eEdge2;
    union { int          iFunc;
    void *               pFunc; };
    unsigned             Level     : 29;
    unsigned             fCompl0   :  1;
    unsigned             fCompl1   :  1;
    unsigned             fCompl2   :  1;
    unsigned             simValue;
};

typedef struct Gra_Graph_t_ Gra_Graph_t;
struct Gra_Graph_t_
{
    int          nLeaves;
    int          nSize;     // # of nodes (including the leaves)
    int          nCap;      // # of allocated nodes
    Gra_Node_t * pNodes;    // the array of leaves and internal nodes
    Gra_Edge_t   eRoot;     // the pointer to the topmost node
};


#define Gra_GraphForEachLeaf( pGraph, pLeaf, i )                                                      \
    for( i = 0; (i < (pGraph)->nLeaves) && (((pLeaf)=Gra_GraphNode(pGraph,i)),1); i++ )
#define Gra_GraphForEachNode( pGraph, pNode, i )                                                      \
    for( i = (pGraph)->nLeaves; (i < (pGraph)->nSize) && (((pNode)=Gra_GraphNode(pGraph, i)), 1); i++)

// truth table
extern unsigned Gra_GraphDeriveTruth( Gra_Graph_t * pGraph );

static inline Gra_Edge_t Gra_EdgeCreate( int Node, int fCompl )
{
    Gra_Edge_t e = { fCompl, Node };
    return e;
}

static inline unsigned Gra_EdgeToInt( Gra_Edge_t eEdge )
{
    return (eEdge.Node << 1 ) | eEdge.fCompl;
}

static inline Gra_Edge_t Gra_IntToEdge( unsigned Edge )
{
    return Gra_EdgeCreate( Edge >> 1, Edge & 1 );
}

static inline Gra_Graph_t * Gra_GraphCreate( int nLeaves )
{
    Gra_Graph_t * pGraph;
    pGraph = ABC_ALLOC( Gra_Graph_t, 1 );
    memset( pGraph, 0, sizeof(Gra_Graph_t) );
    pGraph->nLeaves = nLeaves;
    pGraph->nSize = nLeaves;
    pGraph->nCap = 2 * nLeaves + 50;
    pGraph->pNodes = ABC_ALLOC( Gra_Node_t, pGraph->nCap );
    memset( pGraph->pNodes, 0, sizeof(Gra_Node_t) * pGraph->nSize );
    return pGraph;
}

static inline Gra_Graph_t * Gra_GraphCreateConst0()
{
    Gra_Graph_t * pGraph;
    pGraph = ABC_ALLOC( Gra_Graph_t, 1 );
    memset( pGraph, 0, sizeof(Gra_Graph_t) );
    pGraph->nLeaves = 1;      // note that pNodes is not allocated.
    pGraph->eRoot.Node = 0;   // point to const node
    pGraph->eRoot.fCompl = 1;
    pGraph->pNodes = ABC_ALLOC( Gra_Node_t, 5 );
    memset( pGraph->pNodes, 0, sizeof(Gra_Node_t) * 5 );
    return pGraph;
}

static inline Gra_Graph_t * Gra_GraphCreateConst1()
{
    Gra_Graph_t * pGraph;
    pGraph = ABC_ALLOC( Gra_Graph_t, 1 );
    memset( pGraph, 0, sizeof(Gra_Graph_t) );
    pGraph->nLeaves = 1;      // note that pNodes is not allocated.
    pGraph->eRoot.Node = 0;   // point to const node
    pGraph->pNodes = ABC_ALLOC( Gra_Node_t, 5 );
    memset( pGraph->pNodes, 0, sizeof(Gra_Node_t) * 5 );
    return pGraph;
}

static inline Gra_Graph_t * Gra_GraphCreateLeaf( int iLeaf, int nLeaves, int fCompl ) // canon var graph d' = (4,5,1)
{
    Gra_Graph_t * pGraph;
    assert( 0 < iLeaf && iLeaf <= nLeaves ); // iLeaf = 0, 1, 2, 3, 4
    pGraph = Gra_GraphCreate( nLeaves );
    pGraph->eRoot.Node = iLeaf;
    pGraph->eRoot.fCompl = fCompl;
    return pGraph;
}

static inline void Gra_GraphFree( Gra_Graph_t * pGraph )
{ 
    ABC_FREE( pGraph->pNodes );
    ABC_FREE( pGraph );
}

static inline int Gra_GraphIsConst( Gra_Graph_t * pGraph )
{
    return (pGraph->nLeaves == 1);
}

static inline int Gra_GraphIsConst0( Gra_Graph_t * pGraph )
{
    return (pGraph->nLeaves==1) && pGraph->eRoot.fCompl;
}

static inline int Gra_GraphIsConst1( Gra_Graph_t * pGraph )
{
    return (pGraph->nLeaves == 1) && !pGraph->eRoot.fCompl;
}

static inline int Gra_GraphIsVar( Gra_Graph_t * pGraph )
{
    return (pGraph->eRoot.Node < (unsigned)pGraph->nLeaves) && ((unsigned)pGraph->nLeaves != 1);
}

static inline int Gra_GraphIsComplement( Gra_Graph_t * pGraph )
{
    return pGraph->eRoot.fCompl;
}

static inline void Gra_GraphComplement( Gra_Graph_t * pGraph )
{
    pGraph->eRoot.fCompl ^= 1;
}

static inline void Gra_GraphSetRoot( Gra_Graph_t * pGraph, Gra_Edge_t eRoot )
{
    pGraph->eRoot = eRoot;
}

static inline int Gra_GraphLeaveNum( Gra_Graph_t * pGraph )
{ 
    return pGraph->nLeaves;
}

static inline int Gra_GraphNodeNum( Gra_Graph_t * pGraph )
{
    return pGraph->nSize - pGraph->nLeaves;
}

static inline Gra_Node_t * Gra_GraphNode( Gra_Graph_t * pGraph, int i )
{
    return pGraph->pNodes + i;
}

static inline int Gra_GraphNodeInt( Gra_Graph_t * pGraph, Gra_Node_t * pNode )
{
    return pNode - pGraph->pNodes;
}

static inline Gra_Node_t * Gra_GraphVar( Gra_Graph_t * pGraph )
{
    assert( Gra_GraphIsVar(pGraph) );
    return Gra_GraphNode( pGraph, pGraph->eRoot.Node );
}

static inline int Gra_GraphVarInt( Gra_Graph_t * pGraph )
{
    assert( Gra_GraphIsVar(pGraph) );
    return Gra_GraphNodeInt( pGraph, Gra_GraphVar(pGraph) );
}

// creating new node
static inline Gra_Node_t * Gra_GraphAppendNode( Gra_Graph_t * pGraph )
{
    Gra_Node_t * pNode;
    if( pGraph->nSize == pGraph->nCap )
    {
        pGraph->pNodes = ABC_REALLOC( Gra_Node_t, pGraph->pNodes, 2 * pGraph->nCap );
        pGraph->nCap = 2 * pGraph->nCap;
    }
    pNode = pGraph->pNodes + pGraph->nSize++;
    memset( pNode, 0, sizeof(Gra_Node_t) );
    return pNode;
}

static inline Gra_Edge_t Gra_GraphAddNodeMaj( Gra_Graph_t * pGraph, Gra_Edge_t e0, Gra_Edge_t e1, Gra_Edge_t e2 )
{
    Gra_Node_t * pNode;
    pNode = Gra_GraphAppendNode( pGraph );
    pNode->eEdge0 = e0;
    pNode->eEdge1 = e1;
    pNode->eEdge2 = e2;
    pNode->fCompl0 = e0.fCompl;
    pNode->fCompl1 = e1.fCompl;
    pNode->fCompl2 = e2.fCompl;
    return Gra_EdgeCreate( pGraph->nSize - 1, 0 );
}

ABC_NAMESPACE_HEADER_END

#endif
