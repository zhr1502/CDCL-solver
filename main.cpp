#include<iostream>
#include "cdcl.hpp"

int main(){
    //freopen("test/data.in","r",stdin);
    DIMACS d;
    d.Input();

    CNF cnf;
    cnf.from_DIMACS(&d);

    CDCL cdcl;
    cdcl.init(&cnf);
    cdcl.solve();

    cdcl.print();
    //cdcl.debug();
    //fclose(stdin);

    return 0;
}
