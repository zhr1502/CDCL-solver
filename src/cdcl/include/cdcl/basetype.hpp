#include <vector>
#include <list>
#include "cnf.hpp"

#pragma once

struct CDCL;

using Variable = int;

enum Value
// Value: Possible value of a variable
{
    Free,
    True,
    False
};

std::ostream &operator<<(std::ostream &, const Value &);

struct Assign
// Assign: an object reflect an assignment to a single variable
{
    int variable_index; // The index of the variable
    Value value;        // The value of the variable
};

class Lit
{
    int lit;

public:
    Lit(int, bool);
    int get_var();
    bool is_neg();

    bool operator==(const Lit &a) { return a.lit == lit; }
    bool operator!=(const Lit &a) { return a.lit != lit; }
};

class CRef;

class VariableWrapper
{
    Variable var;
    CDCL &cdcl;
    std::list<CRef> pos_watcher, neg_watcher;

public:
    VariableWrapper(Variable, CDCL &);
    CRef update_watchlist(Assign);
    std::list<CRef> &get_watchlist(Assign);
    void watchlist_pushback(CRef, Lit);
    Value get_value();
};

const Lit nullLit = Lit(0, true);

struct ClauseWrapper
// ClauseWrapper: Clause object used in clause database
{
    int index;

    std::vector<Lit> literals;

    /*
     * ClauseWrapper::find_pure_lit(): Find a pure literal in clause and return
     * its ptr Pure literal: The only literal that hasn't been picked in a
     * unsatisfied yet clause Return Value: ptr to the literal when pure literal
     * exists. Otherwise nullptr
     */
    Lit update_watcher(Variable, CDCL &);

    Lit get_blocker(Variable);

    /*
     * ClauseWrapper::update():
     * Update this.lits_value this.satisfied and this.picked_lits_number
     * according to the given assign
     * Return Value:
     * true when the given assign triggers a conflict in the clause.
     * Otherwise return false
     */
    bool update(Assign);

    bool is_unit(Lit, CDCL *);

    ClauseWrapper(Clause &,
                  int); // Initialize a new ClauseWrapper with raw clause
    void drop();

    void debug();
};

class ClauseDatabase
{
    friend class LRef;
    friend class CRef;
    std::vector<ClauseWrapper> clause;

public:
    void parse();
    void add_clause(Clause);
    CRef get_clause(int);
    CRef get_last_cls();
    int size();
};

class CRef
{
    int cid;
    ClauseDatabase &db;

public:
    CRef(int, ClauseDatabase &);
    bool isnull();

    ClauseWrapper &operator*() { return db.clause.at(cid); }
    bool operator==(const CRef &a) { return cid == a.cid; }
    bool operator!=(const CRef &a) { return cid != a.cid; }
    CRef &operator=(const CRef &a)
    {
        cid = a.cid;
        db = a.db;
        return *this;
    }
};
