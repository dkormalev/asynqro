#include "tasksbasetest.h"

#include <chrono>

using namespace std::chrono_literals;

class TasksSequenceRunTest : public TasksBaseTest
{};

TEST_F(TasksSequenceRunTest, intensiveSequenceRun)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    std::vector<int> input;
    int capacity = TasksDispatcher::instance()->subPoolCapacity(TaskType::Intensive);
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input.push_back(i);
    TestFuture<std::vector<int>> future = run(input,
                                              [&ready, &runCounter](int x) {
                                                  ++runCounter;
                                                  while (!ready)
                                                      std::this_thread::sleep_for(1ms);
                                                  return x * 2;
                                              },
                                              TaskType::Intensive);

    auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10000);
    while (runCounter < capacity && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    auto result = future.result();
    ASSERT_EQ(n, result.size());
    for (size_t i = 0; i < static_cast<size_t>(n); ++i)
        EXPECT_EQ(i * 2, result[i]);
}

TEST_F(TasksSequenceRunTest, sequenceRun)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    std::vector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input.push_back(i);
    TestFuture<std::vector<int>> future = run(input,
                                              [&ready, &runCounter](int x) {
                                                  ++runCounter;
                                                  while (!ready)
                                                      std::this_thread::sleep_for(1ms);
                                                  return x * 2;
                                              },
                                              TaskType::Custom, 0);

    auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10000);
    while (runCounter < capacity && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    ASSERT_EQ(n, result.size());
    for (size_t i = 0; i < static_cast<size_t>(n); ++i)
        EXPECT_EQ(i * 2, result[i]);
}

TEST_F(TasksSequenceRunTest, deferredSequenceRun)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    std::vector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input.push_back(i);
    TestFuture<std::vector<int>> future = run(input,
                                              [&ready, &runCounter](int x) {
                                                  ++runCounter;
                                                  while (!ready)
                                                      std::this_thread::sleep_for(1ms);
                                                  return TestFuture<int>::successful(x * 2);
                                              },
                                              TaskType::Custom, 0);

    auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10000);
    while (runCounter < capacity && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    ASSERT_EQ(n, result.size());
    for (size_t i = 0; i < static_cast<size_t>(n); ++i)
        EXPECT_EQ(i * 2, result[i]);
}

TEST_F(TasksSequenceRunTest, voidSequenceRun)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    std::vector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input.push_back(i);
    TestFuture<bool> future = run(input,
                                  [&ready, &runCounter](int) {
                                      ++runCounter;
                                      while (!ready)
                                          std::this_thread::sleep_for(1ms);
                                  },
                                  TaskType::Custom, 0);

    auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10000);
    while (runCounter < capacity && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    EXPECT_TRUE(result);
}

TEST_F(TasksSequenceRunTest, sequenceRunWithIndices)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    std::vector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input.push_back(n - i);
    TestFuture<std::vector<long long>> future = run(input,
                                                    [n, &ready, &runCounter](long long index, int x) {
                                                        ++runCounter;
                                                        while (!ready)
                                                            std::this_thread::sleep_for(1ms);
                                                        return ((n - index) == x) ? index * 2 : -42;
                                                    },
                                                    TaskType::Custom, 0);

    auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10000);
    while (runCounter < capacity && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    ASSERT_EQ(n, result.size());
    for (size_t i = 0; i < static_cast<size_t>(n); ++i)
        EXPECT_EQ(i * 2, result[i]);
}

TEST_F(TasksSequenceRunTest, deferredSequenceRunWithIndices)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    std::vector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input.push_back(n - i);
    TestFuture<std::vector<long long>> future = run(input,
                                                    [n, &ready, &runCounter](long long index, int x) {
                                                        ++runCounter;
                                                        while (!ready)
                                                            std::this_thread::sleep_for(1ms);
                                                        return TestFuture<long long>::successful(
                                                            ((n - index) == x) ? index * 2 : -42);
                                                    },
                                                    TaskType::Custom, 0);

    auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10000);
    while (runCounter < capacity && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    ASSERT_EQ(n, result.size());
    for (size_t i = 0; i < static_cast<size_t>(n); ++i)
        EXPECT_EQ(i * 2, result[i]);
}

TEST_F(TasksSequenceRunTest, voidSequenceRunWithIndices)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    std::vector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input.push_back(n - i);
    TestFuture<bool> future = run(input,
                                  [&ready, &runCounter](long long, int) {
                                      ++runCounter;
                                      while (!ready)
                                          std::this_thread::sleep_for(1ms);
                                  },
                                  TaskType::Custom, 0);

    auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10000);
    while (runCounter < capacity && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    EXPECT_TRUE(result);
}

TEST_F(TasksSequenceRunTest, emptySequenceRun)
{
    TestFuture<std::vector<int>> future = run(std::vector<int>(), [](int x) { return x * 2; });
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    EXPECT_EQ(0, result.size());
}

TEST_F(TasksSequenceRunTest, sequenceRunWithFailure)
{
    std::atomic_bool ready{false};
    std::vector<int> input;
    const int n = 5;
    for (int i = 0; i < n; ++i)
        input.push_back(i);
    std::atomic_int doneCount{0};
    TestFuture<std::vector<int>> future = run(input, [&ready, &doneCount](int x) -> int {
        while (!ready)
            ;
        ++doneCount;
        if (x == 3)
            return WithTestFailure("failed");
        return x * 2;
    });
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());
    EXPECT_EQ(0, future.result().size());

    auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(10000);
    while (doneCount < n && std::chrono::high_resolution_clock::now() < timeout)
        ;
    EXPECT_EQ(n, doneCount);
}
