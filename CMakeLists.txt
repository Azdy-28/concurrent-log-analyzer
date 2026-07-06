cmake_minimum_required(VERSION 3.10)
project(LogParsingEngine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

find_package(Threads REQUIRED)

add_executable(log_parser src/main.cpp)
target_include_directories(log_parser PRIVATE include)
target_link_libraries(log_parser PRIVATE Threads::Threads)

option(BUILD_TESTS "Build test binaries" OFF)
if(BUILD_TESTS)
    enable_testing()

    add_executable(test_tokenizer tests/test_tokenizer.cpp)
    target_include_directories(test_tokenizer PRIVATE include)
    add_test(NAME tokenizer_tests COMMAND test_tokenizer)

    add_executable(verify_zero_alloc tests/verify_zero_alloc.cpp)
    target_include_directories(verify_zero_alloc PRIVATE include)
    target_link_libraries(verify_zero_alloc PRIVATE Threads::Threads)
endif()
