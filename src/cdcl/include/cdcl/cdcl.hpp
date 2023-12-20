#include "cnf.hpp"
#include "basetype.hpp"
#include "graph.hpp"
#include <sstream>
#include <vector>
#include <map>
#include <stack>
#include <list>
#include <queue>

#pragma once

struct CDCL;

struct CDCL
{
    int variable_number, clause_size;
    ClauseDatabase clausedb;
    std::vector<VariableWrapper> vars;
    std::vector<Assign> assignment;
    std::stack<Assign> pick_stack;
    std::queue<std::pair<Lit, CRef>> unchecked_queue;

    std::vector<int> vars_rank;

    CRef confl, nullCRef;

    ImpGraph *graph = nullptr;
    bool satisfiable = false, solved = false;
    // variable CDCL::solved will be set true when CDCL::solve() is called
    // CDCL::satisfiable implies CDCL::solved

    CDCL(CNF &);
    ~CDCL();

    void analyze(CRef);

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
    CRef update(Assign);

    /*
     * CDCL::add_clause(clause)
     * add a clause to the CDCL clause set. This method will update the member
     * 'vars_contained_clause' and, if the new clause is pickable, add the
     * clause to pickable_clause.
     */
    void add_clause(ClauseWrapper *, std::vector<ClauseWrapper *> &);

    /*
     * CDCL::unit_propogation():
     * Do unit propagation. If there is conflict automatically generate conflict
     * clause and push it into self::conflict_clause.
     * Return Value:
     * The first element of std::pair indicate which variables are assigned in
     * propagation The second element of std::pair indicate whether there is a
     * conflict
     */
    bool unit_propogation();

    /*
     * CDCL::choose_variable()
     * Choose a variable that is not picked yet and return a ptr to it.
     * Return Value:
     * ptr to the selected variable's literal
     */
    Lit choose_variable();

    Lit insert_literal(Variable, bool);

    Value get_lit_value(Lit);
    void init(CNF &);

    void debug();

    void print();
    void print_dimacs();
    void stream_dimacs(std::ostream &, std::ostream &);
};
