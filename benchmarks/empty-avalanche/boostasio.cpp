#include "../helpers/asio_thread_pool.hpp"

#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#ifndef JOBS_COUNT
#    define JOBS_COUNT 1000000
#endif

int main(int, const char *[])
{
    std::cout << "Benchmark job avalanche (empty): " << JOBS_COUNT << std::endl;
    {
        size_t workers_count = std::thread::hardware_concurrency();
        if (0 == workers_count)
            workers_count = 1;

        AsioThreadPool asio_thread_pool(workers_count);
        std::cout << "***asio thread pool***" << std::endl;
        long long *finished = new long long[JOBS_COUNT];
        memset(finished, 0, sizeof(long long) * JOBS_COUNT);
        long long begin = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (int i = 0; i < JOBS_COUNT; ++i) {
            asio_thread_pool.post([resultPlace = &finished[i]]() {
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
