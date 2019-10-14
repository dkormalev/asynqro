#include "tasksbasetest.h"

using namespace std::chrono_literals;
using namespace asynqro::repeater;

class RepeatTest : public TasksBaseTest
{};

using RepeatDeathTest = RepeatTest;
using RepeatedDataResult = RepeaterResult<std::vector<int>, int, std::vector<int>>;
using RepeatedFutureResult = RepeaterFutureResult<int, std::string, int>;

struct CopyCheck
{
    CopyCheck() { ++createCounter; }
    CopyCheck(const CopyCheck &) { ++copyCounter; }
    CopyCheck &operator=(const CopyCheck &)
    {
        ++copyCounter;
        return *this;
    }
    CopyCheck(CopyCheck &&other) = default;
    CopyCheck &operator=(CopyCheck &&other) = default;
    ~CopyCheck() = default;

    static inline std::atomic_int createCounter{0};
    static inline std::atomic_int copyCounter{0};
};

using RepeatedCopyCheckResult = RepeaterResult<CopyCheck, CopyCheck>;
using RepeatedFutureCopyCheckResult = RepeaterFutureResult<CopyCheck, std::string, CopyCheck>;

TEST_F(RepeatTest, repeatFuture)
{
    TestPromise<RepeatedFutureResult::Value> promise;
    std::vector<int> order;
    TestFuture<int> f = repeat<int, std::string>(
        [promise, &order](int step) -> RepeatedFutureResult {
            order.push_back(step);
            if (step >= 99)
                return promise.future();
            return tasks::run([step]() -> RepeaterResult<int, int> { return Continue(step + 1); });
        },
        0);
    ASSERT_FALSE(f.isCompleted());
    promise.success(42);
    f.wait();
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isSucceeded());
    EXPECT_EQ(42, f.result());
    EXPECT_EQ(100, order.size());
    for (int i = 0; i < order.size(); ++i)
        EXPECT_EQ(i, order[i]);
}

TEST_F(RepeatTest, repeatData)
{
    TestFuture<std::vector<int>> f = repeat<std::vector<int>, std::string>(
        [](int step, std::vector<int> order) -> RepeatedDataResult {
            order.push_back(step);
            if (step >= 99)
                return Finish(order);
            return Continue(step + 1, std::move(order));
        },
        0, std::vector<int>{});
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isSucceeded());
    EXPECT_EQ(100, f.resultRef().size());
    for (int i = 0; i < f.resultRef().size(); ++i)
        EXPECT_EQ(i, f.resultRef()[i]);
}

TEST_F(RepeatTest, repeatFutureFailure)
{
    TestPromise<RepeatedFutureResult::Value> promise;
    std::vector<int> order;
    TestFuture<int> f = repeat<int, std::string>(
        [promise, &order](int step) -> RepeatedFutureResult {
            order.push_back(step);
            if (step >= 99)
                return promise.future();
            return tasks::run([step]() -> RepeaterResult<int, int> { return Continue(step + 1); });
        },
        0);
    ASSERT_FALSE(f.isCompleted());
    promise.failure("failed");
    f.wait();
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isFailed());
    EXPECT_EQ("failed", f.failureReason());
    EXPECT_EQ(100, order.size());
    for (int i = 0; i < order.size(); ++i)
        EXPECT_EQ(i, order[i]);
}

TEST_F(RepeatTest, repeatDataFailure)
{
    TestFuture<std::vector<int>> f = repeat<std::vector<int>, std::string>(
        [](int step, std::vector<int> order) -> RepeatedDataResult {
            order.push_back(step);
            if (step >= 99)
                return WithFailure(std::string("failed"));
            return Continue(step + 1, std::move(order));
        },
        0, std::vector<int>{});
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isFailed());
    EXPECT_EQ("failed", f.failureReason());
}

