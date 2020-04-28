#include <benchmark/benchmark.h>
#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <vector>

#include "fmt/compile.h"

static char const * const s = "text";

static void BM_Fmt(benchmark::State &state) {
    auto n = state.range(0);
    for (auto _ : state) {
        constexpr auto compiled_format = fmt::compile<int, int, const char *>(
            FMT_STRING("foo {} bar {} do {}"));
        for (long i = 0; i < n; ++i) {
            char buf[100];
            benchmark::DoNotOptimize(buf);
            auto end = fmt::format_to(buf, compiled_format, i, 2, s);
            *end = 0;
            benchmark::ClobberMemory();
        }
    }
}
BENCHMARK(BM_Fmt)->Range(1, 1 << 4);