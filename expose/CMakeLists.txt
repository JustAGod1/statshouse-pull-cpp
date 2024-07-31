cmake_minimum_required(VERSION 3.27)

set(CMAKE_CXX_STANDARD 20)
file(GLOB expose_headers include/*.h)
file(GLOB expose_sources src/*.cpp)

add_library(expose ${expose_sources} ${expose_headers})
target_include_directories(expose PUBLIC ".")
target_link_libraries(expose core)

