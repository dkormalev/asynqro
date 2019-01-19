#include "tasksbasetest.h"

#include <QSet>
#include <QVector>

class TasksTest : public TasksBaseTest
{};

TEST_F(TasksTest, capacities)
{
    auto dispatcher = TasksDispatcher::instance();
    EXPECT_GT(dispatcher->capacity(), dispatcher->subPoolCapacity(TaskType::Intensive));
    EXPECT_EQ(dispatcher->subPoolCapacity(TaskType::Custom, 1), dispatcher->subPoolCapacity(TaskType::Intensive));
    EXPECT_GT(dispatcher->capacity(), dispatcher->subPoolCapacity(TaskType::Custom, 1));
    EXPECT_EQ(dispatcher->capacity(), dispatcher->subPoolCapacity(TaskType::Custom));
    EXPECT_EQ(dispatcher->capacity(), dispatcher->subPoolCapacity(TaskType::Custom, 0));
}

TEST_F(TasksTest, changeCapacities)
{
    auto dispatcher = TasksDispatcher::instance();

    qint32 oldCapacity = dispatcher->capacity();
    qint32 newCapacity = oldCapacity + 10;
    dispatcher->setCapacity(newCapacity);
    ASSERT_EQ(newCapacity, dispatcher->capacity());
    dispatcher->setCapacity(oldCapacity);
    ASSERT_EQ(oldCapacity, dispatcher->capacity());

    oldCapacity = dispatcher->subPoolCapacity(TaskType::ThreadBound);
    newCapacity = oldCapacity + 2;
    dispatcher->setBoundCapacity(newCapacity);
    ASSERT_EQ(newCapacity, dispatcher->subPoolCapacity(TaskType::ThreadBound));
    dispatcher->setBoundCapacity(oldCapacity);
    ASSERT_EQ(oldCapacity, dispatcher->subPoolCapacity(TaskType::ThreadBound));

    qint32 customLimit = dispatcher->subPoolCapacity(TaskType::Intensive) * 3;
    EXPECT_NE(customLimit, dispatcher->subPoolCapacity(TaskType::Custom, 42));
    dispatcher->addCustomTag(42, customLimit);
    EXPECT_EQ(customLimit, dispatcher->subPoolCapacity(TaskType::Custom, 42));
    EXPECT_GT(dispatcher->capacity(), dispatcher->subPoolCapacity(TaskType::ThreadBound));

    EXPECT_EQ(dispatcher->capacity(), dispatcher->subPoolCapacity(TaskType::Custom, -5));
    dispatcher->addCustomTag(-5, customLimit);
    EXPECT_EQ(dispatcher->capacity(), dispatcher->subPoolCapacity(TaskType::Custom, -5));
}

TEST_F(TasksTest, changeIdleLoopsAmount)
{
    auto dispatcher = TasksDispatcher::instance();
    qint32 oldIdleLoops = dispatcher->idleLoopsAmount();
    qint32 newIdleLoops = oldIdleLoops + 20000;
    dispatcher->setIdleLoopsAmount(newIdleLoops);
    ASSERT_EQ(newIdleLoops, dispatcher->idleLoopsAmount());
    dispatcher->setIdleLoopsAmount(oldIdleLoops);
    ASSERT_EQ(oldIdleLoops, dispatcher->idleLoopsAmount());
}

TEST_F(TasksTest, singleTask)
{
    TasksTestResult<int> result = run([]() { return pairedResult(42); }).result();
    EXPECT_NE(currentThread(), result.first);
    EXPECT_EQ(42, result.second);
}

TEST_F(TasksTest, singleDeferredTask)
{
    Promise<int> innerPromise;
    Future<int> result = run([innerPromise]() { return innerPromise.future(); });
    EXPECT_FALSE(result.isCompleted());
    innerPromise.success(42);
    result.wait(10000);
    ASSERT_TRUE(result.isCompleted());
    EXPECT_TRUE(result.isSucceeded());
    EXPECT_FALSE(result.isFailed());
    EXPECT_EQ(42, result.result());
}

