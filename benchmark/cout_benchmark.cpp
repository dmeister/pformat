#include <benchmark/benchmark.h>

#include <sstream>

static char const * const s = "text";

static void BM_Cout(benchmark::State &state) {
    auto n = state.range(0);
    for (auto _ : state) {
        for (long i = 0; i < n; ++i) {
            std::string str;
            benchmark::DoNotOptimize(str);
            std::stringstream ss;
            ss << "foo " << i << " bar " << 2 << "do " << s;
            str = ss.str().c_str();
            benchmark::ClobberMemory();
        }
    }
}
BENCHMARK(BM_Cout)->Range(1, 1 << 4);
