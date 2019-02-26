#include "tasksbasetest.h"

#include <thread>

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
    ASSERT_TRUE(firstResult.isCompleted());
    ASSERT_TRUE(secondResult.isCompleted());
    EXPECT_NE(currentThread(), firstResult.result().first);
    EXPECT_NE(currentThread(), secondResult.result().first);
    EXPECT_EQ(secondResult.result().first, firstResult.result().first);
}

TEST_F(TasksThreadBoundTest, threadBindingToDifferentKeys)
{
    auto task = []() { return pairedResult(1); };

    std::vector<TestFuture<TasksTestResult<int>>> firstResults;
    std::vector<TestFuture<TasksTestResult<int>>> secondResults;
    int n = TasksDispatcher::instance()->capacity() * 2;
    for (int i = 0; i < n; ++i) {
        firstResults.push_back(run(task, TaskType::ThreadBound, 1));
        secondResults.push_back(run(task, TaskType::ThreadBound, 2));
    }

    std::thread::id firstThread = std::this_thread::get_id();
    std::thread::id secondThread = std::this_thread::get_id();
    for (size_t i = 0; i < firstResults.size(); ++i) {
        auto r = firstResults[i];
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted() && r.isSucceeded()) << i << "; " << r.isCompleted() << "; " << r.isSucceeded();
        if (firstThread == std::this_thread::get_id())
            firstThread = r.result().first;
        EXPECT_NE(currentThread(), r.result().first);
        EXPECT_EQ(firstThread, r.result().first);
    }
    for (size_t i = 0; i < secondResults.size(); ++i) {
        auto r = secondResults[i];
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted() && r.isSucceeded()) << i << "; " << r.isCompleted() << "; " << r.isSucceeded();
        if (secondThread == std::this_thread::get_id())
            secondThread = r.result().first;
        EXPECT_NE(currentThread(), r.result().first);
        EXPECT_EQ(secondThread, r.result().first);
    }
    EXPECT_NE(firstThread, secondThread);
}

TEST_F(TasksThreadBoundTest, threadBindingToLotsOfKeys)
{
    auto task = []() { return pairedResult(1); };

    std::vector<TestFuture<TasksTestResult<int>>> results;
    int n = TasksDispatcher::instance()->subPoolCapacity(TaskType::ThreadBound) * 10;
    for (int i = 1; i <= n; ++i) {
        for (int j = 0; j < 10; ++j) {
            results.push_back(run(task, TaskType::ThreadBound, 2 * i));
            results.push_back(run(task, TaskType::ThreadBound, 2 * i + 1));
        }
    }

    std::set<std::thread::id> threads;
    for (size_t i = 0; i < results.size(); ++i) {
        auto r = results[i];
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted() && r.isSucceeded()) << i << "; " << r.isCompleted() << "; " << r.isSucceeded();
        threads.insert(r.result().first);
    }
    EXPECT_GE(TasksDispatcher::instance()->subPoolCapacity(TaskType::ThreadBound), threads.size());
}

TEST_F(TasksThreadBoundTest, threadBindingAmongNormalTasks)
{
    auto task = []() { return pairedResult(1); };

    std::vector<TestFuture<TasksTestResult<int>>> boundResults;
    std::vector<TestFuture<TasksTestResult<int>>> otherResults;
    int n = TasksDispatcher::instance()->capacity() * 10;
    for (int i = 0; i < n; ++i) {
        if (i % 5)
            otherResults.push_back(run(task, TaskType::Intensive));
        else
            boundResults.push_back(run(task, TaskType::ThreadBound, 1));
    }

    EXPECT_NE(0, boundResults.size());
    EXPECT_NE(0, otherResults.size());
    std::thread::id boundThread = std::this_thread::get_id();
    for (size_t i = 0; i < boundResults.size(); ++i) {
        auto r = boundResults[i];
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted() && r.isSucceeded()) << i << "; " << r.isCompleted() << "; " << r.isSucceeded();
        if (boundThread == std::this_thread::get_id())
            boundThread = r.result().first;
        EXPECT_NE(currentThread(), r.result().first) << i;
        EXPECT_EQ(boundThread, r.result().first) << i;
    }
    for (size_t i = 0; i < otherResults.size(); ++i) {
        auto r = otherResults[i];
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted() && r.isSucceeded()) << i << "; " << r.isCompleted() << "; " << r.isSucceeded();
    }
}

TEST_F(TasksThreadBoundTest, threadBindingToDifferentKeysAmongOtherTasks)
{
    auto task = []() { return pairedResult(1); };

    std::vector<TestFuture<TasksTestResult<int>>> firstResults;
    std::vector<TestFuture<TasksTestResult<int>>> secondResults;
    std::vector<TestFuture<TasksTestResult<int>>> otherResults;
    int n = TasksDispatcher::instance()->capacity() * 20;
    for (int i = 0; i < n; ++i) {
        if (i % 5)
            otherResults.push_back(run(task, TaskType::Custom, 0));
        else if (i % 10)
            firstResults.push_back(run(task, TaskType::ThreadBound, 1));
        else
            secondResults.push_back(run(task, TaskType::ThreadBound, 2));
    }

    EXPECT_NE(0, firstResults.size());
    EXPECT_NE(0, secondResults.size());
    EXPECT_NE(0, otherResults.size());
    std::thread::id firstThread = std::this_thread::get_id();
    std::thread::id secondThread = std::this_thread::get_id();
    for (size_t i = 0; i < firstResults.size(); ++i) {
        auto r = firstResults[i];
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted() && r.isSucceeded()) << i << "; " << r.isCompleted() << "; " << r.isSucceeded();
        if (firstThread == std::this_thread::get_id())
            firstThread = r.result().first;
        EXPECT_NE(currentThread(), r.result().first);
        EXPECT_EQ(firstThread, r.result().first);
    }
    for (size_t i = 0; i < secondResults.size(); ++i) {
        auto r = secondResults[i];
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted() && r.isSucceeded()) << i << "; " << r.isCompleted() << "; " << r.isSucceeded();
        if (secondThread == std::this_thread::get_id())
            secondThread = r.result().first;
        EXPECT_NE(currentThread(), r.result().first);
        EXPECT_EQ(secondThread, r.result().first);
    }
    EXPECT_NE(firstThread, secondThread);
    for (size_t i = 0; i < otherResults.size(); ++i) {
        auto r = otherResults[i];
        r.wait(10000);
        ASSERT_TRUE(r.isCompleted() && r.isSucceeded()) << i << "; " << r.isCompleted() << "; " << r.isSucceeded();
    }
}