TEST_F(TasksTest, singleVoidTask)
{
    std::atomic_bool flag{false};
    Future<bool> result = run([&flag]() { flag = true; });
    result.wait(10000);
    ASSERT_TRUE(result.isCompleted());
    EXPECT_TRUE(result.isSucceeded());
    EXPECT_FALSE(result.isFailed());
    EXPECT_EQ(true, result.result());
    EXPECT_EQ(true, flag);
}

TEST_F(TasksTest, taskCancelation)
{
    TasksDispatcher::instance()->addCustomTag(11, 1);
    Promise<int> blockingPromise;
    run([blockingPromise]() { blockingPromise.future().wait(); }, TaskType::Custom, 11);
    std::atomic_bool executed{false};
    CancelableFuture<int> f = run(
        [&executed]() {
            executed = true;
            return 42;
        },
        TaskType::Custom, 11);
    CancelableFuture<int> f2 = run([]() { return 42; }, TaskType::Custom, 11);
    f.cancel();
    blockingPromise.success(1);
    f.wait(10000);
    ASSERT_TRUE(f.isCompleted());
    f2.wait(10000);
    ASSERT_TRUE(f2.isCompleted());
    EXPECT_EQ(42, f2.result());

    EXPECT_FALSE(executed);
    EXPECT_FALSE(f.isSucceeded());
    EXPECT_TRUE(f.isFailed());
    EXPECT_EQ("Canceled", f.failureReason().toString());
}

TEST_F(TasksTest, deferedTaskCancelation)
{
    TasksDispatcher::instance()->addCustomTag(11, 1);
    Promise<int> blockingPromise;
    run([blockingPromise]() { return blockingPromise.future().wait(); }, TaskType::Custom, 11);
    std::atomic_bool executed{false};
    Promise<int> innerPromise;
    CancelableFuture<int> f = run(
        [innerPromise, &executed]() {
            executed = true;
            return innerPromise.future();
        },
        TaskType::Custom, 11);
    CancelableFuture<int> f2 = run([]() { return 42; }, TaskType::Custom, 11);
    f.cancel();
    blockingPromise.success(1);
    f.wait(10000);
    ASSERT_TRUE(f.isCompleted());
    f2.wait(10000);
    ASSERT_TRUE(f2.isCompleted());
    EXPECT_EQ(42, f2.result());

    EXPECT_FALSE(executed);
    EXPECT_FALSE(f.isSucceeded());
    EXPECT_TRUE(f.isFailed());
    EXPECT_EQ("Canceled", f.failureReason().toString());
}

TEST_F(TasksTest, voidTaskCancelation)
{
    TasksDispatcher::instance()->addCustomTag(11, 1);
    Promise<int> blockingPromise;
    run([blockingPromise]() { return blockingPromise.future().wait(); }, TaskType::Custom, 11);
    std::atomic_bool executed{false};
    CancelableFuture<bool> f = run([&executed]() { executed = true; }, TaskType::Custom, 11);
    CancelableFuture<int> f2 = run([]() { return 42; }, TaskType::Custom, 11);
    f.cancel();
    blockingPromise.success(1);
    f.wait(10000);
    ASSERT_TRUE(f.isCompleted());
    f2.wait(10000);
    ASSERT_TRUE(f2.isCompleted());
    EXPECT_EQ(42, f2.result());

    EXPECT_FALSE(executed);
    EXPECT_FALSE(f.isSucceeded());
    EXPECT_TRUE(f.isFailed());
    EXPECT_EQ("Canceled", f.failureReason().toString());
}

