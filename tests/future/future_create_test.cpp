#include "futurebasetest.h"

class FutureCreationTest : public FutureBaseTest
{};

TEST_F(FutureCreationTest, successful)
{
    Future<int> future = Future<int>::successful(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureCreationTest, successfulEmpty)
{
    Future<int> future = Future<int>::successful();
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(0, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureCreationTest, successfulNoT)
{
    Future<int> future = Future<>::successful(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureCreationTest, fail)
{
    Future<int> future = Future<int>::failed("isFailed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("isFailed", future.failureReason().toString());
}

TEST_F(FutureCreationTest, success)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    promise.success(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureCreationTest, failure)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    promise.failure("isFailed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("isFailed", future.failureReason().toString());
}