TEST_F(RepeatTest, repeatFutureFailureFromException)
{
    TestPromise<RepeatedFutureResult::Value> promise;
    std::vector<int> order;
    TestFuture<int> f = repeat<int, std::string>(
        [promise, &order](int step) -> RepeatedFutureResult {
            order.push_back(step);
            throw std::runtime_error("Hi");
            if (step >= 99)
                return promise.future();
            return tasks::run([step]() -> RepeaterResult<int, int> { return Continue(step + 1); });
        },
        0);
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isFailed());
    EXPECT_EQ("Exception: Hi", f.failureReason());
    EXPECT_EQ(1, order.size());
    for (int i = 0; i < order.size(); ++i)
        EXPECT_EQ(i, order[i]);
}

TEST_F(RepeatTest, repeatDataFailureFromException)
{
    TestFuture<std::vector<int>> f = repeat<std::vector<int>, std::string>(
        [](int step, std::vector<int> order) -> RepeatedDataResult {
            order.push_back(step);
            throw std::runtime_error("Hi");
            if (step >= 99)
                return WithFailure(std::string("failed"));
            return Continue(step + 1, std::move(order));
        },
        0, std::vector<int>{});
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isFailed());
    EXPECT_EQ("Exception: Hi", f.failureReason());
}

TEST_F(RepeatTest, repeatFutureFailureFromNonStdException)
{
    TestPromise<RepeatedFutureResult::Value> promise;
    std::vector<int> order;
    TestFuture<int> f = repeat<int, std::string>(
        [promise, &order](int step) -> RepeatedFutureResult {
            order.push_back(step);
            throw 42;
            if (step >= 99)
                return promise.future();
            return tasks::run([step]() -> RepeaterResult<int, int> { return Continue(step + 1); });
        },
        0);
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isFailed());
    EXPECT_EQ("Exception", f.failureReason());
    EXPECT_EQ(1, order.size());
    for (int i = 0; i < order.size(); ++i)
        EXPECT_EQ(i, order[i]);
}

TEST_F(RepeatTest, repeatDataFailureFromNonStdException)
{
    TestFuture<std::vector<int>> f = repeat<std::vector<int>, std::string>(
        [](int step, std::vector<int> order) -> RepeatedDataResult {
            order.push_back(step);
            throw 42;
            if (step >= 99)
                return WithFailure(std::string("failed"));
            return Continue(step + 1, std::move(order));
        },
        0, std::vector<int>{});
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isFailed());
    EXPECT_EQ("Exception", f.failureReason());
}

TEST_F(RepeatTest, repeatCopyCheck)
{
    CopyCheck::copyCounter = 0;
    CopyCheck::createCounter = 0;
    int step = 0;
    TestFuture<CopyCheck> f = repeat<CopyCheck, std::string>(
        [&step](CopyCheck x) -> RepeatedCopyCheckResult {
            if (step >= 99)
                return Finish(std::move(x));
            ++step;
            return Continue(std::move(x));
        },
        CopyCheck());
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isSucceeded());
    EXPECT_EQ(0, CopyCheck::copyCounter);
    EXPECT_EQ(1, CopyCheck::createCounter);
}

TEST_F(RepeatTest, repeatFutureCopyCheck)
{
    CopyCheck::copyCounter = 0;
    CopyCheck::createCounter = 0;
    int step = 0;
    TestFuture<CopyCheck> f = repeat<CopyCheck, std::string>(
        [&step](CopyCheck x) -> RepeatedFutureCopyCheckResult {
            if (step >= 99)
                return RepeatedFutureCopyCheckResult::successful(std::move(x));
            ++step;
            return RepeatedFutureCopyCheckResult::successful(Continue(std::move(x)));
        },
        CopyCheck());
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isSucceeded());
    //It should only copy where it is required by Future design - input and output of flatMap, i.e. twice per iteration
    EXPECT_EQ(200, CopyCheck::copyCounter);
    EXPECT_EQ(1, CopyCheck::createCounter);
}

