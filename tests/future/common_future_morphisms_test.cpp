#include "futurebasetest.h"

class FutureMorphismsTest : public FutureBaseTest
{};

TEST_F(FutureMorphismsTest, map)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> mappedFuture = future.map([](int x) { return x * 2; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    EXPECT_NE(future, mappedFuture);
    EXPECT_NE(mappedFuture, future);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(84, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, differentTypeMap)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<double> mappedFuture = future.map([](int x) -> double { return x / 2.0; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(21.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, mapFailure)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> mappedFuture = future.mapFailure([](std::string x) { return x + "abc"; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    EXPECT_NE(future, mappedFuture);
    EXPECT_NE(mappedFuture, future);
    promise.failure("42");
    EXPECT_EQ("42", future.failureReason());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("42abc", mappedFuture.failureReason());
}

TEST_F(FutureMorphismsTest, differentTypeMapFailure)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    Future<int, int> mappedFuture = future.mapFailure([](std::string x) { return std::stoi(x); });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("42");
    EXPECT_EQ("42", future.failureReason());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ(42, mappedFuture.failureReason());
}

TEST_F(FutureMorphismsTest, mapNoEffect)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    bool executed = false;
    TestFuture<int> mappedFuture = future.map([&executed](int x) {
        executed = true;
        return x * 2;
    });
    EXPECT_FALSE(mappedFuture.isCompleted());
    EXPECT_NE(future, mappedFuture);
    promise.failure("42");
    mappedFuture.wait(1000);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("42", mappedFuture.failureReason());
    EXPECT_FALSE(executed);
}
TEST_F(FutureMorphismsTest, differentTypeMapNoEffect)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    bool executed = false;
    TestFuture<double> mappedFuture = future.map([&executed](int x) {
        executed = true;
        return x / 2.0;
    });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.failure("42");
    mappedFuture.wait(1000);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("42", mappedFuture.failureReason());
    EXPECT_FALSE(executed);
}

TEST_F(FutureMorphismsTest, mapFailureNoEffect)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    bool executed = false;
    TestFuture<int> mappedFuture = future.mapFailure([&executed](std::string x) {
        executed = true;
        return x + "abc";
    });
    EXPECT_FALSE(mappedFuture.isCompleted());
    EXPECT_NE(future, mappedFuture);
    promise.success(42);
    mappedFuture.wait(1000);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(42, mappedFuture.result());
    EXPECT_FALSE(executed);
}

TEST_F(FutureMorphismsTest, differentTypeMapFailureNoEffect)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    bool executed = false;
    Future<int, int> mappedFuture = future.mapFailure([&executed](std::string x) {
        executed = true;
        return std::stoi(x);
    });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    mappedFuture.wait(1000);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(42, mappedFuture.result());
    EXPECT_FALSE(executed);
}

