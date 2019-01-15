#include "futurebasetest.h"

class FutureZipTest : public FutureBaseTest
{};

TEST_F(FutureZipTest, zip)
{
    Promise<int> firstPromise;
    Promise<double> secondPromise;
    Promise<QString> thirdPromise;
    Future<std::tuple<int, double, QString>> future =
        createFuture(firstPromise).zip(createFuture(secondPromise), createFuture(thirdPromise));
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success(5.0);
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(42);
    EXPECT_FALSE(future.isSucceeded());
    thirdPromise.success("Done");

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, std::get<0>(future.result()));
    EXPECT_DOUBLE_EQ(5.0, std::get<1>(future.result()));
    EXPECT_EQ("Done", std::get<2>(future.result()));
}

TEST_F(FutureZipTest, zipLeftTuple)
{
    Promise<std::tuple<int, double>> firstPromise;
    Promise<QString> secondPromise;
    Future<std::tuple<int, double, QString>> future = createFuture(firstPromise).zip(createFuture(secondPromise));
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success("Done");
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(std::make_tuple(42, 5.0));

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, std::get<0>(future.result()));
    EXPECT_DOUBLE_EQ(5.0, std::get<1>(future.result()));
    EXPECT_EQ("Done", std::get<2>(future.result()));
}

TEST_F(FutureZipTest, zipRightTuple)
{
    Promise<std::tuple<int, double>> firstPromise;
    Promise<QString> secondPromise;
    Future<std::tuple<QString, int, double>> future = createFuture(secondPromise).zip(createFuture(firstPromise));
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success("Done");
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(std::make_tuple(42, 5.0));

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ("Done", std::get<0>(future.result()));
    EXPECT_EQ(42, std::get<1>(future.result()));
    EXPECT_DOUBLE_EQ(5.0, std::get<2>(future.result()));
}

TEST_F(FutureZipTest, zipBothTuples)
{
    Promise<std::tuple<int, double>> firstPromise;
    Promise<std::tuple<QString, int>> secondPromise;
    Future<std::tuple<int, double, QString, int>> future = createFuture(firstPromise).zip(createFuture(secondPromise));
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success(std::make_tuple("Done", 1024));
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(std::make_tuple(42, 5.0));

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, std::get<0>(future.result()));
    EXPECT_DOUBLE_EQ(5.0, std::get<1>(future.result()));
    EXPECT_EQ("Done", std::get<2>(future.result()));
    EXPECT_EQ(1024, std::get<3>(future.result()));
}

TEST_F(FutureZipTest, zipLeftFails)
{
    Promise<int> firstPromise;
    Promise<double> secondPromise;
    Future<std::tuple<int, double>> future = createFuture(firstPromise).zip(createFuture(secondPromise));
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success(5.0);
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.failure("failed");

    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason().toString());
}

TEST_F(FutureZipTest, zipRightFails)
{
    Promise<int> firstPromise;
    Promise<double> secondPromise;
    Future<std::tuple<int, double>> future = createFuture(firstPromise).zip(createFuture(secondPromise));
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(42);
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.failure("failed");

    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason().toString());
}

TEST_F(FutureZipTest, zipValue)
{
    Promise<double> firstPromise;
    Future<std::tuple<double, int>> future = createFuture(firstPromise).zipValue(42);
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(5.0);

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_DOUBLE_EQ(5.0, std::get<0>(future.result()));
    EXPECT_EQ(42, std::get<1>(future.result()));
}

TEST_F(FutureZipTest, zipValueLeftTuple)
{
    Promise<std::tuple<int, double>> firstPromise;
    Future<std::tuple<int, double, QString>> future = createFuture(firstPromise).zipValue(QStringLiteral("Done"));
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(std::make_tuple(42, 5.0));

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, std::get<0>(future.result()));
    EXPECT_DOUBLE_EQ(5.0, std::get<1>(future.result()));
    EXPECT_EQ("Done", std::get<2>(future.result()));
}
