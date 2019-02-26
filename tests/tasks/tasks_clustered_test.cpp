#include "tasksbasetest.h"

#include <chrono>

using namespace std::chrono_literals;

class TasksClusteredRunTest : public TasksBaseTest
{};

TEST_F(TasksClusteredRunTest, clusteredRun)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    std::atomic_int totalCounter{0};
    SpinLock initialDataLock;
    std::vector<int> initialData;
    std::vector<int> input;
    int capacity = TasksDispatcher::instance()->subPoolCapacity(TaskType::Intensive);
    int n = capacity * 100;
    int minClusterSize = 5;
    for (int i = 0; i < n; ++i)
        input.push_back(i);
    TestFuture<std::vector<int>> future = clusteredRun(input,
                                                       [&ready, &runCounter, &totalCounter, &initialDataLock,
                                                        &initialData](int x) {
                                                           if (!ready) {
                                                               initialDataLock.lock();
                                                               initialData.push_back(x);
                                                               initialDataLock.unlock();
                                                           }
                                                           ++runCounter;
                                                           while (!ready)
                                                               ;
                                                           ++totalCounter;
                                                           return x * 2;
                                                       },
                                                       minClusterSize, TaskType::Intensive);
    auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(1000);
    while (runCounter < capacity && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(30000);
    EXPECT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_EQ(n, totalCounter);
    auto result = future.result();
    ASSERT_EQ(n, result.size());
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, result[i]);

    std::sort(initialData.begin(), initialData.end());
    EXPECT_EQ(capacity, initialData.size());
    for (int i = 1; i < initialData.size(); ++i)
        EXPECT_LE(minClusterSize, initialData[i] - initialData[i - 1]);
}

TEST_F(TasksClusteredRunTest, emptyClusteredRun)
{
    TestFuture<std::vector<int>> future = clusteredRun(std::vector<int>(), [](int x) { return x * 2; });
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    EXPECT_EQ(0, result.size());
}

TEST_F(TasksClusteredRunTest, clusteredRunWithExtraBigCluster)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    std::atomic_int totalCounter{0};
    SpinLock initialDataLock;
    std::vector<int> initialData;
    std::vector<int> input;

    int capacity = 4;
    TasksDispatcher::instance()->addCustomTag(42, capacity);
    int n = capacity * 100;
    int realClustersCount = 2;
    int minClusterSize = n / realClustersCount;
    for (int i = 0; i < n; ++i)
        input.push_back(i);
    TestFuture<std::vector<int>> future = clusteredRun(input,
                                                       [&ready, &runCounter, &totalCounter, &initialDataLock,
                                                        &initialData](int x) {
                                                           if (!ready) {
                                                               initialDataLock.lock();
                                                               initialData.push_back(x);
                                                               initialDataLock.unlock();
                                                           }
                                                           ++runCounter;
                                                           while (!ready)
                                                               ;
                                                           ++totalCounter;
                                                           return x * 2;
                                                       },
                                                       minClusterSize, TaskType::Custom, 42);

    auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(1000);
    while (runCounter < realClustersCount && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(realClustersCount, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(30000);
    EXPECT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_EQ(n, totalCounter);
    auto result = future.result();
    ASSERT_EQ(n, result.size());
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, result[i]);

    std::sort(initialData.begin(), initialData.end());
    EXPECT_EQ(realClustersCount, initialData.size());
    for (int i = 1; i < initialData.size(); ++i)
        EXPECT_EQ(minClusterSize, initialData[i] - initialData[i - 1]);
}

TEST_F(TasksClusteredRunTest, clusteredRunWithFailure)
{
    std::atomic_bool ready{false};
    std::vector<int> input;
    int n = 20;
    for (int i = 0; i < n; ++i)
        input.push_back(i);
    TestFuture<std::vector<int>> future = clusteredRun(input,
                                                       [&ready](int x) -> int {
                                                           while (!ready)
                                                               ;
                                                           if (x == 3)
                                                               return WithTestFailure("failed");
                                                           return x * 2;
                                                       },
                                                       5);
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());
    EXPECT_EQ(0, future.result().size());
}

TEST_F(TasksClusteredRunTest, clusteredRunWithFailureInLastCluster)
{
    std::atomic_bool ready{false};
    std::vector<int> input;
    int n = 20;
    for (int i = 0; i < n; ++i)
        input.push_back(i);
    TestFuture<std::vector<int>> future = clusteredRun(input,
                                                       [&ready](int x) -> int {
                                                           while (!ready)
                                                               ;
                                                           if (x == 19)
                                                               return WithTestFailure("failed");
                                                           return x * 2;
                                                       },
                                                       5);
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());
    EXPECT_EQ(0, future.result().size());
}

TEST_F(TasksClusteredRunTest, clusteredRunWithNegativeClusterSize)
{
    std::atomic_bool ready{false};
    std::vector<int> input;
    int n = 20;
    for (int i = 0; i < n; ++i)
        input.push_back(i);
    TestFuture<std::vector<int>> future = clusteredRun(input,
                                                       [&ready](int x) -> int {
                                                           while (!ready)
                                                               ;
                                                           if (x == 3)
                                                               return WithTestFailure("failed");
                                                           return x * 2;
                                                       },
                                                       -5);
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());
    EXPECT_EQ(0, future.result().size());
}
