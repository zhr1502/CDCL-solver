#include <vector>
#include <queue>
#include "basetype.hpp"
#pragma once
class ImpNode;

class ImpGraph
// ImpGraph: object that describe a CDCL Implication graph
{
    // Pointing to CDCL object the graph belongs to
    // std::vector<ImpNode *> fixed_var_nodes;
    // std::vector<ImpRelation *> relations;
    std::vector<ImpNode *> vars_to_nodes;
    std::vector<std::vector<ImpNode *>> trail;
    std::vector<int> assigned_order;
    // Map from variable to node. If a variable is not assigned (i.e.
    // Value::Free), then map_from_vars_to_nodes[var_index] = nullptr.

public:
    ImpGraph(CDCL &);
    ~ImpGraph();
    /*
     * ImpGraph::pick_var(std::pair<clause, literal>):
     * Will construct a new node when we assign value to a variable
     * Input: std::pair of a ClauseWrapper and Literal object,
     * The second is the newly assigned variable and the first is the clause by
     * which the variable's value is determined.
     */
    int pick_var(CRef, Lit);

    /*
     * ImpGraph::add_node(assign, rank):
     * Add a new node to the implcation graph with the given assign and rank
     * Return Value:
     * ptr pointing to the node
     */
    ImpNode *add_node(Assign, int);

    /*
     * ImpGraph::add_rel(premise, conclusion, clause):
     * Add a new edge to the implcation graph with the given two nodes
     * (premise and conclusion) and the clause
     * Return Value:
     * ptr pointing to the edge
     */

    /*
     * ImpGraph::conflict_clause_gen(clause):
     * If the clause given cause a conflict, this function will generate a
     * conflict raw clause and return it.
     * Attention: When the given clause do not cause a conflict, this function's
     * behavior is undefined. If the variable whose value is Value::Free is
     * encountered while calculating, the whole program will abort()
     * Return Value:
     * The generated raw clause
     */
    Clause conflict_clause_gen(CRef);

    /*
     * ImpGraph::drop_to(rank)
     * This will drop all nodes with ranks bigger or equal to parameter 'rank'
     * also drop relevant edge
     */
    std::queue<Assign> drop_to(int);

    /*
     * ImpNode::drop()
     * drop all nodes and edges
     */
    void add_reason(ImpNode *, ImpNode *, CRef);
    void debug();
};

class ImpNode
// ImpNode: Object describe a node in the CDCL's Implication graph
{
    std::vector<ImpNode *> in_node, out_node;
    std::vector<CRef> in_reason, out_reason;
    // Set of edges. Either conclusion or premise in these ImpRelation objects
    // is self
    Assign assign;
    int rank;
    // rank: describe when a node is generated
    // More accurately, the rank of a node depends on the number of the picked
    // variable when it is generated
    // rank is useful when we need to drop some node in the graph due to a new
    // generated clause
    bool fixed = false;

public:
    ImpNode(Assign, int, bool = false);
    ~ImpNode();
    friend void ImpGraph::add_reason(ImpNode *, ImpNode *, CRef);
    int get_rank();
    int get_var_index();
    Assign get_assign();

    const std::vector<ImpNode *> &get_in_node();

    void debug();
};
