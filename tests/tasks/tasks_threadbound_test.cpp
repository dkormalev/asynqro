#include "tasksbasetest.h"

#include <QSet>
#include <QVector>

class TasksThreadBoundTest : public TasksBaseTest
{};

TEST_F(TasksThreadBoundTest, threadBinding)
{
    std::atomic_bool firstCanStart{false};
    std::atomic_bool secondStarted{false};

    auto first = [&firstCanStart]() {
        while (!firstCanStart)
            ;
        return pairedResult(1);
    };

    auto second = [&secondStarted]() {
        secondStarted = true;
        return pairedResult(1);
    };

    auto firstResult = run(first, TaskType::ThreadBound, 1);
    auto secondResult = run(second, TaskType::ThreadBound, 1);
    EXPECT_FALSE(firstResult.isCompleted());
    EXPECT_FALSE(secondResult.isCompleted());
    EXPECT_FALSE(secondStarted);
    firstCanStart = true;
    firstResult.wait(10000);
    secondResult.wait(10000);
    EXPECT_TRUE(secondStarted);
    EXPECT_NE(currentThread(), firstResult.result().first);
    EXPECT_NE(currentThread(), secondResult.result().first);
    EXPECT_EQ(secondResult.result().first, firstResult.result().first);
}

TEST_F(TasksThreadBoundTest, threadBindingToDifferentKeys)
{
    auto task = []() { return pairedResult(1); };

    QVector<Future<TasksTestResult<int>>> firstResults;
    QVector<Future<TasksTestResult<int>>> secondResults;
    int n = TasksDispatcher::instance()->capacity() * 2;
    for (int i = 0; i < n; ++i) {
        firstResults << run(task, TaskType::ThreadBound, 1);
        secondResults << run(task, TaskType::ThreadBound, 2);
    }

    unsigned long long firstThread = 0ull;
    unsigned long long secondThread = 0ull;
    for (const auto &r : firstResults) {
        r.wait(10000);
        if (!firstThread)
            firstThread = r.result().first;
        EXPECT_NE(currentThread(), r.result().first);
        EXPECT_EQ(firstThread, r.result().first);
    }
    for (const auto &r : secondResults) {
        r.wait(10000);
        if (!secondThread)
            secondThread = r.result().first;
        EXPECT_NE(currentThread(), r.result().first);
        EXPECT_EQ(secondThread, r.result().first);
    }
    EXPECT_NE(firstThread, secondThread);
}

//TEST_F(TasksThreadBoundTest, threadBindingToLotsOfKeys)
//{
//    auto task = []() { return pairedResult(1); };

//    QVector<Future<TasksTestResult<int>>> firstResults;
//    QVector<Future<TasksTestResult<int>>> secondResults;
//    int n = TasksDispatcher::instance()->capacity() * 2;
//    for (int i = 1; i <= n; ++i) {
//        firstResults << run(task, TaskType::ThreadBound, 2 * i);
//        secondResults << run(task, TaskType::ThreadBound, 2 * i + 1);
//    }

//    QSet<unsigned long long> threads;
//    for (const auto &r : firstResults) {
//        r.wait(10000);
//        threads << r.result().first;
//    }
//    for (const auto &r : secondResults) {
//        r.wait(10000);
//        threads << r.result().first;
//    }
//    EXPECT_GE(TasksDispatcher::instance()->subPoolCapacity(TaskType::ThreadBound), threads.count());
//}

TEST_F(TasksThreadBoundTest, threadBindingAmongNormalTasks)
{
    auto task = []() { return pairedResult(1); };

    QVector<Future<TasksTestResult<int>>> boundResults;
    QVector<Future<TasksTestResult<int>>> otherResults;
    int n = TasksDispatcher::instance()->capacity() * 10;
    for (int i = 0; i < n; ++i) {
        if (i % 5)
            otherResults << run(task, TaskType::Intensive);
        else
            boundResults << run(task, TaskType::ThreadBound, 1);
    }

    EXPECT_NE(0, boundResults.count());
    EXPECT_NE(0, otherResults.count());
    unsigned long long boundThread = 0ull;
    for (int i = 0; i < boundResults.count(); ++i) {
        auto r = boundResults[i];
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted() && r.isSucceeded()) << i << "; " << r.isCompleted() << "; " << r.isSucceeded();
        if (!boundThread)
            boundThread = r.result().first;
        EXPECT_NE(currentThread(), r.result().first) << i;
        EXPECT_EQ(boundThread, r.result().first) << i;
    }
    for (int i = 0; i < otherResults.count(); ++i) {
        auto r = otherResults[i];
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted() && r.isSucceeded()) << i << "; " << r.isCompleted() << "; " << r.isSucceeded();
    }
}

TEST_F(TasksThreadBoundTest, threadBindingToDifferentKeysAmongOtherTasks)
{
    auto task = []() { return pairedResult(1); };

    QVector<Future<TasksTestResult<int>>> firstResults;
    QVector<Future<TasksTestResult<int>>> secondResults;
    QVector<Future<TasksTestResult<int>>> otherResults;
    int n = TasksDispatcher::instance()->capacity() * 20;
    for (int i = 0; i < n; ++i) {
        if (i % 5)
            otherResults << run(task, TaskType::Custom, 0);
        else if (i % 10)
            firstResults << run(task, TaskType::ThreadBound, 1);
        else
            secondResults << run(task, TaskType::ThreadBound, 2);
    }

    EXPECT_NE(0, firstResults.count());
    EXPECT_NE(0, secondResults.count());
    EXPECT_NE(0, otherResults.count());
    unsigned long long firstThread = 0ull;
    unsigned long long secondThread = 0ull;
    for (const auto &r : firstResults) {
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted());
        ASSERT_TRUE(r.isSucceeded());
        if (!firstThread)
            firstThread = r.result().first;
        EXPECT_NE(currentThread(), r.result().first);
        EXPECT_EQ(firstThread, r.result().first);
    }
    for (const auto &r : secondResults) {
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted());
        ASSERT_TRUE(r.isSucceeded());
        if (!secondThread)
            secondThread = r.result().first;
        EXPECT_NE(currentThread(), r.result().first);
        EXPECT_EQ(secondThread, r.result().first);
    }
    EXPECT_NE(firstThread, secondThread);
    for (const auto &r : otherResults) {
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted());
        ASSERT_TRUE(r.isSucceeded());
    }
}
