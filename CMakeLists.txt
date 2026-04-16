cmake_minimum_required(VERSION 3.21)
project(ga_lab CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ga_core
    src/ga/rng.cpp
    src/ga/problem.cpp
    src/ga/chromosome.cpp
    src/ga/population.cpp
    src/ga/ga.cpp
    src/io/config.cpp
    src/io/logger.cpp)
target_include_directories(ga_core PUBLIC src)
target_compile_options(ga_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(ga src/main.cpp)
target_compile_options(ga PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(ga PRIVATE ga_core)
