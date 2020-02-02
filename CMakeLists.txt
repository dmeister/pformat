cmake_minimum_required(VERSION 3.13)
project(pformat)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g")

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()
FetchContent_MakeAvailable(googletest)

include_directories(/usr/local/include/ include)
add_executable(pformat_benchmark benchmark/fmt_benchmark.cpp
    benchmark/pformat_benchmark.cpp benchmark/benchmark_main.cpp benchmark/printf_benchmark.cpp)

add_executable(pformat_test test/pformat_test.cpp test/test_main.cpp)

find_library(BENCHMARK_LIB benchmark)
find_library(FMT_LIB fmt)
target_link_libraries(pformat_benchmark LINK_PUBLIC ${BENCHMARK_LIB} ${FMT_LIB})
target_link_libraries(pformat_test LINK_PUBLIC gtest_main)

