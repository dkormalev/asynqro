#include "futurebasetest.h"

class FutureFailureTest : public FutureBaseTest
{};

TEST_F(FutureFailureTest, lastFailureSanity)
{
    struct Dummy
    {
        int nonZero = 3;
        int zero = 0;
    };

    EXPECT_FALSE(detail::hasLastFailure());
    int dummy = WithFailure<std::string>("abc");
    EXPECT_TRUE(detail::hasLastFailure());
    EXPECT_EQ("abc", detail::lastFailure<std::string>());
    EXPECT_TRUE(detail::hasLastFailure());
    Dummy dummyFailure = detail::lastFailure<Dummy>();
    EXPECT_EQ(0, dummyFailure.zero);
    EXPECT_EQ(3, dummyFailure.nonZero);
    EXPECT_TRUE(detail::hasLastFailure());
    EXPECT_EQ("abc", detail::lastFailure<std::string>());
    EXPECT_TRUE(detail::hasLastFailure());
    detail::invalidateLastFailure();
    EXPECT_FALSE(detail::hasLastFailure());
}

TEST_F(FutureFailureTest, cancelation)
{
    TestPromise<int> promise;
    CancelableTestFuture<int> future(promise);
    EXPECT_FALSE(future.isCompleted());
    future.cancel();
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("Canceled", future.failureReason());
    promise.success(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("Canceled", future.failureReason());
}

TEST_F(FutureFailureTest, withFailureCopy)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    std::string failure = "failed";
    promise.success(WithTestFailure(failure));
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());
}

TEST_F(FutureFailureTest, withFailureMove)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    promise.success(WithTestFailure(std::string("failed")));
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());
}

TEST_F(FutureFailureTest, withFailureArgs)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    promise.success(WithTestFailure("failed"));
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());
}

TEST_F(FutureFailureTest, failureFromMap)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    TestFuture<int> mappedFuture = future.map([](int x) { return x * 2; });
    TestFuture<int> mappedAgainFuture = mappedFuture.map([](int) -> int { return WithTestFailure("failed"); });
    TestFuture<int> mappedOnceMoreFuture = mappedAgainFuture.map([](int) -> int { return 24; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    EXPECT_NE(future, mappedFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(84, mappedFuture.result());
    ASSERT_TRUE(mappedAgainFuture.isCompleted());
    EXPECT_FALSE(mappedAgainFuture.isSucceeded());
    EXPECT_TRUE(mappedAgainFuture.isFailed());
    EXPECT_EQ("failed", mappedAgainFuture.failureReason());
    EXPECT_EQ(0, mappedAgainFuture.result());
    ASSERT_TRUE(mappedOnceMoreFuture.isCompleted());
    EXPECT_FALSE(mappedOnceMoreFuture.isSucceeded());
    EXPECT_TRUE(mappedOnceMoreFuture.isFailed());
    EXPECT_EQ("failed", mappedOnceMoreFuture.failureReason());
    EXPECT_EQ(0, mappedOnceMoreFuture.result());
}

TEST_F(FutureFailureTest, failureFromFlatMap)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    TestFuture<int> mappedFuture = future.map([](int x) { return x * 2; });
    TestFuture<int> mappedAgainFuture = mappedFuture.flatMap(
        [](int) -> TestFuture<int> { return WithTestFailure("failed"); });
    TestFuture<int> mappedOnceMoreFuture = mappedAgainFuture.map([](int) -> int { return 24; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    EXPECT_NE(future, mappedFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(84, mappedFuture.result());
    ASSERT_TRUE(mappedAgainFuture.isCompleted());
    EXPECT_FALSE(mappedAgainFuture.isSucceeded());
    EXPECT_TRUE(mappedAgainFuture.isFailed());
    EXPECT_EQ("failed", mappedAgainFuture.failureReason());
    EXPECT_EQ(0, mappedAgainFuture.result());
    ASSERT_TRUE(mappedOnceMoreFuture.isCompleted());
    EXPECT_FALSE(mappedOnceMoreFuture.isSucceeded());
    EXPECT_TRUE(mappedOnceMoreFuture.isFailed());
    EXPECT_EQ("failed", mappedOnceMoreFuture.failureReason());
    EXPECT_EQ(0, mappedOnceMoreFuture.result());
}

TEST_F(FutureFailureTest, failureFromAndThen)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    TestFuture<int> mappedFuture = future.map([](int x) { return x * 2; });
    TestFuture<int> mappedAgainFuture = mappedFuture.andThen(
        []() -> TestFuture<int> { return WithTestFailure("failed"); });
    TestFuture<int> mappedOnceMoreFuture = mappedAgainFuture.map([](int) -> int { return 24; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    EXPECT_NE(future, mappedFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(84, mappedFuture.result());
    ASSERT_TRUE(mappedAgainFuture.isCompleted());
    EXPECT_FALSE(mappedAgainFuture.isSucceeded());
    EXPECT_TRUE(mappedAgainFuture.isFailed());
    EXPECT_EQ("failed", mappedAgainFuture.failureReason());
    EXPECT_EQ(0, mappedAgainFuture.result());
    ASSERT_TRUE(mappedOnceMoreFuture.isCompleted());
    EXPECT_FALSE(mappedOnceMoreFuture.isSucceeded());
    EXPECT_TRUE(mappedOnceMoreFuture.isFailed());
    EXPECT_EQ("failed", mappedOnceMoreFuture.failureReason());
    EXPECT_EQ(0, mappedOnceMoreFuture.result());
}

TEST_F(FutureFailureTest, failureFromInnerReduce)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    TestFuture<std::vector<int>> mappedFuture = future.map([](int x) { return std::vector<int>{x, x * 2}; });
    auto reducedFuture = mappedFuture.innerReduce([](int, int) -> int { return WithTestFailure("failed"); }, 0);
    TestFuture<int> mappedOnceMoreFuture = reducedFuture.map([](int) -> int { return 24; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    ASSERT_EQ(2, mappedFuture.result().size());
    ASSERT_TRUE(reducedFuture.isCompleted());
    EXPECT_FALSE(reducedFuture.isSucceeded());
    EXPECT_TRUE(reducedFuture.isFailed());
    EXPECT_EQ("failed", reducedFuture.failureReason());
    EXPECT_EQ(0, reducedFuture.result());
    ASSERT_TRUE(mappedOnceMoreFuture.isCompleted());
    EXPECT_FALSE(mappedOnceMoreFuture.isSucceeded());
    EXPECT_TRUE(mappedOnceMoreFuture.isFailed());
    EXPECT_EQ("failed", mappedOnceMoreFuture.failureReason());
    EXPECT_EQ(0, mappedOnceMoreFuture.result());
}
