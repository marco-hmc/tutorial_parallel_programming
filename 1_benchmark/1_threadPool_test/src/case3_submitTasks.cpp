#include <benchmark/benchmark.h>

#include <future>
#include <numeric>
#include <thread>
#include <vector>

#include "threadPool.h"
#include "utils.h"

namespace {

    NO_OPTIMIZE std::vector<int> heavyComputation(int size) {
        std::vector<int> result(size);
        for (int i = 0; i < size; ++i) {
            result[i] = i * i;  // 模拟复杂计算
        }
        return result;
    }

    NO_OPTIMIZE void serialComputation(const std::vector<int>& data) {
        int sum = 0;
        for (int value : data) {
            sum += value % 7;  // 模拟复杂的串行计算
        }
        benchmark::DoNotOptimize(sum);
    }

    using namespace StdThreadPool;
    ThreadPool gPool(std::thread::hardware_concurrency());

}  // namespace

void case3_submitTasks(benchmark::State& state) {
    const int tasks_nums = state.range(0);  // 任务数量
    const int vector_size = state.range(1);  // 每个任务生成的 vector 大小

    for (auto _ : state) {
        std::vector<int> task_indices(tasks_nums);
        std::iota(task_indices.begin(), task_indices.end(), 0);

        // 使用 submitTasks 提交任务
        auto futures = gPool.submitTasks(
            task_indices.begin(), task_indices.end(),
            [vector_size](int) { return heavyComputation(vector_size); });

        // 不等待所有任务完成，直接开始串行计算
        for (auto& future : futures) {
            serialComputation(future.get());
        }
    }
}

void case3_submitTask(benchmark::State& state) {
    const int tasks_nums = state.range(0);  // 任务数量
    const int vector_size = state.range(1);  // 每个任务生成的 vector 大小

    for (auto _ : state) {
        std::vector<std::future<std::vector<int>>> futures;
        futures.reserve(tasks_nums);

        for (int i = 0; i < tasks_nums; ++i) {
            futures.push_back(gPool.submitTask(
                [vector_size]() { return heavyComputation(vector_size); }));
        }

        // 等待所有任务完成后，再进行串行计算
        std::vector<std::vector<int>> results;
        results.reserve(tasks_nums);
        for (auto& future : futures) {
            results.push_back(future.get());
        }

        for (const auto& result : results) {
            serialComputation(result);
        }
    }
}

// 注册基准测试，双参数：任务数量和任务重度
BENCHMARK(case3_submitTasks)
    ->Args({50, 10'000})
    ->Args({50, 50'000})
    ->Args({200, 10'000})
    ->Args({200, 50'000})
    ->Unit(benchmark::kMillisecond);

BENCHMARK(case3_submitTask)
    ->Args({50, 10'000})
    ->Args({50, 50'000})
    ->Args({200, 10'000})
    ->Args({200, 50'000})
    ->Unit(benchmark::kMillisecond);