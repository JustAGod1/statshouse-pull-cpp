cmake_minimum_required(VERSION 3.27)

file(GLOB core_headers include/*.h)

add_library(core src/collector.cpp ${core_headers}
        src/core_types.cpp)
target_include_directories(core PUBLIC ".")

