#include <iostream>
#include "cdcl.hpp"

int main()
{
    // freopen("/home/zhr1502/project/CDCL-solver/data.in","r",stdin);
    DIMACS d;
    d.Input();

    CNF cnf;
    cnf.from_DIMACS(&d);

    CDCL cdcl;
    cdcl.init(&cnf);
    cdcl.solve();

    cdcl.print();
    std::cout << cdcl.conflict_clause.size() << std::endl;
    // for (auto c : cdcl.conflict_clause) c->debug();

    return 0;
}
