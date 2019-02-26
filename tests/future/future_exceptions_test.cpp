#include "futurebasetest.h"

class FutureExceptionsTest : public FutureBaseTest
{};

TEST_F(FutureExceptionsTest, onSuccessException)
{
    TestPromise<int> promise;
    int result = 0;
    promise.future().onSuccess([](int) { throw std::runtime_error("Hi"); }).onSuccess([&result](int x) { result = x; });
    promise.success(42);
    EXPECT_EQ(42, result);
}

TEST_F(FutureExceptionsTest, onFailureException)
{
    TestPromise<int> promise;
    std::string result;
    promise.future().onFailure([](auto) { throw std::runtime_error("Hi"); }).onFailure([&result](auto x) { result = x; });
    promise.failure("failed");
    EXPECT_EQ("failed", result);
}

TEST_F(FutureExceptionsTest, onSuccessPostException)
{
    TestPromise<int> promise;
    int result = 0;
    promise.success(42);
    promise.future().onSuccess([](int) { throw std::runtime_error("Hi"); }).onSuccess([&result](int x) { result = x; });
    EXPECT_EQ(42, result);
}

TEST_F(FutureExceptionsTest, onFailurePostException)
{
    TestPromise<int> promise;
    std::string result;
    promise.failure("failed");
    promise.future().onFailure([](auto) { throw std::runtime_error("Hi"); }).onFailure([&result](auto x) { result = x; });
    EXPECT_EQ("failed", result);
}

TEST_F(FutureExceptionsTest, mapException)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().map([](int) -> int { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception: Hi", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, mapExceptionNonStd)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().map([](int) -> int { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, mapFailureException)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().mapFailure([](std::string) -> std::string { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("42");
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception: Hi", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, mapFailureExceptionNonStd)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().mapFailure([](std::string) -> std::string { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("42");
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, flatMapException)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().flatMap([](int) -> TestFuture<int> { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception: Hi", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, flatMapExceptionNonStd)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().flatMap([](int) -> TestFuture<int> { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, andThenException)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().andThen([]() -> TestFuture<int> { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception: Hi", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, andThenExceptionNonStd)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().andThen([]() -> TestFuture<int> { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, filterException)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().filter([](int) -> bool { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception: Hi", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, filterExceptionNonStd)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().filter([](int) -> bool { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, innerReduceException)
{
    TestPromise<std::vector<int>> promise;
    auto mappedFuture = promise.future().innerReduce([](int, int) -> int { throw std::runtime_error("Hi"); }, 0);
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception: Hi", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, innerReduceExceptionNonStd)
{
    TestPromise<std::vector<int>> promise;
    auto mappedFuture = promise.future().innerReduce([](int, int) -> int { throw 42; }, 0);
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, innerMapException)
{
    TestPromise<std::vector<int>> promise;
    auto mappedFuture = promise.future().innerMap([](int) -> int { throw std::runtime_error("Hi"); }, std::vector<int>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception: Hi", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, innerMapExceptionNonStd)
{
    TestPromise<std::vector<int>> promise;
    auto mappedFuture = promise.future().innerMap([](int) -> int { throw 42; }, std::vector<int>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, innerMapShortException)
{
    TestPromise<std::vector<int>> promise;
    auto mappedFuture = promise.future().innerMap([](int) -> int { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception: Hi", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, innerMapShortExceptionNonStd)
{
    TestPromise<std::vector<int>> promise;
    auto mappedFuture = promise.future().innerMap([](int) -> int { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({42});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, recoverException)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().recover([](const auto &) -> int { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception: Hi", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, recoverExceptionNonStd)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().recover([](const auto &) -> int { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, recoverWithException)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().recoverWith(
        [](const auto &) -> TestFuture<int> { throw std::runtime_error("Hi"); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception: Hi", mappedFuture.failureReason());
}

TEST_F(FutureExceptionsTest, recoverWithExceptionNonStd)
{
    TestPromise<int> promise;
    auto mappedFuture = promise.future().recoverWith([](const auto &) -> TestFuture<int> { throw 42; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("Exception", mappedFuture.failureReason());
}
