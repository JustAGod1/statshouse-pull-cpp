cmake_minimum_required(VERSION 3.27)
project(statshouse_pull_cpp)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(core)
add_subdirectory(container)
add_subdirectory(expose)
add_subdirectory(test)

add_executable(main main.cpp)
