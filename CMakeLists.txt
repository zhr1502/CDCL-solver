cmake_minimum_required(VERSION 3.9)
project(CDCL-solver)

add_executable(solver src/main.cpp src/cdcl.cpp src/cnf.cpp)
add_executable(verify src/verifier/verify.cpp src/cdcl.cpp src/cnf.cpp)
