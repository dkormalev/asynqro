#include "tasksbasetest.h"

#include <chrono>

using namespace std::chrono_literals;

class TasksTest : public TasksBaseTest
{};

using TasksDeathTest = TasksTest;

struct ConvertingRunnerInfo
{
    using PlainFailure = int;
    constexpr static bool deferredFailureShouldBeConverted = true;
    template <typename DeferredFailure>
    static PlainFailure toPlainFailure(const DeferredFailure &)
    {
        return 42;
    }
};

namespace asynqro::failure {
template <>
inline int failureFromString<int>(const std::string &s)
{
    return 21;
}
} // namespace asynqro::failure

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

    int32_t oldCapacity = dispatcher->capacity();
    int32_t newCapacity = oldCapacity + 10;
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

    int32_t customLimit = dispatcher->subPoolCapacity(TaskType::Intensive) * 3;
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
    int_fast32_t oldIdleLoops = dispatcher->idleLoopsAmount();
    int_fast32_t newIdleLoops = oldIdleLoops + 20000;
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
    TestPromise<int> innerPromise;
    TestFuture<int> result = run([innerPromise]() { return innerPromise.future(); });
    EXPECT_FALSE(result.isCompleted());
    innerPromise.success(42);
    result.wait(10000);
    ASSERT_TRUE(result.isCompleted());
    EXPECT_TRUE(result.isSucceeded());
    EXPECT_FALSE(result.isFailed());
    EXPECT_EQ(42, result.result());
}

TEST_F(TasksTest, singleDeferredTaskWithCastedFailure)
{
    TestPromise<int> innerPromise;
    Future<int, int> result = run<TaskRunner<ConvertingRunnerInfo>>([innerPromise]() { return innerPromise.future(); });
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
    TestFuture<bool> result = run([&flag]() { flag = true; });
    result.wait(10000);
    ASSERT_TRUE(result.isCompleted());
    EXPECT_TRUE(result.isSucceeded());
    EXPECT_FALSE(result.isFailed());
    EXPECT_EQ(true, result.result());
    EXPECT_EQ(true, flag);
}

TEST_F(TasksTest, singleRunAndForgetTask)
{
    TestPromise<TasksTestResult<int>> p;
    runAndForget([p]() { p.success(pairedResult(42)); });
    auto f = p.future();
    f.wait(10000);
    ASSERT_TRUE(f.isCompleted());
    EXPECT_TRUE(f.isSucceeded());
    EXPECT_FALSE(f.isFailed());
    TasksTestResult<int> result = f.result();
    EXPECT_NE(currentThread(), result.first);
    EXPECT_EQ(42, result.second);
}

TEST_F(TasksTest, taskCancelation)
{
    TasksDispatcher::instance()->addCustomTag(11, 1);
    TestPromise<int> blockingPromise;
    run([blockingPromise]() { blockingPromise.future().wait(); }, TaskType::Custom, 11);
    std::atomic_bool executed{false};
    CancelableTestFuture<int> f = run(
        [&executed]() {
            executed = true;
            return 42;
        },
        TaskType::Custom, 11);
    CancelableTestFuture<int> f2 = run([]() { return 42; }, TaskType::Custom, 11);
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
    EXPECT_EQ("Canceled", f.failureReason());
}

TEST_F(TasksTest, deferedTaskCancelation)
{
    TasksDispatcher::instance()->addCustomTag(11, 1);
    TestPromise<int> blockingPromise;
    run([blockingPromise]() { return blockingPromise.future().wait(); }, TaskType::Custom, 11);
    std::atomic_bool executed{false};
    TestPromise<int> innerPromise;
    CancelableTestFuture<int> f = run(
        [innerPromise, &executed]() {
            executed = true;
            return innerPromise.future();
        },
        TaskType::Custom, 11);
    CancelableTestFuture<int> f2 = run([]() { return 42; }, TaskType::Custom, 11);
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
    EXPECT_EQ("Canceled", f.failureReason());
}

TEST_F(TasksTest, voidTaskCancelation)
{
    TasksDispatcher::instance()->addCustomTag(11, 1);
    TestPromise<int> blockingPromise;
    run([blockingPromise]() { return blockingPromise.future().wait(); }, TaskType::Custom, 11);
    std::atomic_bool executed{false};
    CancelableTestFuture<bool> f = run([&executed]() { executed = true; }, TaskType::Custom, 11);
    CancelableTestFuture<int> f2 = run([]() { return 42; }, TaskType::Custom, 11);
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
    EXPECT_EQ("Canceled", f.failureReason());
}

TEST_F(TasksTest, taskPriority)
{
    TasksDispatcher::instance()->addCustomTag(11, 1);
    TestPromise<int> blockingPromise;
    TestPromise<int> blockingPromise2;
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
    TestPromise<int> innerPromise;
    TestFuture<int> result = run([innerPromise]() { return innerPromise.future(); });
    EXPECT_FALSE(result.isCompleted());
    innerPromise.failure("failed");
    result.wait(10000);
    ASSERT_TRUE(result.isCompleted());
    EXPECT_FALSE(result.isSucceeded());
    EXPECT_TRUE(result.isFailed());
    EXPECT_EQ(0, result.result());
    EXPECT_EQ("failed", result.failureReason());
}

