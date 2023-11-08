cmake_minimum_required(VERSION 3.20)
project(SO LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -fsanitize=address,undefined")

add_executable(Lab1_main Lab1/main.c)

add_executable(Lab2_hello Lab2/hello.c)
add_executable(Lab2_mycopy Lab2/mycopy.c)

add_executable(Lab4_forkls Lab4/forkls.c)
add_executable(Lab4_collatz Lab4/collatz.c Lab4/collatz_child.h)
add_executable(Lab4_ncollatz Lab4/ncollatz.c Lab4/collatz_child.h)

add_executable(Lab5_shmcollatz Lab5/shmcollatz.c)
