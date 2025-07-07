cmake_minimum_required(VERSION 3.10)
project(matrix_multiply_test)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 17)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak -fsanitize=address -fsanitize=undefined -g -ggdb3 -Wall -Wextra")
    message(STATUS "Selected ${CMAKE_BUILD_TYPE} build type. Building with sanitizers.")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -ggdb3 -Wall -Wextra -mavx2 -mfma")
endif ()

add_executable(matrix_multiply_test source/main.cpp)