TEST_F(RepeatTest, repeatForSequence)
{
    std::vector<TestPromise<double>> promises(5);
    TestFuture<std::vector<double>> f = repeatForSequence(std::vector<size_t>{0, 1, 2, 3, 4}, std::vector<double>{},
                                                          [&promises](size_t x, std::vector<double> result) {
                                                              return promises[x].future().map([result](double x) {
                                                                  auto newResult = result;
                                                                  newResult.push_back(x);
                                                                  return newResult;
                                                              });
                                                          });
    promises[2].success(0.2);
    for (size_t i = 0; i < promises.size(); ++i) {
        ASSERT_FALSE(f.isCompleted());
        promises[i].success(i / 10.0);
    }
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isSucceeded());
    EXPECT_EQ(5, f.resultRef().size());
    for (size_t i = 0; i < f.resultRef().size(); ++i)
        EXPECT_DOUBLE_EQ(i / 10.0, f.resultRef()[i]);
}

TEST_F(RepeatTest, repeatForSequenceFailure)
{
    std::vector<TestPromise<double>> promises(5);
    int callsCount = 0;
    TestFuture<double> f = repeatForSequence(std::vector<size_t>{0, 1, 2, 3, 4}, 0.0,
                                             [&promises, &callsCount](size_t x, double) {
                                                 ++callsCount;
                                                 return promises[x].future();
                                             });
    for (size_t i = 0; i < promises.size() - 2; ++i) {
        ASSERT_FALSE(f.isCompleted());
        promises[i].success(i / 10.0);
    }
    promises[promises.size() - 2].failure("failed");
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isFailed());
    EXPECT_EQ("failed", f.failureReason());
    EXPECT_EQ(promises.size() - 1, callsCount);
}

TEST_F(RepeatTest, repeatForSequenceFastFailure)
{
    std::vector<TestPromise<double>> promises(5);
    int callsCount = 0;
    TestFuture<double> f = repeatForSequence(std::vector<size_t>{0, 1, 2, 3, 4}, 0.0,
                                             [&promises, &callsCount](size_t x, double) {
                                                 ++callsCount;
                                                 return promises[x].future();
                                             });
    promises[promises.size() - 2].failure("failed");
    for (size_t i = 0; i < promises.size() - 2; ++i) {
        ASSERT_FALSE(f.isCompleted());
        promises[i].success(i / 10.0);
    }
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isFailed());
    EXPECT_EQ("failed", f.failureReason());
    EXPECT_EQ(promises.size() - 1, callsCount);
}

TEST_F(RepeatTest, repeatForSequenceFailureFromException)
{
    int callsCount = 0;
    TestFuture<double> f = repeatForSequence(std::vector<size_t>{0, 1, 2, 3, 4}, 0.0, [&callsCount](size_t, double) {
        ++callsCount;
        throw std::runtime_error("Hi");
        return TestFuture<double>::successful();
    });
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isFailed());
    EXPECT_EQ("Exception: Hi", f.failureReason());
    EXPECT_EQ(1, callsCount);
}

TEST_F(RepeatTest, repeatForSequenceFailureFromNonStdException)
{
    int callsCount = 0;
    TestFuture<double> f = repeatForSequence(std::vector<size_t>{0, 1, 2, 3, 4}, 0.0, [&callsCount](size_t, double) {
        ++callsCount;
        throw 42;
        return TestFuture<double>::successful();
    });
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isFailed());
    EXPECT_EQ("Exception", f.failureReason());
    EXPECT_EQ(1, callsCount);
}

TEST_F(RepeatTest, repeatForSequenceConstRefData)
{
    std::vector<TestPromise<double>> promises(5);
    std::vector<size_t> data = std::vector<size_t>{0, 1, 2, 3, 4};
    TestFuture<std::vector<double>> f = repeatForSequence(data, std::vector<double>{},
                                                          [&promises](size_t x, std::vector<double> result) {
                                                              return promises[x].future().map([result](double x) {
                                                                  auto newResult = result;
                                                                  newResult.push_back(x);
                                                                  return newResult;
                                                              });
                                                          });
    for (size_t i = 0; i < promises.size(); ++i) {
        ASSERT_FALSE(f.isCompleted());
        promises[i].success(i / 10.0);
    }
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isSucceeded());
    EXPECT_EQ(5, f.resultRef().size());
    for (size_t i = 0; i < f.resultRef().size(); ++i)
        EXPECT_DOUBLE_EQ(i / 10.0, f.resultRef()[i]);
}

