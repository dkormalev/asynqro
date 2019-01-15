#include <QtConcurrentRun>

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>

#ifndef JOBS_COUNT
#    define JOBS_COUNT 1000000
#endif

#ifndef CORES
#    define CORES 8
#endif

int main()
{
    std::cout << "Benchmark job avalanche (empty): " << JOBS_COUNT << std::endl;
    {
#if defined(QTC_MAX_THREADS) && QTC_MAX_THREADS > 0
        QThreadPool::globalInstance()->setMaxThreadCount(QTC_MAX_THREADS);
#endif
        std::cout << "***Qt Concurrent***" << std::endl;
        long long *finished = new long long[JOBS_COUNT];
        memset(finished, 0, sizeof(long long) * JOBS_COUNT);
        long long begin = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (int i = 0; i < JOBS_COUNT; ++i) {
            QtConcurrent::run([resultPlace = &finished[i]]() {
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
