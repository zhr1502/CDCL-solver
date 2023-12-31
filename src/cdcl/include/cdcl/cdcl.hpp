#include "cnf.hpp"
#include "basetype.hpp"
#include "graph.hpp"
#include "heuristic.hpp"
#include <sstream>
#include <vector>
#include <map>
#include <stack>
#include <list>
#include <queue>

#pragma once

class CDCL;

class CDCL
{
    friend class VariableWrapper;
    friend struct ClauseWrapper;
    friend class CRef;

private:
    int variable_number, clause_size;
    int restart_r = 100;
    ClauseDatabase clausedb;
    std::vector<VariableWrapper> vars;
    std::vector<Assign> assignment;
    std::stack<Assign> pick_stack;
    std::queue<std::pair<Lit, CRef>> unchecked_queue;

    std::vector<int> vars_rank;

    std::queue<CRef> confl;

    CRef nullCRef;

    ImpGraph graph;
    Heap vsids;
    bool satisfiable = false, solved = false;
    int unpick_to_rank;
    // variable CDCL::solved will be set true when CDCL::solve() is called
    // CDCL::satisfiable implies CDCL::solved

    void update(Assign);
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

    int unit_propogation();
    /*
     * CDCL::unit_propogation():
     * Do unit propagation. If there is conflict automatically generate conflict
     * clause and push it into self::conflict_clause.
     * Return Value:
     * The first element of std::pair indicate which variables are assigned in
     * propagation The second element of std::pair indicate whether there is a
     * conflict
     */

    void analyze(CRef);

    Lit choose_variable();
    /*
     * CDCL::choose_variable()
     * Choose a variable that is not picked yet and return a ptr to it.
     * Return Value:
     * ptr to the selected variable's literal
     */

    Value get_lit_value(Lit);
    bool parse_clause(Clause&);

public:
    CDCL(CNF &);

    int get_variable_num();

    int get_clause_size();

    void solve(int);

    bool is_solved();
    bool is_sat();
    std::vector<int> sol();

    void debug();
    void print();
    void print_dimacs();
    void stream_dimacs(std::ostream &, std::ostream &);
};
