#include "futurebasetest.h"

class FutureCallbacksTest : public FutureBaseTest
{};

TEST_F(FutureCallbacksTest, onSuccess)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    int result = 0;
    Future<int> futureWithCallback = future.onSuccess([&result](int x) { result = x; });
    EXPECT_EQ(future, futureWithCallback);
    promise.success(42);
    EXPECT_EQ(42, result);
}

TEST_F(FutureCallbacksTest, onFailure)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    QVariant result;
    Future<int> futureWithCallback = future.onFailure([&result](QVariant x) { result = x; });
    EXPECT_EQ(future, futureWithCallback);
    promise.failure("failed");
    EXPECT_EQ("failed", result.toString());
}

TEST_F(FutureCallbacksTest, multipleOnSuccess)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    int result[3] = {0, 0, 0};
    QVariant failedResult[3] = {QVariant(), QVariant(), QVariant()};
    for (int i = 0; i < 3; ++i) {
        Future<int> futureWithCallback = future.onSuccess([&result = result[i]](int x) { result = x; });
        EXPECT_EQ(future, futureWithCallback);
        futureWithCallback = future.onFailure([&failedResult = failedResult[i]](QVariant x) { failedResult = x; });
        EXPECT_EQ(future, futureWithCallback);
    }
    promise.success(42);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(42, result[i]);
        EXPECT_FALSE(failedResult[i].isValid());
    }
}

TEST_F(FutureCallbacksTest, multipleOnFailure)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    int result[3] = {0, 0, 0};
    QVariant failedResult[3] = {QVariant(), QVariant(), QVariant()};
    for (int i = 0; i < 3; ++i) {
        Future<int> futureWithCallback = future.onSuccess([&result = result[i]](int x) { result = x; });
        EXPECT_EQ(future, futureWithCallback);
        futureWithCallback = future.onFailure([&failedResult = failedResult[i]](QVariant x) { failedResult = x; });
        EXPECT_EQ(future, futureWithCallback);
    }
    promise.failure("failed");
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(0, result[i]);
        EXPECT_EQ("failed", failedResult[i].toString());
    }
}
