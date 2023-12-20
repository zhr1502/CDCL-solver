#pragma once
#include <istream>
#include <string>
#include <vector>

class Clause;

class Literal;

class DIMACS;

class CNF
{
    std::vector<Clause> clauses;

    int variable_number = 0;

public:
    CNF();
    CNF(DIMACS&);
    void from_DIMACS(DIMACS&);
    void debug();
    void push_back(Clause &&);
    int clause_size();
    int var_num();
    int size_of_clause(int);
    Literal locate(int, int);
    Clause copy_clause(int);
};

class DIMACS
{
    friend void CNF::from_DIMACS(DIMACS &);
    std::string header;
    std::vector<std::vector<int>> clauses;
    std::string form = "cnf";
    int literal_num = 0, clause_num = 0;

public:
    void from_stringstream(std::istream&);
    void input();
};

class Literal
{
    int index;
    bool is_neg;
    // Clause *clause;
    CNF* cnf;

public:
    Literal(CNF*, int, bool);
    int get_var();
    bool neg();
    void debug();
};

class Clause
{
    int index;
    std::vector<Literal> literals;
    CNF* cnf;

public:
    Clause();
    Clause(CNF*, std::string);
    Clause(CNF*, std::vector<int>&); // construct the clause from a
                                     // DIMACS format clause vector
    Clause(const Clause &);
    std::vector<Literal>& get_literals();

    void debug();
};
