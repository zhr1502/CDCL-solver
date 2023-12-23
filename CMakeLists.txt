cmake_minimum_required(VERSION 3.9)
project(CDCL-solver)

if(WIN32)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/source-charset:utf-8>")
        add_compile_options("$<$<C_COMPILER_ID:MSVC>:/source-charset:utf-8>")
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        add_definitions(-DUNICODE -D_UNICODE -municode)
    endif()
endif()

add_subdirectory(src/cdcl)
add_subdirectory(indicators)

add_executable(solver src/main.cpp)
target_link_libraries(solver cdcl)

add_executable(verify src/verify.cpp)
target_link_libraries(verify cdcl indicators)
