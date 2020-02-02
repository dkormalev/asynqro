#include <chrono>
#include <future>
#include <iostream>
#include <tbb/tbb.h>
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

struct RepostJob : public tbb::task
{
    volatile size_t counter;
    long long int begin_count;
    long long int offset;
    std::promise<void> *waiter;

    RepostJob(std::promise<void> *waiter) : counter(0), waiter(waiter)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        offset = 0;
    }

    RepostJob(const RepostJob &other)
    {
        counter = other.counter;
        begin_count = other.begin_count;
        offset = other.offset;
        waiter = other.waiter;
    }

    task *execute() override
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
            RepostJob &a = *new (allocate_root()) RepostJob(*this);
            tbb::task::enqueue(a);
        } else {
            long long int end_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::cout << "reposted " << counter << " in " << (double)(end_count - begin_count) / (double)1000000
                      << " ms; "
                      << "overhead = " << (double)(end_count - begin_count - offset) / (double)1000000 << " ms; "
                      << std::endl;
            USEFUL.fetch_add(offset, std::memory_order_relaxed);
            waiter->set_value();
        }
        return NULL;
    }
};

int main(int, const char *[])
{
    size_t workers_count = std::thread::hardware_concurrency();
    if (0 == workers_count)
        workers_count = CORES;
    tbb::global_control c(tbb::global_control::max_allowed_parallelism, workers_count);
    std::cout << "Benchmark job repost (timed): " << CONCURRENCY << "/" << JOBS_COUNT << "/" << JOB_LENGTH << std::endl;
    {
        std::cout << "***Intel TBB thread pool***" << std::endl;

        std::promise<void> waiters[CONCURRENCY];

        long long totalBegin = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (auto &waiter : waiters) {
            RepostJob &a = *new (tbb::task::allocate_root()) RepostJob(&waiter);
            tbb::task::enqueue(a);
        }

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
