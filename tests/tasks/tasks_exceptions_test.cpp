#include "tasksbasetest.h"

#include <QSet>
#include <QVector>

class TasksExceptionsTest : public TasksBaseTest
{};

TEST_F(TasksExceptionsTest, singleTaskException)
{
    auto future = run([]() { throw std::runtime_error("Hi"); });
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("Exception caught: Hi", future.failureReason().toString());
}

TEST_F(TasksExceptionsTest, singleTaskExceptionNonStd)
{
    auto future = run([]() { throw 42; });
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("Exception caught", future.failureReason().toString());
}

TEST_F(TasksExceptionsTest, sequenceRunException)
{
    auto future = run(QVector<int>{1, 2, 3}, [](int) { throw std::runtime_error("Hi"); });
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("Exception caught: Hi", future.failureReason().toString());
}

TEST_F(TasksExceptionsTest, sequenceRunExceptionNonStd)
{
    auto future = run(QVector<int>{1, 2, 3}, [](int) { throw 42; });
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("Exception caught", future.failureReason().toString());
}

TEST_F(TasksExceptionsTest, clusteredRunException)
{
    auto future = clusteredRun(QVector<int>{1, 2, 3}, [](int) -> int { throw std::runtime_error("Hi"); });
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("Exception caught: Hi", future.failureReason().toString());
}

TEST_F(TasksExceptionsTest, clusteredRunExceptionNonStd)
{
    auto future = clusteredRun(QVector<int>{1, 2, 3}, [](int) -> int { throw 42; });
    future.wait(10000);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("Exception caught", future.failureReason().toString());
}
