cmake_minimum_required(VERSION 3.10)
project(compiler-dungeon)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(compiler-dungeon)
target_sources(compiler-dungeon
    PRIVATE
    src/main.cpp
)