TEST_F(TasksTest, singleDeferredTaskFailureWithCastedFailure)
{
    TestPromise<int> innerPromise;
    Future<int, int> result = run<TaskRunner<ConvertingRunnerInfo>>([innerPromise]() { return innerPromise.future(); });
    EXPECT_FALSE(result.isCompleted());
    innerPromise.failure("failed");
    result.wait(10000);
    ASSERT_TRUE(result.isCompleted());
    EXPECT_FALSE(result.isSucceeded());
    EXPECT_TRUE(result.isFailed());
    EXPECT_EQ(0, result.result());
    EXPECT_EQ(42, result.failureReason());
}

TEST_F(TasksTest, multipleTasks)
{
    std::atomic_bool ready{false};
    int n = 5;
    std::vector<TestFuture<TasksTestResult<int>>> results;
    for (int i = 0; i < n; ++i) {
        results.push_back(run(TaskType::Custom, 0, [&ready, i]() {
            while (!ready)
                std::this_thread::sleep_for(1ms);
            return pairedResult(i * 2);
        }));
    }
    for (int i = 0; i < n; ++i)
        EXPECT_FALSE(results[static_cast<size_t>(i)].isCompleted());
    ready = true;
    std::set<std::thread::id> threads;
    for (int i = n - 1; i >= 0; --i) {
        auto result = results[static_cast<size_t>(i)].result();
        threads.insert(result.first);
        EXPECT_NE(currentThread(), result.first);
        EXPECT_EQ(i * 2, result.second);
    }
    EXPECT_EQ(n, threads.size());
}

TEST_F(TasksTest, multipleTasksOverCapacity)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    int n = TasksDispatcher::instance()->capacity() * 2;
    std::vector<TestFuture<int>> results;
    for (int i = 0; i < n; ++i) {
        results.push_back(run(TaskType::Custom, 0, [&ready, &runCounter, i]() {
            ++runCounter;
            while (!ready)
                std::this_thread::sleep_for(1ms);
            return i * 2;
        }));
    }

    auto timeout = std::chrono::high_resolution_clock::now() + 10s;
    while (runCounter < TasksDispatcher::instance()->capacity() && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(TasksDispatcher::instance()->capacity(), runCounter);
    for (int i = 0; i < n; ++i)
        EXPECT_FALSE(results[static_cast<size_t>(i)].isCompleted());
    ready = true;
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, results[static_cast<size_t>(i)].result());
}

TEST_F(TasksTest, multipleTasksOverChangedCapacity)
{
    int32_t newCapacity = TasksDispatcher::instance()->capacity() + 10;
    TasksDispatcher::instance()->setCapacity(newCapacity);
    ASSERT_EQ(newCapacity, TasksDispatcher::instance()->capacity());
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    int n = TasksDispatcher::instance()->capacity() * 2;
    std::vector<TestFuture<int>> results;
    for (int i = 0; i < n; ++i) {
        results.push_back(run(TaskType::Custom, 0, [&ready, &runCounter, i]() {
            ++runCounter;
            while (!ready)
                std::this_thread::sleep_for(1ms);
            return i * 2;
        }));
    }
    auto timeout = std::chrono::high_resolution_clock::now() + 10s;
    while (runCounter < TasksDispatcher::instance()->capacity() && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(TasksDispatcher::instance()->capacity(), runCounter);
    for (int i = 0; i < n; ++i)
        EXPECT_FALSE(results[static_cast<size_t>(i)].isCompleted());
    ready = true;
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, results[static_cast<size_t>(i)].result());
}

