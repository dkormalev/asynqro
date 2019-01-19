#include "futurebasetest.h"

class FutureExceptionsTest : public FutureBaseTest
{};

TEST_F(FutureExceptionsTest, onSuccessException)
{
    Promise<int> promise;
    int result = 0;
    promise.future().onSuccess([](int) { throw std::runtime_error("Hi"); }).onSuccess([&result](int x) { result = x; });
    promise.success(42);
    EXPECT_EQ(42, result);
}

TEST_F(FutureExceptionsTest, onFailureException)
{
    Promise<int> promise;
    QVariant result;
    promise.future().onFailure([](QVariant) { throw std::runtime_error("Hi"); }).onFailure([&result](QVariant x) {
        result = x;
    });
    promise.failure("failed");
    EXPECT_EQ("failed", result.toString());
}

TEST_F(FutureExceptionsTest, onSuccessPostException)
{
    Promise<int> promise;
    int result = 0;
    promise.success(42);
    promise.future().onSuccess([](int) { throw std::runtime_error("Hi"); }).onSuccess([&result](int x) { result = x; });
    EXPECT_EQ(42, result);
}

TEST_F(FutureExceptionsTest, onFailurePostException)
{
    Promise<int> promise;
    QVariant result;
    promise.failure("failed");
    promise.future().onFailure([](QVariant) { throw std::runtime_error("Hi"); }).onFailure([&result](QVariant x) {
        result = x;
    });
    EXPECT_EQ("failed", result.toString());
}

TEST_F(FutureExceptionsTest, mapException)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().map([](int) -> int { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught: Hi", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, mapExceptionNonStd)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().map([](int) -> int { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, flatMapException)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().flatMap([](int) -> Future<int> { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught: Hi", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, flatMapExceptionNonStd)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().flatMap([](int) -> Future<int> { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, andThenException)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().andThen([]() -> Future<int> { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught: Hi", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, andThenExceptionNonStd)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().andThen([]() -> Future<int> { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, filterException)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().filter([](int) -> bool { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught: Hi", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, filterExceptionNonStd)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().filter([](int) -> bool { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, innerReduceException)
{
    Promise<QVector<int>> promise;
    auto mappedFuture = promise.future().innerReduce([](int, int) -> int { throw std::runtime_error("Hi"); }, 0);
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught: Hi", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, innerReduceExceptionNonStd)
{
    Promise<QVector<int>> promise;
    auto mappedFuture = promise.future().innerReduce([](int, int) -> int { throw 42; }, 0);
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, innerMapException)
{
    Promise<QVector<int>> promise;
    auto mappedFuture = promise.future().innerMap([](int) -> int { throw std::runtime_error("Hi"); }, QVector<int>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught: Hi", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, innerMapExceptionNonStd)
{
    Promise<QVector<int>> promise;
    auto mappedFuture = promise.future().innerMap([](int) -> int { throw 42; }, QVector<int>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, innerMapShortException)
{
    Promise<QVector<int>> promise;
    auto mappedFuture = promise.future().innerMap([](int) -> int { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught: Hi", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, innerMapShortExceptionNonStd)
{
    Promise<QVector<int>> promise;
    auto mappedFuture = promise.future().innerMap([](int) -> int { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, recoverException)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().recover([](const QVariant &) -> int { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught: Hi", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, recoverExceptionNonStd)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().recover([](const QVariant &) -> int { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, recoverWithException)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().recoverWith(
        [](const QVariant &) -> Future<int> { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught: Hi", mappedFuture.failureReason().toString());
}

TEST_F(FutureExceptionsTest, recoverWithExceptionNonStd)
{
    Promise<int> promise;
    auto mappedFuture = promise.future().recoverWith([](const QVariant &) -> Future<int> { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception caught", mappedFuture.failureReason().toString());
}
