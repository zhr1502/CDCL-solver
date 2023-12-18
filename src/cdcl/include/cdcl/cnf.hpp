#pragma once
#include <istream>
#include <string>
#include <vector>

struct Literal;

struct Clause;

struct CNF;

struct DIMACS
{
    std::string header;
    std::vector<std::vector<int>> clauses;
    std::string form = "cnf";
    int literal_num = 0, clause_num = 0;
    void from_stringstream(std::istream&), Input();
};

struct Literal
{
    int index;
    bool is_neg;
    // Clause *clause;
    CNF* cnf;
    Literal(CNF* , int, bool);
    void debug();
    void drop();
};

struct Clause
{
    int index;
    std::vector<Literal> literals;
    CNF* cnf;

    Clause();
    Clause(CNF* , std::string);
    Clause(CNF*, std::vector<int>&); // construct the clause from a
                                              // DIMACS format clause vector

    void debug();
    void drop();
};

struct CNF
{
    std::vector<Clause> clauses;

    int variable_number = 0;

    CNF();
    CNF(DIMACS&);
    void from_DIMACS(DIMACS&);
    //Literal& insert_literal(
    //    int,
    //    bool); // insert a literal into vector "literals" and return a ptr to it
    void debug();
    void drop();
};