TEST_F(TasksTest, multipleIntensiveTasksOverCapacity)
{
    std::atomic_bool ready{false};
    std::atomic_int runCounter{0};
    int capacity = TasksDispatcher::instance()->subPoolCapacity(TaskType::Intensive);
    int n = capacity * 2;
    std::vector<TestFuture<int>> results;
    for (int i = 0; i < n; ++i) {
        results.push_back(run(
            [&ready, &runCounter, i]() {
                ++runCounter;
                while (!ready)
                    std::this_thread::sleep_for(1ms);
                return i * 2;
            },
            TaskType::Intensive));
    }
    auto timeout = std::chrono::high_resolution_clock::now() + 10s;
    while (runCounter < capacity && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(capacity, runCounter);
    for (int i = 0; i < n; ++i)
        EXPECT_FALSE(results[static_cast<size_t>(i)].isCompleted());
    ready = true;
    for (int i = 0; i < n; ++i)
        EXPECT_EQ(i * 2, results[static_cast<size_t>(i)].result());
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
    std::vector<TestFuture<int>> results;
    std::vector<TestFuture<int>> otherResults;
    for (int i = 0; i < n; ++i) {
        results.push_back(run(TaskType::Custom, 42, [&ready, &runCounter, i]() {
            ++runCounter;
            while (!ready)
                std::this_thread::sleep_for(1ms);
            return i * 2;
        }));
        otherResults.push_back(run(TaskType::Custom, 24, [&ready, &otherRunCounter, i]() {
            ++otherRunCounter;
            while (!ready)
                std::this_thread::sleep_for(1ms);
            return i * 3;
        }));
    }
    auto timeout = std::chrono::high_resolution_clock::now() + 10s;
    while ((runCounter < capacity || otherRunCounter < otherCapacity)
           && std::chrono::high_resolution_clock::now() < timeout)
        ;
    std::this_thread::sleep_for(25ms);
    EXPECT_EQ(capacity, runCounter);
    EXPECT_EQ(otherCapacity, otherRunCounter);
    for (int i = 0; i < n; ++i) {
        EXPECT_FALSE(results[static_cast<size_t>(i)].isCompleted());
        EXPECT_FALSE(otherResults[static_cast<size_t>(i)].isCompleted());
    }
    ready = true;
    for (int i = 0; i < n; ++i) {
        EXPECT_EQ(i * 2, results[static_cast<size_t>(i)].result());
        EXPECT_EQ(i * 3, otherResults[static_cast<size_t>(i)].result());
    }
}

TEST_F(TasksTest, multipleTasksWithFailure)
{
    std::atomic_bool ready{false};
    int n = 5;
    std::vector<TestFuture<TasksTestResult<int>>> results;
    for (int i = 0; i < n; ++i) {
        results.push_back(run([&ready, i]() -> TasksTestResult<int> {
            while (!ready)
                ;
            if (i % 2)
                return WithTestFailure("failed");
            else
                return pairedResult(i * 2);
        }));
    }
    for (int i = 0; i < n; ++i)
        EXPECT_FALSE(results[static_cast<size_t>(i)].isCompleted());
    ready = true;
    for (int i = 0; i < n; ++i) {
        results[static_cast<size_t>(i)].wait(10000);
        ASSERT_TRUE(results[static_cast<size_t>(i)].isCompleted()) << i;
        if (i % 2) {
            ASSERT_TRUE(results[static_cast<size_t>(i)].isFailed()) << i;
            EXPECT_EQ("failed", results[static_cast<size_t>(i)].failureReason()) << i;
            EXPECT_EQ(0, results[static_cast<size_t>(i)].result().second) << i;
        } else {
            ASSERT_TRUE(results[static_cast<size_t>(i)].isSucceeded()) << i;
            auto result = results[static_cast<size_t>(i)].result();
            EXPECT_NE(currentThread(), result.first) << i;
            EXPECT_EQ(i * 2, result.second) << i;
        }
    }
}

TEST_F(TasksTest, mappedTaskWithFailure)
{
    std::atomic_bool ready{false};
    TestFuture<int> future = run([&ready]() {
        while (!ready)
            ;
        return 42;
    });
    TestFuture<int> mappedFuture = future.map([](int x) -> int { return WithTestFailure(std::to_string(x)); });
    EXPECT_FALSE(future.isCompleted());
    ready = true;
    mappedFuture.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    ASSERT_TRUE(future.isSucceeded());
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    ASSERT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ(0, mappedFuture.result());
    EXPECT_EQ("42", mappedFuture.failureReason());
}

TestFuture<int> deepRecursion(int step, int limit)
{
    auto result = run([step] { return step; });
    if (step >= limit)
        return result;
    return result.flatMap([limit](int x) -> TestFuture<int> { return (deepRecursion(x + 1, limit)); });
}

TestFuture<int> deepRecursionTrampolined(int step, int limit)
{
    auto result = run([step] { return step; });
    if (step >= limit)
        return result;
    return result.flatMap(
        [limit](int x) -> TestFuture<int> { return Trampoline(deepRecursionTrampolined(x + 1, limit)); });
}

constexpr int DEEP_RECURSION_LIMIT = 300000;

TEST_F(TasksDeathTest, deepRecursionNoTrampoline)
{
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    EXPECT_DEATH(deepRecursion(0, DEEP_RECURSION_LIMIT).wait(), ".*");
}

TEST_F(TasksTest, deepRecursionWithTrampoline)
{
    auto f = deepRecursionTrampolined(0, DEEP_RECURSION_LIMIT);
    f.wait();
}

TEST_F(TasksTest, trampolineFailure)
{
    TestPromise<int> p;
    auto f = TestFuture<int>::successful(5).flatMap([p](int x) -> TestFuture<int> { return Trampoline(p.future()); });
    EXPECT_FALSE(f.isCompleted());
    p.failure("failed");
    f.wait();
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isFailed());
    EXPECT_EQ("failed", f.failureReason());
}
