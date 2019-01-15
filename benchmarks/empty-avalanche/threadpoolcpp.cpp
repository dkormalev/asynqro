#include "thread_pool.hpp"

#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

using namespace tp;

#ifndef JOBS_COUNT
#    define JOBS_COUNT 1000000
#endif

int main(int, const char *[])
{
    std::cout << "Benchmark job avalanche (empty): " << JOBS_COUNT << std::endl;
    {
        ThreadPoolOptions options;
        uint32_t min_size = JOBS_COUNT / options.threadCount();
        if (min_size > 500) {
            min_size--;
            min_size |= min_size >> 1;
            min_size |= min_size >> 2;
            min_size |= min_size >> 4;
            min_size |= min_size >> 8;
            min_size |= min_size >> 16;
            min_size++;
            min_size <<= 1;
            options.setQueueSize(min_size);
        }
        ThreadPool thread_pool(options);
        std::cout << "***thread pool cpp***" << std::endl;
        long long *finished = new long long[JOBS_COUNT];
        memset(finished, 0, sizeof(long long) * JOBS_COUNT);
        long long begin = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (int i = 0; i < JOBS_COUNT; ++i) {
            thread_pool.post([resultPlace = &finished[i]]() {
                *resultPlace = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            });
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
