# pformat

This library implements simple compile-time string formatting with a focus of formatting logging messages (clang/gcc and C++17 only).

## Example

A simple example first:

```
uint64_t segment_id = 27;
auto s = "Page {} failed: {}"_fmt(segment_id, "EIO");
s == "Page 27 failed: EIO";
```

As one might expect, the `{}` are replaced with the two
arguments provided to the format method. The syntax is similar
to Python's string formatting or [fmt library](fmt).

It is way more interesting what happens in case of errors:

```
uint64_t segment_id = 27;
auto s = "Page {} failed: {}"_fmt(segment_id);
```

```
[file/line omitted]: error: static_assert failed
      due to requirement 'parameter_count_match' "Number of format arguments does not match format
      string"
    static_assert(parameter_count_match,
    ^             ~~~~~~~~~~~~~~~~~~~~~
```

There are two `{}`, but only a single parameter. This is usually
a runtime error. However, with pformat this is a compile-time error.
This is the most important feature of pformat. It checks that the
number of format parameters and the number of arguments match and
that it can format all the arguments provided.

The error message, in this case, is quite clear to C++ template
games standards.

## Why pformat?

1. Compile-time checking

   Imaging a loggine line somewhere in error handling code
   `d_.error("Segment {} flush at page {} failed due to reason {}, seg_io, reason_str);`

   The logging might fail. Either the logging system detects the issue and might print
   `Segment 17 flush at page EIO failed to reason (NOTSET)` or it crashes or throws an exception. None of these are
   really acceptable outcomes. Logging should never crash the system and hiding
   the bug isn't great either. Unless the log files are carefully scanned for these,
   this kind of bugs might never be found and the information is missing exactly when
   it is needed.

2. Performance

   Performance was not the main design goal, but it is much easier to generate
   fast code when all information are available at compile time.The main job of the
   code isn't logging, so it is important to not waste CPU. We are using C++ for a reason,
   aren't we?

### How is it different from std::format facility?

The formatting facility of C++20, std::format, (which is AFAIK based on the [fmt library](fmt)) is a very good formatting library. However, to the best of my understanding
fmt does all the parsing and error reporting at runtime (possibly throwing exceptions).

Clearly, it is way more flexible in the formatting configuration, e.g. how many characters after the decimal sign for floating point number and so on. However, these formatting
facilities are much less important for logging.

### How is this different from a logging framework?

This library focusses on the formatting aspect for logging. There are other important
aspects like enabling/disable of contexts, routing to sinks (e.g. files). None of
these topics is covered by this library.

## Implementation

It uses the clang/gnu extension called [N3599](N3599) (actually NF3599 mentions compile-time printf as one of the use cases).
This library does not support MSVC. It requires C++17 support.

The library is currently only tested with Apple clang version 11.0 on x64.

## How to extent for own types

At this point, pformat has builtin support for

- integral and floating point numbers
- bool
- enums
- char const \*, std::string, std::string_view

It can be extended for user-defined types by implementing
a format_extention_type or by using ADL. The tests contain
examples for both.

## Performance Results

Performance was not the main design criteria, but it directly
results from being able to make all decisions that can be
made at compile time at compile time. It also solves
an easier problem than the full fletched, highly-customizable
formatting approaches like fmt or printf. This is results
in a exceptional performance.

The benchmark setup is formatting the fmt `foo {} bar {} do {}`
with the iteration count for the first parameter, `2` for the second and `str`
for the third.

```
./pformat_benchmark
2020-02-02 10:27:56
Running ./pformat_benchmark
Run on (4 X 1100 MHz CPU s)
CPU Caches:
  L1 Data 32K (x2)
  L1 Instruction 32K (x2)
  L2 Unified 262K (x2)
  L3 Unified 4194K (x1)
Load Average: 2.12, 2.58, 2.89
--------------------------------------------------------
Benchmark              Time             CPU   Iterations
--------------------------------------------------------
BM_Fmt/1             130 ns          126 ns      6100962
BM_Fmt/8            1081 ns         1025 ns       514971
BM_Fmt/16           2039 ns         2009 ns       336180
BM_PFormat/1        24.1 ns         23.8 ns     31434820
BM_PFormat/8         234 ns          224 ns      3122560
BM_PFormat/16        425 ns          420 ns      1627180
BM_Printf/1          324 ns          318 ns      2245209
BM_Printf/8         1925 ns         1919 ns       324811
BM_Printf/16        4095 ns         4077 ns       178203
```

If these numbers are correct (I am new to micro-benchmarking
and the numbers are surprisingly low), pformat is 10x faster
then printf.

The fmt project refers to [this benchmark](fmt_benchmark). The
formatting tests with complex formatting equivalent to the printf
string `%0.10f:%04d:%+g:%s:%p:%c:%%\n`. Such formatting is not
possible with pformat. And IMHO it is not needed for the use case.
You pay (with performance) for features one isn't using for
logging.

## Contact

Please contact me (see [Github profile](github)) if you have comments for find issues.

## Links

- [fmt] https://github.com/fmtlib/fmt
- [N3599] http://open-std.org/JTC1/SC22/WG21/docs/papers/2013/n3599.html
- [fmt_benchmark] https://github.com/fmtlib/format-benchmark/blob/master/tinyformat_test.cpp
- [profile] https://github.com/dmeister
