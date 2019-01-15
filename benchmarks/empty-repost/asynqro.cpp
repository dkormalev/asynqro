#include "asynqro/asynqro"

#include <QVector>

#include <atomic>
#include <chrono>
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

#if defined(TASK_MODE) && TASK_MODE == 0
#    define TASK_TYPE asynqro::tasks::TaskType::Custom
#    define TASK_TAG 0
#elif defined(TASK_MODE) && TASK_MODE == 2
#    define TASK_TYPE asynqro::tasks::TaskType::ThreadBound
#    define TASK_TAG ((id % CORES) + 1)
#else
#    define TASK_TYPE asynqro::tasks::TaskType::Intensive
#    define TASK_TAG 0
#endif

#ifndef IDLE_AMOUNT
#    define IDLE_AMOUNT 10000
#endif

struct RepostJob
{
    int id = 0;

    mutable volatile int counter;
    long long int begin_count;
    asynqro::Promise<bool> waiter;

    RepostJob(const asynqro::Promise<bool> &waiter, int id) : id(id), counter(0), waiter(waiter)
    {
        begin_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }

    void operator()() const noexcept
    {
        if (++counter < JOBS_COUNT) {
#if defined(WITH_FUTURES) && WITH_FUTURES
            asynqro::tasks::run
#else
            asynqro::tasks::runAndForget
#endif
                (*this, TASK_TYPE, TASK_TAG);
        } else {
            long long int end_count = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            std::cout << "reposted " << counter << " in " << (double)(end_count - begin_count) / (double)1000000
                      << " ms" << std::endl;
            waiter.success(true);
        }
    }
};

int main()
{
    std::cout << "Benchmark job repost (empty): " << CONCURRENCY << "/" << JOBS_COUNT << std::endl;
    {
#if defined(WITH_PREHEAT) && WITH_PREHEAT
        asynqro::tasks::TasksDispatcher::instance()->preHeatPool();
#endif
        asynqro::tasks::TasksDispatcher::instance()->setIdleLoopsAmount(IDLE_AMOUNT);
        std::cout << "***asynqro***" << std::endl;

        QVector<asynqro::Future<bool>> waiters;
        waiters.reserve(CONCURRENCY);
        for (int id = 0; id < CONCURRENCY; ++id) {
            asynqro::Promise<bool> p;
#if defined(WITH_FUTURES) && WITH_FUTURES
            asynqro::tasks::run
#else
            asynqro::tasks::runAndForget
#endif
                (RepostJob(p, id), TASK_TYPE, TASK_TAG);
            waiters << p.future();
        }

        asynqro::Future<>::sequence(waiters).wait();
    }
    return 0;
}
