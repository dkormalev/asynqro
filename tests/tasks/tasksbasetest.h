#ifndef TASKSBASETEST_H
#define TASKSBASETEST_H

#include "../future/futurebasetest.h"

using namespace asynqro::tasks;

template <typename T>
using TestPromise = Promise<T, std::string>;
template <typename T>
using TestFuture = Future<T, std::string>;
template <typename T>
using CancelableTestFuture = CancelableFuture<T, std::string>;
using WithTestFailure = WithFailure<std::string>;

inline std::thread::id currentThread()
{
    return std::this_thread::get_id();
}
template <typename T>
using TasksTestResult = std::pair<std::thread::id, T>;
template <typename T>
TasksTestResult<T> pairedResult(const T &v)
{
    return std::make_pair(currentThread(), v);
}

class TasksBaseTest : public FutureBaseTest
{
protected:
    void TearDown() override
    {
        auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10000);
        while (std::chrono::high_resolution_clock::now() < timeout && TasksDispatcher::instance()->instantUsage() != 0)
            ;
        EXPECT_EQ(0, TasksDispatcher::instance()->instantUsage());
        FutureBaseTest::TearDown();
    }
};

#endif // TASKSBASETEST_H
