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
    void Input();
};

struct Literal
{
    int index;
    bool is_neg;
    // Clause *clause;
    CNF *cnf;
    Literal(CNF *, int, bool);
    void debug();
};

struct Clause
{
    int index;
    std::vector<Literal *> literals;
    CNF *cnf;
    void from_str(CNF *, std::string);
    void from_vec(CNF *, std::vector<int> *); // construct the clause from a
                                              // DIMACS format clause vector
    void debug();
};

struct CNF
{
    std::vector<Clause *> clauses;
    std::vector<Literal *> literals;
    int variable_number = 0;
    void from_DIMACS(DIMACS *);
    Literal *insert_literal(
        int,
        bool); // insert a literal into vector "literals" and return a ptr to it
    void debug();
};

