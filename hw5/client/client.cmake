cmake_minimum_required(VERSION 3.22)

project(hw2)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

SET(SOURCE_FILES
        client/main.cpp
        client/ExprGenerator.h
        client/ConnectionsHandler.h
)

add_executable(client ${SOURCE_FILES})

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(client PRIVATE -g -O0 -Wall -Wextra -Werror)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(client PRIVATE -O3 -DNDEBUG)
endif ()