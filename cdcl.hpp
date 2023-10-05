#include "cnf.hpp"
#include <vector>
#include <map>
#include <stack>

struct ClauseWrapper;

struct ImplNode;

struct ImplRel;

struct ImplGraph;

struct Assign;

struct CDCL;

enum Value
{
    Free,
    True,
    False
};

struct ClauseWrapper
{
    Clause *clause;
    std::vector<Value> lits_value;
    std::map<int, int> lits_pos;

    bool satisfied = 0;
    int picked_lits_number = 0;
    Literal *find_pure_lit();
    bool update(Assign);
    bool global_update(CDCL*);
    ClauseWrapper(Clause *);

    void debug();
};

struct Assign
{
    int variable_index;
    Value value;
};

struct ImplRelation
{
    Clause *relation_clause;
    ImplNode *premise;
    ImplNode *conclusion;
};

struct ImplNode
{
    std::vector<ImplRelation *> relation;
    Assign *assign;
    int rank;
};

struct ImplGraph
{
    CDCL *cdcl;
    std::vector<ImplNode *> nodes;
    std::vector<ImplRelation *> relations;
    std::vector<ImplNode *> map_from_vars_to_nodes;

    void init(CDCL *);
    void construct(std::pair<ClauseWrapper *, Literal *>);
    ImplNode *add_node(Assign *, int);
    ImplRelation *add_rel(ImplNode *, ImplNode *, ClauseWrapper *);
    Clause *conflict_clause_gen(ClauseWrapper*);
    void drop_to(int);
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
    ImplGraph *graph;
    bool satisfiable = false, solved = false;

    void solve();
    std::pair<ClauseWrapper *, bool> update(Assign);
    std::pair<std::vector<Assign>, bool> unit_propogation();
    Literal* choose_variable();
    void init(CNF *);

    void debug();
    void print();
};
