#include <tbb/tbb.h>

#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#ifndef JOBS_COUNT
#    define JOBS_COUNT 1000000
#endif

struct AvalancheJob : public tbb::task
{
    long long* resultPlace;
    long long* spentPlace;
    AvalancheJob(long long* resultPlace, long long* spentPlace) : resultPlace(resultPlace), spentPlace(spentPlace) {}
    task* execute() override
    {
        long long innerBegin = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        long long newId = 0;
        while (std::chrono::high_resolution_clock::now().time_since_epoch().count() - innerBegin < JOB_LENGTH) {
            for (int i = 1; i < innerBegin % 100; ++i) {
                newId += (innerBegin / i);
                newId %= innerBegin;
            }
        }
        long long innerFinish = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        *spentPlace = (innerFinish - innerBegin);
        *resultPlace = innerFinish;
        return NULL;
    }
};

int main(int, const char *[])
{
    size_t workers_count = std::thread::hardware_concurrency();
    if (0 == workers_count)
        workers_count = CORES;
    tbb::global_control c(tbb::global_control::max_allowed_parallelism, workers_count);
    std::cout << "Benchmark job avalanche (timed): " << JOBS_COUNT << "/" << JOB_LENGTH << std::endl;
    {
        std::cout << "***Intel TBB thread pool***" << std::endl;
        long long *finished = new long long[JOBS_COUNT];
        long long *spent = new long long[JOBS_COUNT];
        memset(finished, 0, sizeof(long long) * JOBS_COUNT);
        memset(spent, 0, sizeof(long long) * JOBS_COUNT);
        long long begin = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (int i = 0; i < JOBS_COUNT; ++i) {
            AvalancheJob& a = *new(tbb::task::allocate_root()) AvalancheJob(&finished[i], &spent[i]);
            tbb::task::enqueue(a);
        }

        bool done = false;
        long long max = 0;
        long long totalSpent = 0;
        std::this_thread::sleep_for(std::chrono::microseconds{JOBS_COUNT * 10}
                                    + std::chrono::nanoseconds{JOBS_COUNT * JOB_LENGTH});
        std::cout << "***Intel TBB thread pool***" << std::endl;
        while (!done) {
            std::this_thread::sleep_for(std::chrono::microseconds{JOBS_COUNT});
            done = true;
            max = 0;
            totalSpent = 0;
            for (int i = 0; i < JOBS_COUNT; ++i) {
                if (!finished[i] || !spent[i]) {
                    done = false;
                    break;
                }
                if (finished[i] > max)
                    max = finished[i];
                totalSpent += spent[i];
            }
        }
        long long total = max - begin;
        double adjustedUseful = (double)totalSpent / (double)CORES;
        std::cout << "total " << (double)total / (double)1000000 << " ms; "
                  << "useful " << ((double)totalSpent / (double)1000000) << " ms; "
                  << "adjusted useful " << (adjustedUseful / (double)1000000) << " ms; "
                  << "overhead " << ((double)(total - adjustedUseful) / (double)1000000) << " ms" << std::endl;
    }

    return 0;
}
