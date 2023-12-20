#include <algorithm>
#include <iostream>
#include <ratio>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <fstream>
#include <cdcl/cdcl.hpp>
#include <cdcl/cnf.hpp>
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>

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

    for (int i = 1; i <= origin->var_num(); i++) value[i] = Value::Free;

    for (int lit = 0; lit < learned->size_of_clause(index); lit++)
    {
        auto l = learned->locate(index, lit);
        value[l.get_var()] = l.neg() ? Value::True : Value::False;
    }

    while (true)
    {
        bool new_lit_picked = 0;
        for (int c = 0; c < origin->clause_size(); c++)
        {
            bool satisfied = 0;
            int picked_number = 0;
            Literal unpicked_lit(nullptr, 0, 0);
            for (int lit = 0; lit < origin->size_of_clause(c); lit++)
            {
                auto l = origin->locate(c, lit);
                if (value[l.get_var()] == Value::Free)
                {
                    unpicked_lit = l;
                    continue;
                }
                picked_number++;
                if (l.neg() == (value[l.get_var()] == Value::False))
                {
                    satisfied = 1;
                    break;
                }
            }
            if (!satisfied && picked_number == origin->size_of_clause(c) - 1)
            {
                value[unpicked_lit.get_var()] =
                    unpicked_lit.neg() ? Value::False : Value::True;
                new_lit_picked = true;
            }
            if (!satisfied && picked_number == origin->size_of_clause(c))
            {
                origin->push_back(learned->copy_clause(index));
                return true;
            }
        }
        if (!new_lit_picked) break;
    }

    return false;
}

inline bool verify(CDCL &cdcl)
{
    std::ostringstream orig, confl;
    cdcl.stream_dimacs(orig, confl);
    string cstr = confl.str(), ostr = orig.str();

    DIMACS d_c, d_o;
    stringstream stream(cstr);
    d_c.from_stringstream(stream);
    CNF learned_cnf(d_c);

    stringstream stream_a(ostr);
    d_o.from_stringstream(stream_a);
    CNF origin_cnf(d_o);
    if (cdcl.is_sat())
    {
        auto sol = cdcl.sol();
        for (int clause = 0; clause < origin_cnf.clause_size(); clause++)
        {
            bool satisfied = false;
            for (auto lit = 0; lit < origin_cnf.size_of_clause(clause); lit++)
            {
                auto l = origin_cnf.locate(clause, lit);
                if ((l.neg() && sol[l.get_var()] < 0) ||
                    (!l.neg() && sol[l.get_var()] > 0))
                {
                    satisfied = true;
                    break;
                }
            }
            if (!satisfied)
            {
                return false;
            }
        }
        return true;
    }

    //// CDCL solver report UNSAT, now verify it.

    bool unsat = true;

    for (int i = 0; i < learned_cnf.clause_size(); i++)
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
        learned_cnf.push_back(move(empty_clause));

        unsat = check_once(&origin_cnf, &learned_cnf,
                           learned_cnf.clause_size() - 1);
    }

    return unsat;
}

int main(int argv, char *argc[])
{
    int seed = 92822718;
    srand(seed);

    cout << "Tests iteration times & Variables number & Clauses number "
            "(Seperated by whitespace): \n";

    int N, var_num, clause_num;

    bool do_check = true;

    if (argv >= 2)
    {
        for (int i = 1; i < argv; i++)
        {
            if (string(argc[i]) == "--no-check")
            {
                do_check = false;
                break;
            }
            cout << string(argc[i]) << endl;
        }
    }

    cin >> N >> var_num >> clause_num;

    DIMACS dimacs;
    CNF origin_cnf;
    double max_time = 0, min_time = INFINITY, total_time = 0;
    int sat_number = 0, unsat_number = 0;

    if (!do_check) cout << "Enter Benchmark Mode." << endl;

    indicators::show_console_cursor(false);

    using namespace indicators;
    BlockProgressBar bar{
        option::BarWidth{80}, option::ForegroundColor{Color::yellow},
        option::FontStyles{std::vector<FontStyle>{FontStyle::bold}},
        option::MaxProgress{N}};

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

        auto result = do_check ? verify(cdcl) : true;

        if (!result) // Assert: verify result should be true
        {
            bar.set_option(option::ForegroundColor{Color::red});
            bar.mark_as_completed();
            ofstream outfile;
            outfile.open("fail_input.in", ios::out | ios::trunc);
            outfile << dstr << endl;

            cout << "Assertion Failed at test " << iter << endl;
            cout << "Input data dumped in 'fail_input.in'" << endl;
            indicators::show_console_cursor(true);
            return -1;
        }

        if (cdcl.is_sat())
            sat_number++;
        else
            unsat_number++;

        auto duration = end - start;
        auto duration_mili =
            std::chrono::duration<double, std::milli>(duration).count();

        if (duration_mili > max_time) max_time = duration_mili;
        if (duration_mili < min_time) min_time = duration_mili;
        total_time += duration_mili;

        bar.set_option(option::PostfixText{std::to_string(iter) + "/" +
                                           std::to_string(N)});
        bar.tick();
    }

    bar.set_option(option::ForegroundColor{Color::green});
    bar.mark_as_completed();

    cout << "All tests passed. No error reported." << endl;

    cout << endl << "Time consuming:" << endl;
    cout << "Max: " << max_time << "ms  Min: " << min_time
         << "ms  Average: " << total_time / N << "ms" << endl;

    cout << "SAT: " << sat_number << "  UNSAT: " << unsat_number << endl;
    indicators::show_console_cursor(true);

    return 0;
}
