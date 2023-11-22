cmake_minimum_required(VERSION 3.20)
project(SO LANGUAGES C)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -fsanitize=address,undefined")
endif()

string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_compile_definitions(SOURCE_PATH_SIZE=${SOURCE_PATH_SIZE})

add_executable(Lab1_main Lab1/main.c)

add_executable(Lab2_hello Lab2/hello.c)
add_executable(Lab2_mycopy Lab2/mycopy.c)

add_executable(Lab4_forkls Lab4/forkls.c)
add_executable(Lab4_collatz Lab4/collatz.c Lab4/collatz_child.h)
add_executable(Lab4_ncollatz Lab4/ncollatz.c Lab4/collatz_child.h)

add_executable(Lab5_shmcollatz Lab5/shmcollatz.c)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
add_executable(Lab6_strrev Lab6/strrev.c)
add_executable(Lab6_matmult Lab6/matmult.c)
target_link_libraries(Lab6_strrev Threads::Threads)
target_link_libraries(Lab6_matmult Threads::Threads)
add_executable(Lab7_count Lab7/count.c)
add_executable(Lab7_barrier Lab7/barrier.c)
