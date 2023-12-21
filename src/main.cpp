#include <iostream>
#include <cdcl/cdcl.hpp>

int main()
{
    //freopen("/home/zhr1502/project/CDCL-solver/fail_input.in","r",stdin);
    DIMACS d;
    d.input();

    CNF cnf(d);

    CDCL cdcl(cnf);
    cdcl.solve();

    cdcl.print();

    return 0;
}