TEST_F(TasksTest, taskPriority)
{
    TasksDispatcher::instance()->addCustomTag(11, 1);
    Promise<int> blockingPromise;
    Promise<int> blockingPromise2;
    run([blockingPromise]() { blockingPromise.future().wait(); }, TaskType::Custom, 11);
    run([blockingPromise2]() { blockingPromise2.future().wait(); }, TaskType::Custom, 11);
    auto postponed = run([]() { return 24; }, TaskType::Custom, 11);
    auto emergency = run([]() { return 42; }, TaskType::Custom, 11, TaskPriority::Emergency);

    blockingPromise.success(1);
    emergency.wait(10000);
    ASSERT_TRUE(emergency.isCompleted());
    EXPECT_TRUE(emergency.isSucceeded());
    EXPECT_EQ(42, emergency.result());
    EXPECT_FALSE(postponed.isCompleted());

    blockingPromise2.success(1);
    postponed.wait(10000);
    ASSERT_TRUE(postponed.isCompleted());
    EXPECT_TRUE(postponed.isSucceeded());
    EXPECT_EQ(24, postponed.result());
}

TEST_F(TasksTest, singleDeferredTaskWithFailure)
{
    Promise<int> innerPromise;
    Future<int> result = run([innerPromise]() { return innerPromise.future(); });
    EXPECT_FALSE(result.isCompleted());
    innerPromise.failure("failed");
    result.wait(10000);
    ASSERT_TRUE(result.isCompleted());
    EXPECT_FALSE(result.isSucceeded());
    EXPECT_TRUE(result.isFailed());
    EXPECT_EQ(0, result.result());
    EXPECT_EQ("failed", result.failureReason().toString());
}

TEST_F(TasksTest, multipleTasks)
{
    std::atomic_bool ready{false};
    int n = 5;
    QVector<Future<TasksTestResult<int>>> results;
    for (int i = 0; i < n; ++i) {
        results << run(TaskType::Custom, 0, [&ready, i]() {
            while (!ready)
                ;
            return pairedResult(i * 2);
        });
    }
    for (int i = 0; i < n; ++i)
        EXPECT_FALSE(results[i].isCompleted());
    ready = true;
    QSet<quint64> threads;
    for (int i = n - 1; i >= 0; --i) {
        auto result = results[i].result();
        threads << result.first;
        EXPECT_NE(currentThread(), result.first);
        EXPECT_EQ(i * 2, result.second);
    }
    EXPECT_EQ(n, threads.count());
}

TEST_F(TasksTest, multipleTasksOverCapacity)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    int n = TasksDispatcher::instance()->capacity() * 2;
    QVector<Future<int>> results;
    for (int i = 0; i < n; ++i) {
        results << run(TaskType::Custom, 0, [&ready, &runCounter, i]() {
            ++runCounter;
            while (!ready)
                ;
            return i * 2;
        });
    }
    QTime timeout;
    timeout.start();
    while (runCounter < TasksDispatcher::instance()->capacity() && timeout.elapsed() < 1000)
        ;
    QThread::msleep(25);
    EXPECT_EQ(TasksDispatcher::instance()->capacity(), runCounter);
    for (int i = 0; i < n; ++i)
        EXPECT_FALSE(results[i].isCompleted());
    ready = true;
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, results[i].result());
}

TEST_F(TasksTest, multipleTasksOverChangedCapacity)
{
    qint32 newCapacity = TasksDispatcher::instance()->capacity() + 10;
    TasksDispatcher::instance()->setCapacity(newCapacity);
    ASSERT_EQ(newCapacity, TasksDispatcher::instance()->capacity());
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    int n = TasksDispatcher::instance()->capacity() * 2;
    QVector<Future<int>> results;
    for (int i = 0; i < n; ++i) {
        results << run(TaskType::Custom, 0, [&ready, &runCounter, i]() {
            ++runCounter;
            while (!ready)
                ;
            return i * 2;
        });
    }
    QTime timeout;
    timeout.start();
    while (runCounter < TasksDispatcher::instance()->capacity() && timeout.elapsed() < 1000)
        ;
    QThread::msleep(25);
    EXPECT_EQ(TasksDispatcher::instance()->capacity(), runCounter);
    for (int i = 0; i < n; ++i)
        EXPECT_FALSE(results[i].isCompleted());
    ready = true;
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, results[i].result());
}

