#include "tasksbasetest.h"

#include <QSet>
#include <QVector>

class TasksClusteredRunTest : public TasksBaseTest
{};

TEST_F(TasksClusteredRunTest, clusteredRun)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    SpinLock initialDataLock;
    QVector<int> initialData;
    QVector<int> input;
    int capacity = TasksDispatcher::instance()->subPoolCapacity(TaskType::Intensive);
    int n = capacity * 100;
    int minClusterSize = 5;
    for (int i = 0; i < n; ++i)
        input << i;
    Future<QVector<int>> future = clusteredRun(input,
                                               [&ready, &runCounter, &initialDataLock, &initialData](int x) {
                                                   if (!ready) {
                                                       initialDataLock.lock();
                                                       initialData << x;
                                                       initialDataLock.unlock();
                                                   }
                                                   ++runCounter;
                                                   while (!ready)
                                                       ;
                                                   return x * 2;
                                               },
                                               minClusterSize, TaskType::Intensive);
    QTime timeout;
    timeout.start();
    while (runCounter < capacity && timeout.elapsed() < 1000)
        ;
    QThread::msleep(25);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    auto result = future.result();
    ASSERT_EQ(n, result.count());
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, result[i]);

    std::sort(initialData.begin(), initialData.end());
    EXPECT_EQ(capacity, initialData.count());
    for (int i = 1; i < initialData.count(); ++i)
        EXPECT_LE(minClusterSize, initialData[i] - initialData[i - 1]);
}

TEST_F(TasksClusteredRunTest, emptyClusteredRun)
{
    Future<QVector<int>> future = clusteredRun(QVector<int>(), [](int x) { return x * 2; });
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    EXPECT_EQ(0, result.count());
}

TEST_F(TasksClusteredRunTest, clusteredRunWithExtraBigCluster)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    SpinLock initialDataLock;
    QVector<int> initialData;
    QVector<int> input;

    int capacity = 4;
    TasksDispatcher::instance()->addCustomTag(42, capacity);
    int n = capacity * 100;
    int realClustersCount = 2;
    int minClusterSize = n / realClustersCount;
    for (int i = 0; i < n; ++i)
        input << i;
    Future<QVector<int>> future = clusteredRun(input,
                                               [&ready, &runCounter, &initialDataLock, &initialData](int x) {
                                                   if (!ready) {
                                                       initialDataLock.lock();
                                                       initialData << x;
                                                       initialDataLock.unlock();
                                                   }
                                                   ++runCounter;
                                                   while (!ready)
                                                       ;
                                                   return x * 2;
                                               },
                                               minClusterSize, TaskType::Custom, 42);
    QTime timeout;
    timeout.start();
    while (runCounter < realClustersCount && timeout.elapsed() < 1000)
        ;
    QThread::msleep(25);
    EXPECT_EQ(realClustersCount, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    auto result = future.result();
    ASSERT_EQ(n, result.count());
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, result[i]);

    std::sort(initialData.begin(), initialData.end());
    EXPECT_EQ(realClustersCount, initialData.count());
    for (int i = 1; i < initialData.count(); ++i)
        EXPECT_EQ(minClusterSize, initialData[i] - initialData[i - 1]);
}

TEST_F(TasksClusteredRunTest, clusteredRunWithFailure)
{
    std::atomic_bool ready{false};
    QVector<int> input;
    int n = 20;
    for (int i = 0; i < n; ++i)
        input << i;
    Future<QVector<int>> future = clusteredRun(input,
                                               [&ready](int x) -> int {
                                                   while (!ready)
                                                       ;
                                                   if (x == 3)
                                                       return WithFailure("failed");
                                                   return x * 2;
                                               },
                                               5);
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason().toString());
    EXPECT_EQ(0, future.result().count());
}

TEST_F(TasksClusteredRunTest, clusteredRunWithFailureInLastCluster)
{
    std::atomic_bool ready{false};
    QVector<int> input;
    int n = 20;
    for (int i = 0; i < n; ++i)
        input << i;
    Future<QVector<int>> future = clusteredRun(input,
                                               [&ready](int x) -> int {
                                                   while (!ready)
                                                       ;
                                                   if (x == 19)
                                                       return WithFailure("failed");
                                                   return x * 2;
                                               },
                                               5);
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason().toString());
    EXPECT_EQ(0, future.result().count());
}

TEST_F(TasksClusteredRunTest, clusteredRunWithNegativeClusterSize)
{
    std::atomic_bool ready{false};
    QVector<int> input;
    int n = 20;
    for (int i = 0; i < n; ++i)
        input << i;
    Future<QVector<int>> future = clusteredRun(input,
                                               [&ready](int x) -> int {
                                                   while (!ready)
                                                       ;
                                                   if (x == 3)
                                                       return WithFailure("failed");
                                                   return x * 2;
                                               },
                                               -5);
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason().toString());
    EXPECT_EQ(0, future.result().count());
}
