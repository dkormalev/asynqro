#include "futurebasetest.h"

class FutureMorphismsTest : public FutureBaseTest
{};

TEST_F(FutureMorphismsTest, map)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    Future<int> mappedFuture = future.map([](int x) { return x * 2; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    EXPECT_NE(future, mappedFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(84, mappedFuture.result());
}
TEST_F(FutureMorphismsTest, differentTypeMap)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    Future<double> mappedFuture = future.map([](int x) -> double { return x / 2.0; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(21.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, flatMap)
{
    Promise<int> promise;
    Promise<int> innerPromise;
    auto future = createFuture(promise);
    Future<int> mappedFuture = future.flatMap(
        [innerPromise](int x) { return innerPromise.future().map([x](int y) { return x * y; }); });
    EXPECT_NE(future, mappedFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(mappedFuture.isCompleted());
    innerPromise.success(2);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(84, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, differentTypeFlatMap)
{
    Promise<int> promise;
    Promise<double> innerPromise;
    auto future = createFuture(promise);
    Future<double> mappedFuture = future.flatMap(
        [innerPromise](int x) { return innerPromise.future().map([x](double y) { return x / y; }); });
    promise.success(42);
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(mappedFuture.isCompleted());
    innerPromise.success(2.0);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(21.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, andThen)
{
    Promise<int> promise;
    Promise<int> innerPromise;
    auto future = createFuture(promise);
    Future<int> mappedFuture = future.andThen([innerPromise]() { return innerPromise.future(); });
    EXPECT_NE(future, mappedFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(mappedFuture.isCompleted());
    innerPromise.success(2);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(2, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, andThenValueR)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    Future<int> mappedFuture = future.andThenValue(2);
    EXPECT_NE(future, mappedFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(2, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, andThenValueL)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    int result = 2;
    Future<int> mappedFuture = future.andThenValue(result);
    EXPECT_NE(future, mappedFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(2, mappedFuture.result());
    EXPECT_EQ(2, result);
}

TEST_F(FutureMorphismsTest, andThenValueCL)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    const int result = 2;
    Future<int> mappedFuture = future.andThenValue(result);
    EXPECT_NE(future, mappedFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(2, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, differentTypeAndThen)
{
    Promise<int> promise;
    Promise<double> innerPromise;
    auto future = createFuture(promise);
    Future<double> mappedFuture = future.andThen([innerPromise]() { return innerPromise.future(); });
    promise.success(42);
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(mappedFuture.isCompleted());
    innerPromise.success(2.0);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(2.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, differentTypeAndThenValueR)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    Future<double> mappedFuture = future.andThenValue(2.0);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(2.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, differentTypeAndThenValueL)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    double result = 2.0;
    Future<double> mappedFuture = future.andThenValue(result);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(2.0, mappedFuture.result());
    EXPECT_DOUBLE_EQ(2.0, result);
}

TEST_F(FutureMorphismsTest, differentTypeAndThenValueCL)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    const double result = 2.0;
    Future<double> mappedFuture = future.andThenValue(result);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(2.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, filterPositive)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    Future<int> filteredFuture = future.filter([](int x) -> bool { return x % 2; });
    EXPECT_FALSE(filteredFuture.isCompleted());
    EXPECT_NE(future, filteredFuture);
    promise.success(41);
    EXPECT_EQ(41, future.result());
    ASSERT_TRUE(filteredFuture.isCompleted());
    EXPECT_TRUE(filteredFuture.isSucceeded());
    EXPECT_FALSE(filteredFuture.isFailed());
    EXPECT_EQ(41, filteredFuture.result());
}

TEST_F(FutureMorphismsTest, filterNegative)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    Future<int> filteredFuture = future.filter([](int x) { return x % 2; });
    EXPECT_FALSE(filteredFuture.isCompleted());
    EXPECT_NE(future, filteredFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(filteredFuture.isCompleted());
    EXPECT_FALSE(filteredFuture.isSucceeded());
    EXPECT_TRUE(filteredFuture.isFailed());
    EXPECT_EQ("Result wasn't good enough", filteredFuture.failureReason().toString());
}

TEST_F(FutureMorphismsTest, filterNegativeCustomRejectedR)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    Future<int> filteredFuture = future.filter([](int) { return false; }, "Custom");
    EXPECT_FALSE(filteredFuture.isCompleted());
    EXPECT_NE(future, filteredFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(filteredFuture.isCompleted());
    EXPECT_FALSE(filteredFuture.isSucceeded());
    EXPECT_TRUE(filteredFuture.isFailed());
    EXPECT_EQ("Custom", filteredFuture.failureReason().toString());
}

TEST_F(FutureMorphismsTest, filterNegativeCustomRejectedL)
{
    Promise<int> promise;
    auto future = createFuture(promise);
    QVariant rejected = "Custom";
    Future<int> filteredFuture = future.filter([](int) { return false; }, rejected);
    EXPECT_FALSE(filteredFuture.isCompleted());
    EXPECT_NE(future, filteredFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(filteredFuture.isCompleted());
    EXPECT_FALSE(filteredFuture.isSucceeded());
    EXPECT_TRUE(filteredFuture.isFailed());
    EXPECT_EQ("Custom", filteredFuture.failureReason().toString());
}
