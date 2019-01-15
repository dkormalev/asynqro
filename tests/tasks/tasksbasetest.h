#ifndef TASKSBASETEST_H
#define TASKSBASETEST_H

#include "../future/futurebasetest.h"

#include <QPair>

using namespace asynqro::tasks;

inline quint64 currentThread()
{
    return reinterpret_cast<quint64>(QThread::currentThread());
}
template <typename T>
using TasksTestResult = QPair<quint64, T>;
template <typename T>
TasksTestResult<T> pairedResult(const T &v)
{
    return qMakePair(currentThread(), v);
}

class TasksBaseTest : public FutureBaseTest
{
protected:
    void TearDown() override
    {
        QTime timer;
        timer.start();
        while (timer.elapsed() < 10000 && TasksDispatcher::instance()->instantUsage() != 0)
            ;
        EXPECT_EQ(0, TasksDispatcher::instance()->instantUsage());
        FutureBaseTest::TearDown();
    }
};

#endif // TASKSBASETEST_H
