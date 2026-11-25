cmake_minimum_required(VERSION 3.20)
project(orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Lets clangd in the editor see the real compile flags and include paths.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to an optimised build so benchmark timings are meaningful.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(ORDERBOOK_SANITIZE "Build with address and UB sanitizers" OFF)

add_library(orderbook src/orderbook.cpp)
target_include_directories(orderbook PUBLIC include)
target_compile_options(orderbook PRIVATE -Wall -Wextra -Wpedantic)

if(ORDERBOOK_SANITIZE)
    # PUBLIC so everything linking the library is sanitized too.
    # The flag is needed at both compile and link time.
    target_compile_options(orderbook PUBLIC
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(orderbook PUBLIC -fsanitize=address,undefined)
endif()

add_executable(bench bench/main.cpp)
target_link_libraries(bench PRIVATE orderbook)
target_compile_options(bench PRIVATE -Wall -Wextra -Wpedantic)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(tests tests/test_book.cpp)
target_link_libraries(tests PRIVATE orderbook GTest::gtest_main)
target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic)

include(GoogleTest)
gtest_discover_tests(tests)
