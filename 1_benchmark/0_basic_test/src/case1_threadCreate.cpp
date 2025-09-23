#include <benchmark/benchmark.h>

#include <thread>

#include "utils.h"

/*
Experiment 1: Measure the overhead of creating and destroying threads.
    - Create a benchmark that spawns a specified number of threads (e.g., 100, 1000).
    - Each thread should perform a simple task (e.g., sleep for a short duration) and then terminate.

*/

NO_OPTIMIZE void case1_create_threads(benchmark::State& state) {
    const int thread_count = state.range(0);
    for (auto _ : state) {
        for (int i = 0; i < thread_count; ++i) {
            std::thread t([]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            });
            t.detach();
        }
    }
}

BENCHMARK(case1_create_threads)
    ->Arg(100)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond);
