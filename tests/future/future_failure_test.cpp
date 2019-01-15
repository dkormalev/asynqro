#include "futurebasetest.h"

class FutureFailureTest : public FutureBaseTest
{};

TEST_F(FutureFailureTest, cancelation)
{
    Promise<int> promise;
    CancelableFuture<int> future(promise);
    EXPECT_FALSE(future.isCompleted());
    future.cancel();
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("Canceled", future.failureReason().toString());
    promise.success(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("Canceled", future.failureReason().toString());
}

TEST_F(FutureFailureTest, withFailure)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    promise.success(WithFailure("failed"));
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("failed", future.failureReason().toString());
}

TEST_F(FutureFailureTest, failureFromMap)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    Future<int> mappedFuture = future.map([](int x) { return x * 2; });
    Future<int> mappedAgainFuture = mappedFuture.map([](int) -> int { return WithFailure("failed"); });
    Future<int> mappedOnceMoreFuture = mappedAgainFuture.map([](int) -> int { return 24; });
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
    EXPECT_EQ("failed", mappedAgainFuture.failureReason().toString());
    EXPECT_EQ(0, mappedAgainFuture.result());
    ASSERT_TRUE(mappedOnceMoreFuture.isCompleted());
    EXPECT_FALSE(mappedOnceMoreFuture.isSucceeded());
    EXPECT_TRUE(mappedOnceMoreFuture.isFailed());
    EXPECT_EQ("failed", mappedOnceMoreFuture.failureReason().toString());
    EXPECT_EQ(0, mappedOnceMoreFuture.result());
}

TEST_F(FutureFailureTest, failureFromFlatMap)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    Future<int> mappedFuture = future.map([](int x) { return x * 2; });
    Future<int> mappedAgainFuture = mappedFuture.flatMap([](int) -> Future<int> { return WithFailure("failed"); });
    Future<int> mappedOnceMoreFuture = mappedAgainFuture.map([](int) -> int { return 24; });
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
    EXPECT_EQ("failed", mappedAgainFuture.failureReason().toString());
    EXPECT_EQ(0, mappedAgainFuture.result());
    ASSERT_TRUE(mappedOnceMoreFuture.isCompleted());
    EXPECT_FALSE(mappedOnceMoreFuture.isSucceeded());
    EXPECT_TRUE(mappedOnceMoreFuture.isFailed());
    EXPECT_EQ("failed", mappedOnceMoreFuture.failureReason().toString());
    EXPECT_EQ(0, mappedOnceMoreFuture.result());
}

TEST_F(FutureFailureTest, failureFromAndThen)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    Future<int> mappedFuture = future.map([](int x) { return x * 2; });
    Future<int> mappedAgainFuture = mappedFuture.andThen([]() -> Future<int> { return WithFailure("failed"); });
    Future<int> mappedOnceMoreFuture = mappedAgainFuture.map([](int) -> int { return 24; });
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
    EXPECT_EQ("failed", mappedAgainFuture.failureReason().toString());
    EXPECT_EQ(0, mappedAgainFuture.result());
    ASSERT_TRUE(mappedOnceMoreFuture.isCompleted());
    EXPECT_FALSE(mappedOnceMoreFuture.isSucceeded());
    EXPECT_TRUE(mappedOnceMoreFuture.isFailed());
    EXPECT_EQ("failed", mappedOnceMoreFuture.failureReason().toString());
    EXPECT_EQ(0, mappedOnceMoreFuture.result());
}

