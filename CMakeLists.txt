cmake_minimum_required(VERSION 3.19)
project(compiler_lab2 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(compiler_lab2
        pl0.c
        PL0.h)
