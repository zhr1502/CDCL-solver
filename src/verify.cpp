#include <algorithm>
#include <iostream>
#include <ratio>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <cdcl/cdcl.hpp>
#include <cdcl/cnf.hpp>

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

    for (auto& l : learned->clauses.at(index).literals)
        value[l.index] = l.is_neg ? Value::True : Value::False;

    while (true)
    {
        bool new_lit_picked = 0;
        for (auto& c : origin->clauses)
        {
            bool satisfied = 0;
            int picked_number = 0;
            Literal *unpicked_lit = nullptr;
            for (auto& l : c.literals)
            {
                if (value[l.index] == Value::Free)
                {
                    unpicked_lit = &l;
                    continue;
                }
                picked_number++;
                if (l.is_neg == (value[l.index] == Value::False))
                {
                    satisfied = 1;
                }
            }
            if (!satisfied && picked_number == (int)c.literals.size() - 1)
            {
                value[unpicked_lit->index] =
                    unpicked_lit->is_neg ? Value::False : Value::True;
                new_lit_picked = true;
            }
            if (!satisfied && picked_number == (int)c.literals.size())
            {
                origin->clauses.push_back(learned->clauses.at(index));
                return true;
            }
        }
        if (!new_lit_picked) break;
    }

    return false;
}

inline bool verify(CDCL &cdcl, CNF &o_cnf)
{
    std::ostringstream _, confl;
    cdcl.stream_dimacs(_, confl);
    string cstr = confl.str();
    if (cdcl.satisfiable)
    {
        for (auto clause : o_cnf.clauses)
        {
            bool satisfied = false;
            for (auto l : clause.literals)
                if ((l.is_neg &&
                     cdcl.assignment[l.index].value == Value::False) ||
                    (!l.is_neg &&
                     cdcl.assignment[l.index].value == Value::True))
                {
                    satisfied = true;
                    break;
                }
            if (!satisfied) return false;
        }
        return true;
    }

    //// CDCL solver report UNSAT, now verify it.

    DIMACS d;
    stringstream stream(cstr);
    d.from_stringstream(stream);
    CNF origin_cnf, learned_cnf(d);
    origin_cnf.variable_number = o_cnf.variable_number;

    for (auto c : o_cnf.clauses) origin_cnf.clauses.push_back(c);

    bool unsat = true;

    for (int i = 0; i < learned_cnf.clauses.size(); i++)
    {
        if (!check_once(&origin_cnf, &learned_cnf, i))
        {
            unsat = false;
            break;
        }
    }

    if (unsat)
    {
        vector<int> emp;
        Clause empty_clause(&learned_cnf, emp);
        empty_clause.literals.clear();
        learned_cnf.clauses.push_back(empty_clause);

        unsat = check_once(&origin_cnf, &learned_cnf,
                           learned_cnf.clauses.size() - 1);
    }

    return unsat;
}

int main()
{
    int seed = 92822718;
    srand(seed);

    std::cout << "Tests iteration times & Variables number & Clauses number "
                 "(Seperated by whitespace): \n";

    int N, var_num, clause_num;

    std::cin >> N >> var_num >> clause_num;

    DIMACS dimacs;
    CNF origin_cnf;
    double max_time = 0, min_time = 1.0 / 0.0, total_time = 0;
    int sat_number = 0, unsat_number = 0;

    for (int iter = 1; iter <= N; iter++)
    {
        string dstr = gen_random_str_input(var_num, clause_num);
        std::istringstream stream(dstr);

        dimacs.from_stringstream(stream);
        origin_cnf.from_DIMACS(dimacs);

        CDCL cdcl(origin_cnf);

        auto start = std::chrono::high_resolution_clock::now();
        cdcl.solve();
        auto end = std::chrono::high_resolution_clock::now();

        auto result = verify(cdcl, origin_cnf);

        if (!result) // Assert: verify result should be true
        {
            cout << "Assertion Failed at test " << iter << endl;
            cout << "Input: " << endl;
            cout << dstr << endl;
            return -1;
        }

        if (cdcl.satisfiable)
            sat_number++;
        else
            unsat_number++;

        auto duration = end - start;
        auto duration_mili =
            std::chrono::duration<double, std::milli>(duration).count();

        if (duration_mili > max_time) max_time = duration_mili;
        if (duration_mili < min_time) min_time = duration_mili;
        total_time += duration_mili;

        if (iter % 10 == 0)
            cout << iter << " Assertions passed." << endl,
                cout << "Time consume: " << duration_mili << "ms" << endl;
    }

    cout << "All tests passed. No error reported." << endl;

    cout << endl << "Time consuming:" << endl;
    cout << "Max: " << max_time << "ms  Min: " << min_time
         << "ms  Average: " << total_time / N << "ms" << endl;

    cout << "SAT: " << sat_number << "  UNSAT: " << unsat_number << endl;

    return 0;
}
