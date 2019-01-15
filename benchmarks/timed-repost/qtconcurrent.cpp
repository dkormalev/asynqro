#include <QtConcurrentRun>

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>

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
    int id = 0;

    mutable volatile int counter;
    long long int begin_count;
    mutable long long int offset;
    std::promise<void> *waiter;

    RepostJob(std::promise<void> *waiter, int id) : id(id), counter(0), waiter(waiter)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        offset = 0;
    }

    void operator()() const noexcept
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
            QtConcurrent::run(*this);
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

int main()
{
    std::cout << "Benchmark job repost (timed): " << CONCURRENCY << "/" << JOBS_COUNT << "/" << JOB_LENGTH << std::endl;
    {
        USEFUL = 0;
#if defined(QTC_MAX_THREADS) && QTC_MAX_THREADS > 0
        QThreadPool::globalInstance()->setMaxThreadCount(QTC_MAX_THREADS);
#endif
        std::cout << "***Qt Concurrent***" << std::endl;

        std::promise<void> waiters[CONCURRENCY];
        long long totalBegin = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (auto &waiter : waiters)
            QtConcurrent::run(RepostJob(&waiter, 1));

        for (auto &waiter : waiters)
            waiter.get_future().wait();

        long long totalEnd = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        long long total = (totalEnd - totalBegin);
        double adjustedUseful = (double)USEFUL / (double)CONCURRENCY;
        adjustedUseful *= std::max((double)CONCURRENCY / (double)CORES, 1.0);
        std::cout << "total " << (double)total / (double)1000000 << " ms; "
                  << "useful " << ((double)USEFUL / (double)1000000) << " ms; "
                  << "adjusted useful " << (adjustedUseful / (double)1000000) << " ms; "
                  << "overhead " << ((double)(total - adjustedUseful) / (double)1000000) << " ms" << std::endl;
    }

    return 0;
}
