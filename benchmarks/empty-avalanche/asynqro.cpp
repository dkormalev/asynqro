#include "asynqro/asynqro"

#include <QVector>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#ifndef JOBS_COUNT
#    define JOBS_COUNT 10000000
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

int main()
{
    std::cout << "Benchmark job avalanche (empty): " << JOBS_COUNT << std::endl;
    {
#if defined(WITH_PREHEAT) && WITH_PREHEAT
        asynqro::tasks::TasksDispatcher::instance()->preHeatPool();
#endif
        asynqro::tasks::TasksDispatcher::instance()->setIdleLoopsAmount(IDLE_AMOUNT);
        std::cout << "***asynqro***" << std::endl;
        long long *finished = new long long[JOBS_COUNT];
        memset(finished, 0, sizeof(long long) * JOBS_COUNT);
        long long begin = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for (int id = 0; id < JOBS_COUNT; ++id) {
#if defined(WITH_FUTURES) && WITH_FUTURES
            asynqro::tasks::run
#else
            asynqro::tasks::runAndForget
#endif
                (
                    [resultPlace = &finished[id]]() {
                        *resultPlace = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                    },
                    TASK_TYPE, TASK_TAG);
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
