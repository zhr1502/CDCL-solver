#include <iostream>
#include "cdcl.hpp"

int main()
{
    //freopen("./test/data.in","r",stdin);
    DIMACS d;
    d.Input();

    CNF cnf;
    cnf.from_DIMACS(&d);

    CDCL cdcl;
    cdcl.init(&cnf);
    cdcl.solve();

    // cdcl.print_dimacs();
    cdcl.print();
    // std::cout << "Satisfiable: " << (cdcl.satisfiable ? "Yes" : "No") << std::endl;
    // cdcl.debug();
    // for (auto c: cdcl.conflict_clause) c->debug();

    return 0;
}
