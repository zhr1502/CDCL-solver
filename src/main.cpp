#include <iostream>
#include <cdcl/cdcl.hpp>

int main()
{
    DIMACS d;
    d.input();

    CNF cnf(d);

    CDCL cdcl(cnf);
    cdcl.solve(758926699);

    cdcl.print();

    return 0;
}
