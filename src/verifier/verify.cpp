#include <algorithm>
#include <iostream>
#include <ratio>
#include <sstream>
#include <string>
#include <chrono>
#include "check_utility.hpp"

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
    CDCL *cdcl = new CDCL;

    double max_time = 0, min_time = 1.0 / 0.0, total_time = 0;
    int sat_number = 0, unsat_number = 0;

    for (int iter = 1; iter <= N; iter++)
    {
        string dstr = gen_random_str_input(var_num, clause_num);
        std::istringstream stream(dstr);

        dimacs.from_stringstream(stream);
        origin_cnf.from_DIMACS(&dimacs);
        cdcl->init(&origin_cnf);

        auto start = std::chrono::high_resolution_clock::now();
        cdcl->solve();
        auto end = std::chrono::high_resolution_clock::now();

        auto result = verify(cdcl);

        if (!result) // Assert: verify result should be true
        {
            cout << "Assertion Failed at test " << iter << endl;
            cout << "Input: " << endl;
            cout << dstr<< endl;
            return -1;
        }

        if(cdcl->satisfiable) sat_number++;
        else unsat_number++;

        auto duration = end - start;
        auto duration_mili =
            std::chrono::duration<double, std::milli>(duration).count();

        if (duration_mili > max_time) max_time = duration_mili;
        if (duration_mili < min_time) min_time = duration_mili;
        total_time += duration_mili;

        if (iter % 10 == 0) cout << iter << " Assertions passed." << endl;
    }

    cout << "All tests passed. No error reported." << endl;

    cout << endl << "Time consuming:" << endl;
    cout << "Max: " << max_time << "ms  Min: " << min_time
         << "ms  Average: " << total_time / N << "ms" << endl;

    cout << "SAT: " << sat_number << "  UNSAT: " << unsat_number << endl;

    return 0;
}
