#include <benchmark/benchmark.h>

#include <cstdio>

static void BM_Printf(benchmark::State &state) {
    auto n = state.range(0);
    for (auto _ : state) {
        for (long i = 0; i < n; ++i) {
            char buf[100];
            benchmark::DoNotOptimize(buf);
            std::snprintf(buf, 100, "foo %ld bar %d do %s", i, 2, "buf");
            benchmark::ClobberMemory();
        }
    }
}
BENCHMARK(BM_Printf)->Range(1, 1 << 4);
