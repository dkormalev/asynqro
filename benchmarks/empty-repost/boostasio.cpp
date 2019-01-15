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
#    define JOBS_COUNT 1000000
#endif

#ifndef CORES
#    define CORES 8
#endif

struct RepostJob
{
    AsioThreadPool *asio_thread_pool;

    volatile size_t counter;
    long long int begin_count;
    std::promise<void> *waiter;

    RepostJob(AsioThreadPool *asio_thread_pool, std::promise<void> *waiter)
        : asio_thread_pool(asio_thread_pool), counter(0), waiter(waiter)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    void operator()()
    {
        if (++counter < JOBS_COUNT) {
            asio_thread_pool->post(*this);
        } else {
            long long int end_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::cout << "reposted " << counter << " in " << (double)(end_count - begin_count) / (double)1000000
                      << " ms" << std::endl;
            waiter->set_value();
        }
    }
};

int main(int, const char *[])
{
    std::cout << "Benchmark job repost (empty): " << CONCURRENCY << "/" << JOBS_COUNT << std::endl;
    {
        std::cout << "***asio thread pool***" << std::endl;

        size_t workers_count = std::thread::hardware_concurrency();
        if (0 == workers_count)
            workers_count = 1;

        AsioThreadPool asio_thread_pool(workers_count);

        std::promise<void> waiters[CONCURRENCY];
        for (auto &waiter : waiters)
            asio_thread_pool.post(RepostJob(&asio_thread_pool, &waiter));

        for (auto &waiter : waiters)
            waiter.get_future().wait();
    }

    return 0;
}
