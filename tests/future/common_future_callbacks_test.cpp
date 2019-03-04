#include "futurebasetest.h"

class FutureCallbacksTest : public FutureBaseTest
{};

TEST_F(FutureCallbacksTest, onSuccess)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    int result = 0;
    TestFuture<int> futureWithCallback = future.onSuccess([&result](int x) { result = x; });
    EXPECT_EQ(future, futureWithCallback);
    EXPECT_EQ(futureWithCallback, future);
    promise.success(42);
    EXPECT_EQ(42, result);
}

TEST_F(FutureCallbacksTest, onFailure)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    std::string result;
    TestFuture<int> futureWithCallback = future.onFailure([&result](auto x) { result = x; });
    EXPECT_EQ(future, futureWithCallback);
    EXPECT_EQ(futureWithCallback, future);
    promise.failure("failed");
    EXPECT_EQ("failed", result);
}

TEST_F(FutureCallbacksTest, multipleOnSuccess)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    int result[3] = {0, 0, 0};
    std::string failedResult[3] = {};
    for (int i = 0; i < 3; ++i) {
        TestFuture<int> futureWithCallback = future.onSuccess([&result = result[i]](int x) { result = x; });
        EXPECT_EQ(future, futureWithCallback);
        futureWithCallback = future.onFailure([&failedResult = failedResult[i]](auto x) { failedResult = x; });
        EXPECT_EQ(future, futureWithCallback);
    }
    promise.success(42);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(42, result[i]);
        EXPECT_TRUE(failedResult[i].empty());
    }
}

TEST_F(FutureCallbacksTest, multipleOnFailure)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    int result[3] = {0, 0, 0};
    std::string failedResult[3] = {};
    for (int i = 0; i < 3; ++i) {
        TestFuture<int> futureWithCallback = future.onSuccess([&result = result[i]](int x) { result = x; });
        EXPECT_EQ(future, futureWithCallback);
        futureWithCallback = future.onFailure([&failedResult = failedResult[i]](auto x) { failedResult = x; });
        EXPECT_EQ(future, futureWithCallback);
    }
    promise.failure("failed");
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(0, result[i]);
        EXPECT_EQ("failed", failedResult[i]);
    }
}
