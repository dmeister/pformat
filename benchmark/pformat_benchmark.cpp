#include <benchmark/benchmark.h>
#include <pformat/pformat.h>

static char const * const s = "text";

static void BM_PFormat(benchmark::State &state) {
    using namespace pformat;
    auto n = state.range(0);
    // Perform setup here
    for (auto _ : state) {
        constexpr auto compiled_format = "foo {} bar {} do {}"_fmt;
        for (long i = 0; i < n; ++i) {
            char buf[100];
            benchmark::DoNotOptimize(buf);
            auto end = compiled_format.format_to(buf, i, 2, s);
            *end = 0;
            benchmark::ClobberMemory();
        }
    }
}
BENCHMARK(BM_PFormat)->Range(1, 1 << 4);
