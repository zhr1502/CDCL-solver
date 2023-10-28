#include <string>
#include <vector>
#include <algorithm>
#include "../cdcl.hpp"

using namespace std;
const int MAX_VAR_NUMBER = 200 + 10;

inline string make_clause_str(vector<int> *x, int m)
{
    string clause = "";
    random_shuffle(x->begin(), x->end());

    for (auto var = x->begin(); var < x->begin() + m; var++)
        clause += to_string((*var) * (rand() % 2 * 2 - 1)) + " ";
    return clause + "0\n";
}

inline string gen_random_str_input(int var_num, int clause_num)
{
    string header =
        "p cnf " + to_string(var_num) + " " + to_string(clause_num) + "\n";
    string dimacs = header;
    static vector<int> var;
    var.resize(var_num);
    for (int i = 0; i < var_num; i++) var.at(i) = i + 1;

    for (int i = 1; i <= clause_num; i++) dimacs += make_clause_str(&var, 3);

    return dimacs;
}

inline bool check_once(CNF *origin, CNF *learned, int index)
{
    static Value value[MAX_VAR_NUMBER];

    for (int i = 1; i <= origin->variable_number; i++) value[i] = Value::Free;

    for (auto l : learned->clauses.at(index)->literals)
        value[l->index] = l->is_neg ? Value::True : Value::False;

    while (true)
    {
        bool new_lit_picked = 0;
        for (auto c : origin->clauses)
        {
            bool satisfied = 0;
            int picked_number = 0;
            Literal *unpicked_lit = nullptr;
            for (auto l : c->literals)
            {
                if (value[l->index] == Value::Free)
                {
                    unpicked_lit = l;
                    continue;
                }
                picked_number++;
                if (l->is_neg == (value[l->index] == Value::False))
                {
                    satisfied = 1;
                }
            }
            if (!satisfied && picked_number == (int)c->literals.size() - 1)
            {
                value[unpicked_lit->index] =
                    unpicked_lit->is_neg ? Value::False : Value::True;
                new_lit_picked = true;
            }
            if (!satisfied && picked_number == (int)c->literals.size())
            {
                origin->clauses.push_back(learned->clauses.at(index));
                return true;
            }
        }
        if (!new_lit_picked) break;
    }

    return false;
}

inline bool verify(CDCL *cdcl)
{
    CNF *cnf = cdcl->origin_cnf;
    if (cdcl->satisfiable)
    {
        for (auto clause : cnf->clauses)
        {
            bool satisfied = false;
            for (auto l : clause->literals)
                if ((l->is_neg &&
                     cdcl->assignment[l->index].value == Value::False) ||
                    (!l->is_neg &&
                     cdcl->assignment[l->index].value == Value::True))
                {
                    satisfied = true;
                    break;
                }
            if (!satisfied) return false;
        }
        return true;
    }

    // CDCL solver report UNSAT, now verify it.

    CNF *origin_cnf = new CNF, *learned_cnf = new CNF;
    origin_cnf->variable_number = learned_cnf->variable_number =
        cdcl->origin_cnf->variable_number;

    for (auto c : cdcl->origin_cnf->clauses) origin_cnf->clauses.push_back(c);
    for (auto c : cdcl->cdcl_cnf->clauses) learned_cnf->clauses.push_back(c);

    bool unsat = true;

    for (int i = 0; i < learned_cnf->clauses.size(); i++)
    {
        if (!check_once(origin_cnf, learned_cnf, i))
        {
            unsat = false;
            break;
        }
    }

    if (unsat)
    {
        Clause *empty_clause = new Clause;
        empty_clause->literals.clear();
        learned_cnf->clauses.push_back(empty_clause);

        unsat = check_once(origin_cnf, learned_cnf,
                           learned_cnf->clauses.size() - 1);

        empty_clause->drop();
    }

    origin_cnf->clauses.reserve(0);
    learned_cnf->clauses.reserve(0);

    delete origin_cnf;
    delete learned_cnf;

    return unsat;
}
