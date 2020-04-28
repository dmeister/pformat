cmake_minimum_required(VERSION 3.13)
project(pformat)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g")

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
)
FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
)
FetchContent_Declare(
  fmt 
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        6.2.0
)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()
FetchContent_MakeAvailable(googletest)

FetchContent_GetProperties(benchmark)
if(NOT benchmark_POPULATED)
  FetchContent_Populate(benchmark)
  add_subdirectory(${benchmark_SOURCE_DIR} ${benchmark_BINARY_DIR})
endif()
FetchContent_MakeAvailable(benchmark)

FetchContent_GetProperties(fmt)
if(NOT fmt_POPULATED)
  FetchContent_Populate(fmt)
  add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR})
endif()
FetchContent_MakeAvailable(fmt)

include_directories(/usr/local/include/ include)
add_executable(pformat_benchmark benchmark/fmt_benchmark.cpp
    benchmark/pformat_benchmark.cpp benchmark/benchmark_main.cpp benchmark/printf_benchmark.cpp benchmark/cout_benchmark.cpp)

add_executable(pformat_test test/pformat_test.cpp test/test_main.cpp)

target_link_libraries(pformat_benchmark LINK_PUBLIC benchmark fmt)
target_link_libraries(pformat_test LINK_PUBLIC gtest_main)

