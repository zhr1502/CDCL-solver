add_library(cdcl STATIC cdcl.cpp cnf.cpp graph.cpp basetype.cpp)

target_include_directories(cdcl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
