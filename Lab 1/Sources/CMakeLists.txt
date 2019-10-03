# cmake_minimum_required(VERSION <specify CMake version here>)
project(Sources C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(Sources
        FIFO.h
        main.c
        packet.h
        types.h
        UART.h)
