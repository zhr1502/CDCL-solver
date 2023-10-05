### Introduction
An simple conflict-driven clause learning(CDCL) SAT-solver written in C++.

### Develop

#### Environment 
You should develop in GNU-make or Mingw-make environment.

If you're using nix toolchain, just type
```
nix develop
```
to enter the develop shell.

#### Build from the source
Use
```
make all
```
and the binary will be generated as `bin/main`

### Usage
Input the conjunctive normal form (CNF) in DIMACS format to the stdin.

Note that comments is not supported yet.

#### Example
```
[host@host:~]$ ./bin/main
p cnf 10 10
1 4 -5 -7 0
1 5 0
1 7 0
2 4 -9 0
-2 9 -10 0
-3 -8 0
-6 9 0
6 10 0
-7 8 -9 10 0
-9 -10 0
```
Output:
```
Satisfiable: Yes
Possible Truth Assignment:
Variable 1      -> False
Variable 2      -> True
Variable 3      -> False
Variable 4      -> True
Variable 5      -> True
Variable 6      -> True
Variable 7      -> True
Variable 8      -> True
Variable 9      -> True
Variable 10     -> False
```
