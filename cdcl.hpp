#include "cnf.hpp"
#include <vector>
#include <map>
#include <stack>

struct ClauseWrapper;

struct ImpNode;

struct ImpRel;

struct ImpGraph;

struct Assign;

struct CDCL;

enum Value
// Value: Possible value of a variable
{
    Free,
    True,
    False
};

struct Assign
// Assign: an object reflect an assignment to a single variable
{
    int variable_index; // The index of the variable
    Value value;        // The value of the variable
};

struct ClauseWrapper
// ClauseWrapper: Object wraps a raw clause
// that contains useful information in DCL solving
{
    Clause *clause;                // Pointing to raw clause
    std::vector<Value> lits_value; // lits_value[index] indicate the current
                                   // value of this->clause->literals[index]
    std::map<int, int> lits_pos;   // Map from variable number to its index in
                                   // this->clause->literals

    bool satisfied = 0;         // Is the clause satisfied currently
    int picked_lits_number = 0; // Numbers of how many variables appeared in the
                                // clause have been picked.

    /*
     * ClauseWrapper::find_pure_lit(): Find a pure literal in clause and return
     * its ptr Pure literal: The only literal that hasn't been picked in a
     * unsatisfied yet clause Return Value: ptr to the literal when pure literal
     * exists. Otherwise nullptr
     */
    Literal *find_pure_lit();

    /*
     * ClauseWrapper::update():
     * Update this.lits_value this.satisfied and this.picked_lits_number
     * according to the given assign
     * Return Value:
     * true when the given assign triggers a conflict in the clause.
     * Otherwise return false
     */
    bool update(Assign);

    /*
     * ClauseWrapper::global_update():
     * Update this.lits_value this.satisfied and this.picked_lits_number
     * according to the CDCL object given
     * This function is useful when to initialize a newly generated
     * conflict-clause, whose variable assignment do not align with CDCL
     * object Return Value: Refer to update()
     */
    bool global_update(CDCL *);

    ClauseWrapper(Clause *); // Initialize a new ClauseWrapper with raw clause

    void debug();
};

struct ImpRelation
// ImpRelation: Object describe a edge in the CDCL's Implication graph
{
    Clause *relation_clause;
    // The clause according to which this->conclusion holds if this->premise
    // holds
    ImpNode *premise;
    ImpNode *conclusion;
};

struct ImpNode
// ImpNode: Object describe a node in the CDCL's Implication graph
{
    std::vector<ImpRelation *> relation;
    // Set of edges. Either conclusion or premise in these ImpRelation objects
    // is self
    Assign *assign;
    int rank;
    // rank: describe when a node is generated
    // More accurately, the rank of a node depends on the number of the picked
    // variable when it is generated
    // rank is useful when we need to drop some node in the graph due to a new
    // generated clause
};

struct ImpGraph
// ImpGraph: object that describe a CDCL Implication graph
{
    CDCL *cdcl;
    // Pointing to CDCL object the graph belongs to
    std::vector<ImpNode *> nodes;
    std::vector<ImpRelation *> relations;
    std::vector<ImpNode *> map_from_vars_to_nodes;
    // Map from variable to node. If a variable is not assigned (i.e.
    // Value::Free), then map_from_vars_to_nodes[var_index] = nullptr.

    void init(CDCL *);

    /*
     * ImpGraph::construct(std::pair<clause, literal>):
     * Will construct a new node when we assign value to a variable
     * Input: std::pair of a ClauseWrapper and Literal object,
     * The second is the newly assigned variable and the first is the clause by
     * which the variable's value is determined.
     */
    void construct(std::pair<ClauseWrapper *, Literal *>);

    /*
     * ImpGraph::add_node(assign, rank):
     * Add a new node to the implcation graph with the given assign and rank
     * Return Value:
     * ptr pointing to the node
     */
    ImpNode *add_node(Assign *, int);

    /*
     * ImpGraph::add_rel(premise, conclusion, clause):
     * Add a new edge to the implcation graph with the given two nodes
     * (premise and conclusion) and the clause
     * Return Value:
     * ptr pointing to the edge
     */
    ImpRelation *add_rel(ImpNode *, ImpNode *, ClauseWrapper *);

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
    Clause *conflict_clause_gen(ClauseWrapper *);

    /*
     * ImpGraph::drop_to(rank)
     * This will drop all nodes with ranks bigger or equal to rank
     * also drop relevant edge
     */
    void drop_to(int);

    /*
     * ImpNode::drop()
     * drop all nodes and edges
     */
    void drop();

    void debug();
};

struct CDCL
{
    CNF *origin_cnf, *cdcl_cnf;
    std::vector<ClauseWrapper *> conflict_clause;
    std::vector<ClauseWrapper *> clause;
    std::vector<Assign> assignment;
    std::stack<Assign> pick_stack;
    ImpGraph *graph;
    bool satisfiable = false, solved = false;
    // variable CDCL::solved will be set true when CDCL::solve() is called
    // CDCL::satisfiable <= CDCL::solved

    void solve();
    /*
     * CDCL::update(assign, do_return)
     * update every clauses and conflict clauses in the CDCL object
     * When do_return is set to true, the function will return once a conflict
     * is encountered in updating process. Otherwise will update all clause
     * regardless of conflict
     * Return Value: std::pair<ClauseWrapper*, bool>
     * If conflict is encountered then return pair<ptr to conflict clause, true>
     * Else return pair<nullptr, false>
     */
    std::pair<ClauseWrapper *, bool> update(Assign, bool = true);

    /*
     * CDCL::unit_propogation():
     * Do unit propagation. If there is conflict automatically generate conflict
     * clause and push it into self::conflict_clause.
     * Return Value:
     * The first element of std::pair indicate which variables are assigned in propagation
     * The second element of std::pair indicate whether there is conflict
     */
    std::pair<std::vector<Assign>, bool> unit_propogation();

    /*
     * CDCL::choose_variable()
     * Choose a variable that is not picked yet and return a ptr to it.
     * Return Value:
     * ptr to the selected variable's literal
     */
    Literal *choose_variable();
    void init(CNF *);

    void debug();
    void print();
};
