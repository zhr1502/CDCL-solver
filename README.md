## Introduction
An simple conflict-driven clause learning(CDCL) SAT-solver written in C++.

## Develop

### Environment 
The project can be compiled under GNU, MSVC and Mingw compilers.

If you're using nix toolchain, just type
```
nix develop
```
to enter the develop shell.

### Build from the source
- Submodule
```
git submodule init indicators
git submodule update indicators
```
- Configure
```
cmake -S . -B build
```
- Build
```
cmake --build build
```
The binary targets will be generated as `build/solver` and `build/verify`

## Usage
### Solver
Input the conjunctive normal form (CNF) in DIMACS format to the stdin.

Note that comments is not supported yet.

#### Example
```
[host@host:~]$ ./build/solver
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
### Verify
Input the number of verification rounds and scale of data, and `build/verify` will verify the correctness of the solver on random data.
#### Example
```
[host@host:~]$ ./build/verify
Tests iteration times & Variables number & Clauses number (Separated by whitespace):
1000 100 400
[================================================================================] 1000/1000
[================================================================================] 1000/1000
All tests passed. No error reported.

Time consuming:
Max: 302.033ms  Min: 0.0758ms  Average: 22.559ms
SAT: 931  UNSAT: 69
```