TEST_F(FutureFailureTest, failureFromInnerReduce)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    Future<QVector<int>> mappedFuture = future.map([](int x) { return QVector<int>{x, x * 2}; });
    auto reducedFuture = mappedFuture.innerReduce([](int, int) -> int { return WithFailure("failed"); }, 0);
    Future<int> mappedOnceMoreFuture = reducedFuture.map([](int) -> int { return 24; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    ASSERT_EQ(2, mappedFuture.result().count());
    ASSERT_TRUE(reducedFuture.isCompleted());
    EXPECT_FALSE(reducedFuture.isSucceeded());
    EXPECT_TRUE(reducedFuture.isFailed());
    EXPECT_EQ("failed", reducedFuture.failureReason().toString());
    EXPECT_EQ(0, reducedFuture.result());
    ASSERT_TRUE(mappedOnceMoreFuture.isCompleted());
    EXPECT_FALSE(mappedOnceMoreFuture.isSucceeded());
    EXPECT_TRUE(mappedOnceMoreFuture.isFailed());
    EXPECT_EQ("failed", mappedOnceMoreFuture.failureReason().toString());
    EXPECT_EQ(0, mappedOnceMoreFuture.result());
}

TEST_F(FutureFailureTest, recover)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    Future<int> recoveredFuture = future.recover([](const QVariant &) { return 42; });
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("failed", future.failureReason().toString());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_FALSE(recoveredFuture.isFailed());
    EXPECT_EQ(42, recoveredFuture.result());
}

TEST_F(FutureFailureTest, recoverNoOp)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    Future<int> recoveredFuture = future.recover([](const QVariant &) { return 42; });
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.success(21);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_EQ(21, recoveredFuture.result());
}

TEST_F(FutureFailureTest, recoverFromWithFailure)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    Future<int> recoveredFuture = future.recover([](const QVariant &) { return 42; });
    EXPECT_NE(future, recoveredFuture);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.success(WithFailure("failed"));
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("failed", future.failureReason().toString());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_FALSE(recoveredFuture.isFailed());
    EXPECT_EQ(42, recoveredFuture.result());
}

TEST_F(FutureFailureTest, recoverAndFail)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    Future<int> recoveredFuture = future.recover([](const QVariant &) -> int { return WithFailure("failed2"); });
    EXPECT_NE(future, recoveredFuture);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("failed", future.failureReason().toString());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_FALSE(recoveredFuture.isSucceeded());
    EXPECT_TRUE(recoveredFuture.isFailed());
    EXPECT_TRUE(recoveredFuture.failureReason().isValid());
    EXPECT_EQ("failed2", recoveredFuture.failureReason().toString());
    EXPECT_EQ(0, recoveredFuture.result());
}

TEST_F(FutureFailureTest, recoverWith)
{
    Promise<int> promise;
    Promise<int> innerPromise;
    Future<int> future = promise.future();
    Future<int> recoveredFuture = future.recoverWith([innerPromise](const QVariant &) { return innerPromise.future(); });
    EXPECT_NE(future, recoveredFuture);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("failed", future.failureReason().toString());
    EXPECT_FALSE(recoveredFuture.isCompleted());

    innerPromise.success(42);
    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_FALSE(recoveredFuture.isFailed());
    EXPECT_EQ(42, recoveredFuture.result());
}

TEST_F(FutureFailureTest, recoverWithNoOp)
{
    Promise<int> promise;
    Promise<int> innerPromise;
    Future<int> future = promise.future();
    Future<int> recoveredFuture = future.recoverWith([innerPromise](const QVariant &) { return innerPromise.future(); });
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.success(21);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_EQ(21, recoveredFuture.result());
}

TEST_F(FutureFailureTest, recoverWithAndFail)
{
    Promise<int> promise;
    Promise<int> innerPromise;
    Future<int> future = promise.future();
    Future<int> recoveredFuture = future.recoverWith([innerPromise](const QVariant &) { return innerPromise.future(); });
    EXPECT_NE(future, recoveredFuture);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("failed", future.failureReason().toString());
    EXPECT_FALSE(recoveredFuture.isCompleted());

    innerPromise.failure("failed2");
    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_FALSE(recoveredFuture.isSucceeded());
    EXPECT_TRUE(recoveredFuture.failureReason().isValid());
    EXPECT_EQ("failed2", recoveredFuture.failureReason().toString());
}

TEST_F(FutureFailureTest, recoverValue)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    Future<int> recoveredFuture = future.recoverValue(42);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("failed", future.failureReason().toString());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_FALSE(recoveredFuture.isFailed());
    EXPECT_EQ(42, recoveredFuture.result());
}

TEST_F(FutureFailureTest, recoverValueNoOp)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    Future<int> recoveredFuture = future.recoverValue(42);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.success(21);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_EQ(21, recoveredFuture.result());
}
