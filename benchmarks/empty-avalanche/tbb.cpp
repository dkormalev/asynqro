#include <chrono>
#include <future>
#include <iostream>
#include <tbb/tbb.h>
#include <thread>
#include <vector>

#ifndef JOBS_COUNT
#    define JOBS_COUNT 1000000
#endif

struct AvalancheJob : public tbb::task
{
    long long *resultPlace;
    AvalancheJob(long long *resultPlace) : resultPlace(resultPlace) {}
    task *execute() override
    {
        *resultPlace = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        return NULL;
    }
};

int main(int, const char *[])
{
    size_t workers_count = std::thread::hardware_concurrency();
    if (0 == workers_count)
        workers_count = CORES;
    tbb::global_control c(tbb::global_control::max_allowed_parallelism, workers_count);
    std::cout << "Benchmark job avalanche (empty): " << JOBS_COUNT << std::endl;
    {
        std::cout << "***Intel TBB thread pool***" << std::endl;
        long long *finished = new long long[JOBS_COUNT];
        memset(finished, 0, sizeof(long long) * JOBS_COUNT);
        long long begin = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (int i = 0; i < JOBS_COUNT; ++i) {
            AvalancheJob &a = *new (tbb::task::allocate_root()) AvalancheJob(&finished[i]);
            tbb::task::enqueue(a);
        }

        std::this_thread::sleep_for(std::chrono::microseconds{JOBS_COUNT * 10});
        bool done = false;
        long long max = 0;
        while (!done) {
            std::this_thread::sleep_for(std::chrono::microseconds{JOBS_COUNT});
            done = true;
            max = 0;
            for (int i = 0; i < JOBS_COUNT; ++i) {
                if (!finished[i]) {
                    done = false;
                    break;
                }
                if (finished[i] > max)
                    max = finished[i];
            }
        }
        std::cout << "processed " << JOBS_COUNT << " in " << (double)(max - begin) / (double)1000000 << " ms"
                  << std::endl;
    }

    return 0;
}
