#include <iostream>
#include <cdcl/cdcl.hpp>

int main()
{
    DIMACS d;
    d.Input();

    CNF cnf(d);

    CDCL cdcl(cnf);
    cdcl.solve();

    cdcl.print();

    return 0;
}
