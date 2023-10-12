#include <iostream>
#include "cdcl.hpp"

int main()
{
    DIMACS d;
    d.Input();

    CNF cnf;
    cnf.from_DIMACS(&d);

    CDCL cdcl;
    cdcl.init(&cnf);
    cdcl.solve();

    cdcl.print();
    cdcl.debug();

    return 0;
}
