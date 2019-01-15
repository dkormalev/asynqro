#include "tasksbasetest.h"

#include <QSet>
#include <QVector>

class TasksSequenceRunTest : public TasksBaseTest
{};

TEST_F(TasksSequenceRunTest, intensiveSequenceRun)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    QVector<int> input;
    int capacity = TasksDispatcher::instance()->subPoolCapacity(TaskType::Intensive);
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input << i;
    Future<QVector<int>> future = run(input,
                                      [&ready, &runCounter](int x) {
                                          ++runCounter;
                                          while (!ready)
                                              ;
                                          return x * 2;
                                      },
                                      TaskType::Intensive);
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
}

TEST_F(TasksSequenceRunTest, sequenceRun)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    QVector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input << i;
    Future<QVector<int>> future = run(input,
                                      [&ready, &runCounter](int x) {
                                          ++runCounter;
                                          while (!ready)
                                              ;
                                          return x * 2;
                                      },
                                      TaskType::Custom, 0);
    QTime timeout;
    timeout.start();
    while (runCounter < capacity && timeout.elapsed() < 1000)
        ;
    QThread::msleep(25);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    ASSERT_EQ(n, result.count());
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, result[i]);
}

TEST_F(TasksSequenceRunTest, deferredSequenceRun)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    QVector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input << i;
    Future<QVector<int>> future = run(input,
                                      [&ready, &runCounter](int x) {
                                          ++runCounter;
                                          while (!ready)
                                              ;
                                          return Future<int>::successful(x * 2);
                                      },
                                      TaskType::Custom, 0);
    QTime timeout;
    timeout.start();
    while (runCounter < capacity && timeout.elapsed() < 1000)
        ;
    QThread::msleep(25);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    ASSERT_EQ(n, result.count());
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, result[i]);
}

TEST_F(TasksSequenceRunTest, voidSequenceRun)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    QVector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input << i;
    Future<bool> future = run(input,
                              [&ready, &runCounter](int) {
                                  ++runCounter;
                                  while (!ready)
                                      ;
                              },
                              TaskType::Custom, 0);
    QTime timeout;
    timeout.start();
    while (runCounter < capacity && timeout.elapsed() < 1000)
        ;
    QThread::msleep(25);
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
    QVector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input << (n - i);
    Future<QVector<long long>> future = run(input,
                                            [n, &ready, &runCounter](long long index, int x) {
                                                ++runCounter;
                                                while (!ready)
                                                    ;
                                                return ((n - index) == x) ? index * 2 : -42;
                                            },
                                            TaskType::Custom, 0);
    QTime timeout;
    timeout.start();
    while (runCounter < capacity && timeout.elapsed() < 1000)
        ;
    QThread::msleep(25);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    ASSERT_EQ(n, result.count());
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, result[i]);
}

TEST_F(TasksSequenceRunTest, deferredSequenceRunWithIndices)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    QVector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input << (n - i);
    Future<QVector<long long>> future = run(input,
                                            [n, &ready, &runCounter](long long index, int x) {
                                                ++runCounter;
                                                while (!ready)
                                                    ;
                                                return Future<long long>::successful(((n - index) == x) ? index * 2
                                                                                                        : -42);
                                            },
                                            TaskType::Custom, 0);
    QTime timeout;
    timeout.start();
    while (runCounter < capacity && timeout.elapsed() < 1000)
        ;
    QThread::msleep(25);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    ASSERT_EQ(n, result.count());
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, result[i]);
}

TEST_F(TasksSequenceRunTest, voidSequenceRunWithIndices)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    QVector<int> input;
    int capacity = TasksDispatcher::instance()->capacity();
    int n = capacity * 2;
    for (int i = 0; i < n; ++i)
        input << (n - i);
    Future<bool> future = run(input,
                              [&ready, &runCounter](long long, int) {
                                  ++runCounter;
                                  while (!ready)
                                      ;
                              },
                              TaskType::Custom, 0);
    QTime timeout;
    timeout.start();
    while (runCounter < capacity && timeout.elapsed() < 1000)
        ;
    QThread::msleep(25);
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
    Future<QVector<int>> future = run(QVector<int>(), [](int x) { return x * 2; });
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    auto result = future.result();
    EXPECT_EQ(0, result.count());
}

TEST_F(TasksSequenceRunTest, sequenceRunWithFailure)
{
    std::atomic_bool ready{false};
    QVector<int> input;
    const int n = 5;
    for (int i = 0; i < n; ++i)
        input << i;
    std::atomic_int doneCount{0};
    Future<QVector<int>> future = run(input, [&ready, &doneCount](int x) -> int {
        while (!ready)
            ;
        ++doneCount;
        if (x == 3)
            return WithFailure("failed");
        return x * 2;
    });
    ready = true;
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason().toString());
    EXPECT_EQ(0, future.result().count());
    QTime timeout;
    timeout.start();
    while (doneCount < n && timeout.elapsed() < 1000)
        ;
    EXPECT_EQ(n, doneCount);
}