TEST_F(FutureMorphismsTest, flatMap)
{
    TestPromise<int> promise;
    TestPromise<int> innerPromise;
    auto future = createFuture(promise);
    TestFuture<int> mappedFuture = future.flatMap(
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
    TestPromise<int> promise;
    TestPromise<double> innerPromise;
    auto future = createFuture(promise);
    TestFuture<double> mappedFuture = future.flatMap(
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

TEST_F(FutureMorphismsTest, flatMapNoEffect)
{
    TestPromise<int> promise;
    TestPromise<int> innerPromise;
    auto future = createFuture(promise);
    bool executed = false;
    TestFuture<int> mappedFuture = future.flatMap([innerPromise, &executed](int x) {
        executed = true;
        return innerPromise.future().map([x](int y) { return x * y; });
    });
    EXPECT_NE(future, mappedFuture);
    promise.failure("42");
    innerPromise.success(2);
    mappedFuture.wait(1000);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("42", mappedFuture.failureReason());
    EXPECT_FALSE(executed);
}

TEST_F(FutureMorphismsTest, differentTypeFlatMapNoEffect)
{
    TestPromise<int> promise;
    TestPromise<double> innerPromise;
    auto future = createFuture(promise);
    bool executed = false;
    TestFuture<double> mappedFuture = future.flatMap([innerPromise, &executed](int x) {
        executed = true;
        return innerPromise.future().map([x](double y) { return x / y; });
    });
    promise.failure("42");
    innerPromise.success(2.0);
    mappedFuture.wait(1000);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_FALSE(mappedFuture.isSucceeded());
    EXPECT_TRUE(mappedFuture.isFailed());
    EXPECT_EQ("42", mappedFuture.failureReason());
    EXPECT_FALSE(executed);
}

TEST_F(FutureMorphismsTest, andThen)
{
    TestPromise<int> promise;
    TestPromise<int> innerPromise;
    auto future = createFuture(promise);
    TestFuture<int> mappedFuture = future.andThen([innerPromise]() { return innerPromise.future(); });
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
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> mappedFuture = future.andThenValue(2);
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
    TestPromise<int> promise;
    auto future = createFuture(promise);
    int result = 2;
    TestFuture<int> mappedFuture = future.andThenValue(result);
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
    TestPromise<int> promise;
    auto future = createFuture(promise);
    const int result = 2;
    TestFuture<int> mappedFuture = future.andThenValue(result);
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
    TestPromise<int> promise;
    TestPromise<double> innerPromise;
    auto future = createFuture(promise);
    TestFuture<double> mappedFuture = future.andThen([innerPromise]() { return innerPromise.future(); });
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
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<double> mappedFuture = future.andThenValue(2.0);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(2.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, differentTypeAndThenValueL)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    double result = 2.0;
    TestFuture<double> mappedFuture = future.andThenValue(result);
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
    TestPromise<int> promise;
    auto future = createFuture(promise);
    const double result = 2.0;
    TestFuture<double> mappedFuture = future.andThenValue(result);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(2.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, filterPositive)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> filteredFuture = future.filter([](int x) -> bool { return x % 2; });
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
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> filteredFuture = future.filter([](int x) { return x % 2; });
    EXPECT_FALSE(filteredFuture.isCompleted());
    EXPECT_NE(future, filteredFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(filteredFuture.isCompleted());
    EXPECT_FALSE(filteredFuture.isSucceeded());
    EXPECT_TRUE(filteredFuture.isFailed());
    EXPECT_EQ("Result wasn't good enough", filteredFuture.failureReason());
}

TEST_F(FutureMorphismsTest, filterNegativeCustomRejectedR)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> filteredFuture = future.filter([](int) { return false; }, "Custom");
    EXPECT_FALSE(filteredFuture.isCompleted());
    EXPECT_NE(future, filteredFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(filteredFuture.isCompleted());
    EXPECT_FALSE(filteredFuture.isSucceeded());
    EXPECT_TRUE(filteredFuture.isFailed());
    EXPECT_EQ("Custom", filteredFuture.failureReason());
}

TEST_F(FutureMorphismsTest, filterNegativeCustomRejectedL)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    std::string rejected = "Custom";
    TestFuture<int> filteredFuture = future.filter([](int) { return false; }, rejected);
    EXPECT_FALSE(filteredFuture.isCompleted());
    EXPECT_NE(future, filteredFuture);
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(filteredFuture.isCompleted());
    EXPECT_FALSE(filteredFuture.isSucceeded());
    EXPECT_TRUE(filteredFuture.isFailed());
    EXPECT_EQ("Custom", filteredFuture.failureReason());
}

TEST_F(FutureMorphismsTest, recover)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> recoveredFuture = future.recover([](const auto &) { return 42; });
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_FALSE(recoveredFuture.isFailed());
    EXPECT_EQ(42, recoveredFuture.result());
}

TEST_F(FutureMorphismsTest, recoverNoOp)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> recoveredFuture = future.recover([](const auto &) { return 42; });
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.success(21);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_EQ(21, recoveredFuture.result());
}

TEST_F(FutureMorphismsTest, recoverFromWithTestFailure)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> recoveredFuture = future.recover([](const auto &) { return 42; });
    EXPECT_NE(future, recoveredFuture);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.success(WithTestFailure("failed"));
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_FALSE(recoveredFuture.isFailed());
    EXPECT_EQ(42, recoveredFuture.result());
}

TEST_F(FutureMorphismsTest, recoverAndFail)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> recoveredFuture = future.recover([](const auto &) -> int { return WithTestFailure("failed2"); });
    EXPECT_NE(future, recoveredFuture);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_FALSE(recoveredFuture.isSucceeded());
    EXPECT_TRUE(recoveredFuture.isFailed());
    EXPECT_EQ("failed2", recoveredFuture.failureReason());
    EXPECT_EQ(0, recoveredFuture.result());
}

TEST_F(FutureMorphismsTest, recoverWith)
{
    TestPromise<int> promise;
    TestPromise<int> innerPromise;
    auto future = createFuture(promise);
    TestFuture<int> recoveredFuture = future.recoverWith([innerPromise](const auto &) { return innerPromise.future(); });
    EXPECT_NE(future, recoveredFuture);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());
    EXPECT_FALSE(recoveredFuture.isCompleted());

    innerPromise.success(42);
    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_FALSE(recoveredFuture.isFailed());
    EXPECT_EQ(42, recoveredFuture.result());
}

