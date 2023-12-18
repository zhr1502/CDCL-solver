cmake_minimum_required(VERSION 3.9)
project(CDCL-solver)

add_subdirectory(src/cdcl)

add_executable(solver src/main.cpp)
target_link_libraries(solver cdcl)
add_executable(verify src/verify.cpp)
target_link_libraries(verify cdcl)