TEST_F(RepeatTest, repeatForSequenceEmpty)
{
    int callsCount = 0;
    TestFuture<double> f = repeatForSequence(std::vector<size_t>{}, 3.5, [&callsCount](size_t, double result) {
        ++callsCount;
        return TestFuture<double>::successful(result * 2);
    });
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isSucceeded());
    EXPECT_DOUBLE_EQ(3.5, f.result());
    EXPECT_EQ(0, callsCount);
}

TEST_F(RepeatTest, repeatForSequenceCopyCheck)
{
    CopyCheck::copyCounter = 0;
    CopyCheck::createCounter = 0;
    int callsCount = 0;
    std::vector<TestPromise<size_t>> promises(100);
    TestFuture<size_t> f = repeatForSequence<size_t>(std::vector<CopyCheck>(100), 0,
                                                     [&promises, &callsCount](const CopyCheck &, size_t result) {
                                                         ++callsCount;
                                                         return promises[result].future();
                                                     });
    for (size_t i = 0; i < promises.size(); ++i) {
        ASSERT_FALSE(f.isCompleted());
        promises[i].success(i + 1);
    }
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isSucceeded());
    EXPECT_EQ(promises.size(), callsCount);
    EXPECT_EQ(0, CopyCheck::copyCounter);
    EXPECT_EQ(100, CopyCheck::createCounter);
}

constexpr int DEEP_RECURSION_LIMIT = 300000;

TEST_F(RepeatTest, repeatDataDeepNoTrampoline)
{
    TestFuture<int> f = repeat<int, std::string>(
        [](int step) -> RepeaterResult<int, int> {
            if (step >= DEEP_RECURSION_LIMIT * 10)
                return Finish(step);
            return Continue(step + 1);
        },
        0);
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isSucceeded());
    EXPECT_EQ(DEEP_RECURSION_LIMIT * 10, f.result());
}

TEST_F(RepeatDeathTest, repeatFutureDeepNoTrampoline)
{
#ifndef ASYNQRO_GCOV_ENABLED
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    auto death = []() {
        TestPromise<RepeatedFutureResult::Value> promise;
        TestFuture<int> f = repeat<int, std::string>(
            [promise](int step) -> RepeatedFutureResult {
                if (step >= DEEP_RECURSION_LIMIT)
                    return promise.future();
                return tasks::run([step]() -> RepeaterResult<int, int> { return Continue(step + 1); });
            },
            0);
        promise.success(42);
        f.wait();
    };
    ASSERT_DEATH(death(), ".*");
#endif
}

TEST_F(RepeatTest, repeatFutureDeepWithOccasionalTrampoline)
{
    TestPromise<RepeatedFutureResult::Value> promise;
    TestFuture<int> f = repeat<int, std::string>(
        [promise](int step) -> RepeatedFutureResult {
            if (step >= DEEP_RECURSION_LIMIT)
                return promise.future();
            return tasks::run([step]() -> RepeaterResult<int, int> {
                if (step % 500)
                    return Continue(step + 1);
                return TrampolinedContinue(step + 1);
            });
        },
        0);
    ASSERT_FALSE(f.isCompleted());
    promise.success(42);
    f.wait();
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isSucceeded());
    EXPECT_EQ(42, f.result());
}

TEST_F(RepeatTest, repeatFutureDeepWithTrampoline)
{
    TestPromise<RepeatedFutureResult::Value> promise;
    TestFuture<int> f = repeat<int, std::string>(
        [promise](int step) -> RepeatedFutureResult {
            if (step >= DEEP_RECURSION_LIMIT)
                return promise.future();
            return tasks::run([step]() -> RepeaterResult<int, int> { return TrampolinedContinue(step + 1); });
        },
        0);
    ASSERT_FALSE(f.isCompleted());
    promise.success(42);
    f.wait();
    ASSERT_TRUE(f.isCompleted());
    ASSERT_TRUE(f.isSucceeded());
    EXPECT_EQ(42, f.result());
}