TEST_F(FutureMorphismsTest, recoverWithNoOp)
{
    TestPromise<int> promise;
    TestPromise<int> innerPromise;
    auto future = createFuture(promise);
    TestFuture<int> recoveredFuture = future.recoverWith([innerPromise](const auto &) { return innerPromise.future(); });
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.success(21);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_EQ(21, recoveredFuture.result());
}

TEST_F(FutureMorphismsTest, recoverWithAndFail)
{
    TestPromise<int> promise;
    TestPromise<int> innerPromise;
    auto future = createFuture(promise);
    TestFuture<int> recoveredFuture = future.recoverWith([innerPromise](const auto &) { return innerPromise.future(); });
    EXPECT_NE(future, recoveredFuture);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());
    EXPECT_FALSE(recoveredFuture.isCompleted());

    innerPromise.failure("failed2");
    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_FALSE(recoveredFuture.isSucceeded());
    EXPECT_EQ("failed2", recoveredFuture.failureReason());
}

TEST_F(FutureMorphismsTest, recoverValue)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> recoveredFuture = future.recoverValue(42);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.failure("failed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_FALSE(recoveredFuture.isFailed());
    EXPECT_EQ(42, recoveredFuture.result());
}

TEST_F(FutureMorphismsTest, recoverValueNoOp)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<int> recoveredFuture = future.recoverValue(42);
    EXPECT_FALSE(future.isCompleted());
    EXPECT_FALSE(recoveredFuture.isCompleted());
    promise.success(21);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());

    ASSERT_TRUE(recoveredFuture.isCompleted());
    EXPECT_TRUE(recoveredFuture.isSucceeded());
    EXPECT_EQ(21, recoveredFuture.result());
}

TEST_F(FutureMorphismsTest, differentTypeMapViaOperator)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<double> mappedFuture = future >> [](int x) -> double { return x / 2.0; };
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(21.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, differentTypeFlatMapViaOperator)
{
    TestPromise<int> promise;
    TestPromise<double> innerPromise;
    auto future = createFuture(promise);
    TestFuture<double> mappedFuture = future >> [innerPromise](int x) {
        return innerPromise.future().map([x](double y) { return x / y; });
    };
    promise.success(42);
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(mappedFuture.isCompleted());
    innerPromise.success(2.0);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(21.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, differentTypeAndThenViaOperator)
{
    TestPromise<int> promise;
    TestPromise<double> innerPromise;
    auto future = createFuture(promise);
    TestFuture<double> mappedFuture = future >> [innerPromise]() { return innerPromise.future(); };
    promise.success(42);
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(mappedFuture.isCompleted());
    innerPromise.success(2.0);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(2.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, multipleMapViaOperator)
{
    TestPromise<int> promise;
    auto future = createFuture(promise);
    TestFuture<double> mappedFuture = future >> [](int x) { return x / 2.0; } >> [](double x) { return x * 3.0; };
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success(42);
    EXPECT_EQ(42, future.result());
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(63.0, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, multipleFlatMapViaOperator)
{
    TestPromise<int> promise;
    TestPromise<double> innerPromise;
    auto future = createFuture(promise);
    TestFuture<int> mappedFuture = future >> [innerPromise](int x) {
        return innerPromise.future().map([x](double y) { return x / y; });
    } >> [](double x) { return TestFuture<int>::successful(static_cast<int>(x) - 1); };
    promise.success(42);
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(mappedFuture.isCompleted());
    innerPromise.success(2.0);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(20, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, multipleAndThenViaOperator)
{
    TestPromise<int> promise;
    TestPromise<double> innerPromise;
    auto future = createFuture(promise);
    TestFuture<int> mappedFuture = future >> [innerPromise] { return innerPromise.future(); } >>
                                   [] { return TestFuture<int>::successful(42); };
    promise.success(42);
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(mappedFuture.isCompleted());
    innerPromise.success(2.0);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_EQ(42, mappedFuture.result());
}

TEST_F(FutureMorphismsTest, mixedMapsViaOperator)
{
    TestPromise<int> promise;
    TestPromise<double> innerPromise;
    auto future = createFuture(promise);
    TestFuture<double> mappedFuture = future >> [this, innerPromise]() { return createFuture(innerPromise); } >>
                                      [](double x) { return TestFuture<int>::successful(static_cast<int>(x) + 5); } >>
                                      [](double x) { return x * 2; };
    promise.success(42);
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(mappedFuture.isCompleted());
    innerPromise.success(2.0);
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    EXPECT_DOUBLE_EQ(14, mappedFuture.result());
}
