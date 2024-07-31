cmake_minimum_required(VERSION 3.27)

set(CMAKE_CXX_STANDARD 20)
file(GLOB container_headers include/*.h)
file(GLOB container_sources src/*.cpp)

add_library(container ${container_sources} ${container_headers}
        src/pregen.cpp
        include/time_framed_container.h)
target_include_directories(container PUBLIC ".")
target_link_libraries(container core)

