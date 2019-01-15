#include <QtConcurrentRun>

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>

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
    int id = 0;

    mutable volatile int counter;
    long long int begin_count;
    std::promise<void> *waiter;

    RepostJob(std::promise<void> *waiter, int id) : id(id), counter(0), waiter(waiter)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    void operator()() const noexcept
    {
        if (++counter < JOBS_COUNT) {
            QtConcurrent::run(*this);
        } else {
            long long int end_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::cout << "reposted " << counter << " in " << (double)(end_count - begin_count) / (double)1000000
                      << " ms" << std::endl;
            waiter->set_value();
        }
    }
};

int main()
{
    std::cout << "Benchmark job repost (empty): " << CONCURRENCY << "/" << JOBS_COUNT << std::endl;
    {
#if defined(QTC_MAX_THREADS) && QTC_MAX_THREADS > 0
        QThreadPool::globalInstance()->setMaxThreadCount(QTC_MAX_THREADS);
#endif
        std::cout << "***Qt Concurrent***" << std::endl;

        std::promise<void> waiters[CONCURRENCY];
        for (auto &waiter : waiters)
            QtConcurrent::run(RepostJob(&waiter, 1));

        for (auto &waiter : waiters)
            waiter.get_future().wait();
    }

    return 0;
}
