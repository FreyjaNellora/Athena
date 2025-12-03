cmake_minimum_required(VERSION 3.14)
project(Athena LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


enable_testing()


find_package(CLI11 REQUIRED)
find_package(GTest REQUIRED)
find_package(benchmark REQUIRED)


add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(benchmarks)

