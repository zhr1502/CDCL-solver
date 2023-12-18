add_library(cdcl STATIC cdcl.cpp cnf.cpp)

target_include_directories(cdcl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
