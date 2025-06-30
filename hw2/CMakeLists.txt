cmake_minimum_required(VERSION 3.22)

project(hw2)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

SET(SOURCE_FILES
        source/FileHandler.cpp
)

include_directories(source)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} source/main.cpp)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(${PROJECT_NAME} PRIVATE -g -O0 -Wall -Wextra -Werror)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -DNDEBUG)
endif ()

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

SET(SOURCE_TEST_FILES
        tests/TestFileHandler.cpp
)

add_executable(test_${PROJECT_NAME} ${SOURCE_FILES} ${SOURCE_TEST_FILES})
target_link_libraries(test_${PROJECT_NAME} PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_${PROJECT_NAME})
