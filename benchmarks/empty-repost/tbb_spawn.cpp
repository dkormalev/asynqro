#include <tbb/tbb.h>

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

struct RepostJob : public tbb::task
{

    volatile size_t counter;
    long long int begin_count;
    std::promise<void> *waiter;

    RepostJob(std::promise<void> *waiter)
        : counter(0), waiter(waiter)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    RepostJob(const RepostJob& other)
    {
        counter = other.counter;
        begin_count = other.begin_count;
        waiter = other.waiter;
    }

    task* execute() override
    {
        if (++counter < JOBS_COUNT) {
            RepostJob& a = *new(allocate_root()) RepostJob(*this);
            tbb::task::spawn(a);
        } else {
            long long int end_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::cout << "reposted " << counter << " in " << (double)(end_count - begin_count) / (double)1000000
                      << " ms" << std::endl;
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
    std::cout << "Benchmark job repost (empty): " << CONCURRENCY << "/" << JOBS_COUNT << std::endl;
    {
        std::cout << "***Intel TBB thread pool***" << std::endl;

        std::promise<void> waiters[CONCURRENCY];

        for (auto &waiter : waiters) {
            RepostJob& a = *new(tbb::task::allocate_root()) RepostJob(&waiter);
            tbb::task::enqueue(a);
        }

        for (auto &waiter : waiters)
            waiter.get_future().wait();
    }

    return 0;
}