TEST_F(TasksTest, multipleIntensiveTasksOverCapacity)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    int capacity = TasksDispatcher::instance()->subPoolCapacity(TaskType::Intensive);
    int n = capacity * 2;
    QVector<Future<int>> results;
    for (int i = 0; i < n; ++i) {
        results << run(
            [&ready, &runCounter, i]() {
                ++runCounter;
                while (!ready)
                    ;
                return i * 2;
            },
            TaskType::Intensive);
    }
    QTime timeout;
    timeout.start();
    while (runCounter < capacity && timeout.elapsed() < 1000)
        ;
    QThread::msleep(25);
    EXPECT_EQ(capacity, runCounter);
    for (int i = 0; i < n; ++i)
        EXPECT_FALSE(results[i].isCompleted());
    ready = true;
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, results[i].result());
}

TEST_F(TasksTest, multipleCustomTasksOverCapacity)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    std::atomic_int otherRunCounter{0};
    int capacity = 2;
    TasksDispatcher::instance()->addCustomTag(42, capacity);
    int otherCapacity = TasksDispatcher::instance()->subPoolCapacity(TaskType::Custom, 24);
    int n = otherCapacity * 2;
    QVector<Future<int>> results;
    QVector<Future<int>> otherResults;
    for (int i = 0; i < n; ++i) {
        results << run(TaskType::Custom, 42, [&ready, &runCounter, i]() {
            ++runCounter;
            while (!ready)
                ;
            return i * 2;
        });
        otherResults << run(TaskType::Custom, 24, [&ready, &otherRunCounter, i]() {
            ++otherRunCounter;
            while (!ready)
                ;
            return i * 3;
        });
    }
    QTime timeout;
    timeout.start();
    while ((runCounter < capacity || otherRunCounter < otherCapacity) && timeout.elapsed() < 5000)
        ;
    QThread::msleep(25);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_EQ(otherCapacity, otherRunCounter);
    for (int i = 0; i < n; ++i) {
        EXPECT_FALSE(results[i].isCompleted());
        EXPECT_FALSE(otherResults[i].isCompleted());
    }
    ready = true;
    for (int i = 0; i < n; ++i) {
        EXPECT_EQ(i * 2, results[i].result());
        EXPECT_EQ(i * 3, otherResults[i].result());
    }
}

TEST_F(TasksTest, multipleTasksWithFailure)
{
    std::atomic_bool ready{false};
    int n = 5;
    QVector<Future<TasksTestResult<int>>> results;
    for (int i = 0; i < n; ++i) {
        results << run([&ready, i]() -> TasksTestResult<int> {
            while (!ready)
                ;
            if (i % 2)
                return WithFailure("failed");
            else
                return pairedResult(i * 2);
        });
    }
    for (int i = 0; i < n; ++i)
        EXPECT_FALSE(results[i].isCompleted());
    ready = true;
    for (int i = 0; i < n; ++i) {
        results[i].wait(10000);
        ASSERT_TRUE(results[i].isCompleted()) << i;
        if (i % 2) {
            ASSERT_TRUE(results[i].isFailed()) << i;
            EXPECT_EQ("failed", results[i].failureReason().toString()) << i;
            EXPECT_EQ(0ull, results[i].result().first) << i;
            EXPECT_EQ(0, results[i].result().second) << i;
        } else {
            ASSERT_TRUE(results[i].isSucceeded()) << i;
            auto result = results[i].result();
            EXPECT_NE(currentThread(), result.first) << i;
            EXPECT_EQ(i * 2, result.second) << i;
        }
    }
}

TEST_F(TasksTest, mappedTaskWithFailure)
{
    std::atomic_bool ready{false};
    Future<int> future = run([&ready]() {
        while (!ready)
            ;
        return 42;
    });
    Future<int> mappedFuture = future.map([](int x) -> int { return WithFailure(x); });
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    mappedFuture.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    ASSERT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ(0, mappedFuture.result());
    EXPECT_EQ(42, mappedFuture.failureReason().toInt());
}
