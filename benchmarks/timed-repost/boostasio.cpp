#include "../helpers/asio_thread_pool.hpp"

#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#ifndef CONCURRENCY
#    define CONCURRENCY 4
#endif

#ifndef JOBS_COUNT
#    define JOBS_COUNT 100000
#endif

#ifndef JOB_LENGTH
#    define JOB_LENGTH 100000
#endif

#ifndef CORES
#    define CORES 8
#endif

static std::atomic_llong USEFUL{0};

struct RepostJob
{
    AsioThreadPool *asio_thread_pool;

    volatile size_t counter;
    long long int begin_count;
    long long int offset;
    std::promise<void> *waiter;

    RepostJob(AsioThreadPool *asio_thread_pool, std::promise<void> *waiter)
        : asio_thread_pool(asio_thread_pool), counter(0), waiter(waiter)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        offset = 0;
    }

    void operator()()
    {
        if (++counter < JOBS_COUNT) {
            long long innerBegin = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            long long newId = 0;
            while (std::chrono::high_resolution_clock::now().time_since_epoch().count() - innerBegin < JOB_LENGTH) {
                for (int i = 1; i < innerBegin % 100; ++i) {
                    newId += (innerBegin / i);
                    newId %= innerBegin;
                }
            }
            offset += (std::chrono::high_resolution_clock::now().time_since_epoch().count() - innerBegin);
            asio_thread_pool->post(*this);
        } else {
            long long int end_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::cout << "reposted " << counter << " in " << (double)(end_count - begin_count) / (double)1000000
                      << " ms; "
                      << "overhead = " << (double)(end_count - begin_count - offset) / (double)1000000 << " ms; "
                      << std::endl;
            USEFUL.fetch_add(offset, std::memory_order_relaxed);
            waiter->set_value();
        }
    }
};

int main(int, const char *[])
{
    std::cout << "Benchmark job repost (timed): " << CONCURRENCY << "/" << JOBS_COUNT << "/" << JOB_LENGTH << std::endl;
    {
        USEFUL = 0;
        std::cout << "***asio thread pool***" << std::endl;

        size_t workers_count = std::thread::hardware_concurrency();
        if (0 == workers_count) {
            workers_count = 1;
        }

        AsioThreadPool asio_thread_pool(workers_count);

        std::promise<void> waiters[CONCURRENCY];
        long long totalBegin = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (auto &waiter : waiters)
            asio_thread_pool.post(RepostJob(&asio_thread_pool, &waiter));

        for (auto &waiter : waiters)
            waiter.get_future().wait();

        long long totalEnd = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        long long total = (totalEnd - totalBegin);
        double adjustedUseful = (double)USEFUL / (double)CONCURRENCY;
        adjustedUseful *= std::max((double)CONCURRENCY / 8.0, 1.0);
        std::cout << "total " << (double)total / (double)1000000 << " ms; "
                  << "useful " << ((double)USEFUL / (double)1000000) << " ms; "
                  << "adjusted useful " << (adjustedUseful / (double)1000000) << " ms; "
                  << "overhead " << ((double)(total - adjustedUseful) / (double)1000000) << " ms" << std::endl;
    }

    return 0;
}
