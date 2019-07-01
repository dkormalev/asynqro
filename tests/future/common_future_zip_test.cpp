#include "futurebasetest.h"

class FutureZipTest : public FutureBaseTest
{};

TEST_F(FutureZipTest, zip)
{
    TestPromise<int> firstPromise;
    TestPromise<double> secondPromise;
    TestPromise<std::string> thirdPromise;
    TestFuture<std::tuple<int, double, std::string>> future =
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
    TestPromise<std::tuple<int, double>> firstPromise;
    TestPromise<std::string> secondPromise;
    TestFuture<std::tuple<int, double, std::string>> future = createFuture(firstPromise).zip(createFuture(secondPromise));
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
    TestPromise<std::tuple<int, double>> firstPromise;
    TestPromise<std::string> secondPromise;
    TestFuture<std::tuple<std::string, int, double>> future = createFuture(secondPromise).zip(createFuture(firstPromise));
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
    TestPromise<std::tuple<int, double>> firstPromise;
    TestPromise<std::tuple<std::string, int>> secondPromise;
    TestFuture<std::tuple<int, double, std::string, int>> future =
        createFuture(firstPromise).zip(createFuture(secondPromise));
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
    TestPromise<int> firstPromise;
    TestPromise<double> secondPromise;
    TestFuture<std::tuple<int, double>> future = createFuture(firstPromise).zip(createFuture(secondPromise));
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success(5.0);
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.failure("failed");

    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());
}

TEST_F(FutureZipTest, zipRightFails)
{
    TestPromise<int> firstPromise;
    TestPromise<double> secondPromise;
    TestFuture<std::tuple<int, double>> future = createFuture(firstPromise).zip(createFuture(secondPromise));
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(42);
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.failure("failed");

    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("failed", future.failureReason());
}

TEST_F(FutureZipTest, zipValue)
{
    TestPromise<double> firstPromise;
    TestFuture<std::tuple<double, int>> future = createFuture(firstPromise).zipValue(42);
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
    TestPromise<std::tuple<int, double>> firstPromise;
    TestFuture<std::tuple<int, double, std::string>> future = createFuture(firstPromise).zipValue(std::string("Done"));
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(std::make_tuple(42, 5.0));

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, std::get<0>(future.result()));
    EXPECT_DOUBLE_EQ(5.0, std::get<1>(future.result()));
    EXPECT_EQ("Done", std::get<2>(future.result()));
}

TEST_F(FutureZipTest, zipDifferentFailures)
{
    TestPromise<int> firstPromise;
    Promise<double, int> secondPromise;
    Promise<double, double> thirdPromise;
    Future<std::tuple<int, double, double>, std::variant<std::string, int, double>> future =
        createFuture(firstPromise).zip(secondPromise.future()).zip(thirdPromise.future());
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success(5.0);
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(42);
    EXPECT_FALSE(future.isSucceeded());
    thirdPromise.success(2.0);

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, std::get<0>(future.result()));
    EXPECT_DOUBLE_EQ(5.0, std::get<1>(future.result()));
    EXPECT_DOUBLE_EQ(2.0, std::get<2>(future.result()));
}

TEST_F(FutureZipTest, zipDifferentFailuresGrouped)
{
    TestPromise<int> firstPromise;
    Promise<double, int> secondPromise;
    Promise<double, double> thirdPromise;
    Future<std::tuple<int, double, double>, std::variant<std::string, int, double>> future =
        createFuture(firstPromise).zip(secondPromise.future(), thirdPromise.future());
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success(5.0);
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(42);
    EXPECT_FALSE(future.isSucceeded());
    thirdPromise.success(2.0);

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, std::get<0>(future.result()));
    EXPECT_DOUBLE_EQ(5.0, std::get<1>(future.result()));
    EXPECT_DOUBLE_EQ(2.0, std::get<2>(future.result()));
}

TEST_F(FutureZipTest, zipDifferentFailuresFails)
{
    TestPromise<int> firstPromise;
    Promise<double, int> secondPromise;
    Promise<double, double> thirdPromise;
    Future<std::tuple<int, double, double>, std::variant<std::string, int, double>> future =
        createFuture(firstPromise).zip(secondPromise.future()).zip(thirdPromise.future());
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success(5.0);
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(42);
    EXPECT_FALSE(future.isSucceeded());
    thirdPromise.failure(2.0);

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isFailed());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_DOUBLE_EQ(2.0, std::get<double>(future.failureReason()));
}

TEST_F(FutureZipTest, zipDifferentFailuresFailsFirst)
{
    TestPromise<int> firstPromise;
    Promise<double, int> secondPromise;
    Promise<double, double> thirdPromise;
    Future<std::tuple<int, double, double>, std::variant<std::string, int, double>> future =
        createFuture(firstPromise).zip(secondPromise.future()).zip(thirdPromise.future());
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success(5.0);
    EXPECT_FALSE(future.isSucceeded());
    thirdPromise.success(42);
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.failure("failed");

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isFailed());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_EQ("failed", std::get<std::string>(future.failureReason()));
}

TEST_F(FutureZipTest, zipDifferentFailuresFailsFirstQuick)
{
    TestPromise<int> firstPromise;
    Promise<double, int> secondPromise;
    Promise<double, double> thirdPromise;
    Future<std::tuple<int, double, double>, std::variant<std::string, int, double>> future =
        createFuture(firstPromise).zip(secondPromise.future()).zip(thirdPromise.future());
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.failure("failed");

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isFailed());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_EQ("failed", std::get<std::string>(future.failureReason()));
}

TEST_F(FutureZipTest, zipViaOperator)
{
    TestPromise<int> firstPromise;
    TestPromise<double> secondPromise;
    TestPromise<std::string> thirdPromise;
    TestFuture<std::tuple<int, double, std::string>> future = createFuture(firstPromise) + createFuture(secondPromise)
                                                              + createFuture(thirdPromise);
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

TEST_F(FutureZipTest, zipDifferentFailuresViaOperator)
{
    TestPromise<int> firstPromise;
    Promise<double, int> secondPromise;
    Promise<double, double> thirdPromise;
    Future<std::tuple<int, double, double>, std::variant<std::string, int, double>> future = createFuture(firstPromise)
                                                                                             + secondPromise.future()
                                                                                             + thirdPromise.future();
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success(5.0);
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(42);
    EXPECT_FALSE(future.isSucceeded());
    thirdPromise.success(2.0);

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, std::get<0>(future.result()));
    EXPECT_DOUBLE_EQ(5.0, std::get<1>(future.result()));
    EXPECT_DOUBLE_EQ(2.0, std::get<2>(future.result()));
}

TEST_F(FutureZipTest, zipViaOperatorGrouped)
{
    TestPromise<int> firstPromise;
    TestPromise<double> secondPromise;
    TestPromise<std::string> thirdPromise;
    TestFuture<std::tuple<int, double, std::string>> future = createFuture(firstPromise)
                                                              + (createFuture(secondPromise) + createFuture(thirdPromise));
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

TEST_F(FutureZipTest, zipDifferentFailuresViaOperatorGrouped)
{
    TestPromise<int> firstPromise;
    Promise<double, int> secondPromise;
    Promise<double, double> thirdPromise;
    Future<std::tuple<int, double, double>, std::variant<std::string, int, double>> future = createFuture(firstPromise)
                                                                                             + (secondPromise.future()
                                                                                                + thirdPromise.future());
    EXPECT_FALSE(future.isSucceeded());
    secondPromise.success(5.0);
    EXPECT_FALSE(future.isSucceeded());
    firstPromise.success(42);
    EXPECT_FALSE(future.isSucceeded());
    thirdPromise.success(2.0);

    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, std::get<0>(future.result()));
    EXPECT_DOUBLE_EQ(5.0, std::get<1>(future.result()));
    EXPECT_DOUBLE_EQ(2.0, std::get<2>(future.result()));
}

TEST_F(FutureZipTest, zipWithRegularFutureViaOperator)
{
    {
        TestPromise<int> firstPromise;
        Promise<double, std::string> secondPromise;
        TestFuture<std::tuple<int, double>> future = createFuture(firstPromise) + secondPromise.future();
        EXPECT_FALSE(future.isSucceeded());
        secondPromise.success(5.0);
        EXPECT_FALSE(future.isSucceeded());
        firstPromise.success(42);

        ASSERT_TRUE(future.isCompleted());
        EXPECT_TRUE(future.isSucceeded());
        EXPECT_FALSE(future.isFailed());
        EXPECT_EQ(42, std::get<0>(future.result()));
        EXPECT_DOUBLE_EQ(5.0, std::get<1>(future.result()));
    }

    {
        TestPromise<int> firstPromise;
        Promise<double, std::string> secondPromise;
        TestFuture<std::tuple<double, int>> future = secondPromise.future() + createFuture(firstPromise);
        EXPECT_FALSE(future.isSucceeded());
        secondPromise.success(5.0);
        EXPECT_FALSE(future.isSucceeded());
        firstPromise.success(42);

        ASSERT_TRUE(future.isCompleted());
        EXPECT_TRUE(future.isSucceeded());
        EXPECT_FALSE(future.isFailed());
        EXPECT_EQ(42, std::get<1>(future.result()));
        EXPECT_DOUBLE_EQ(5.0, std::get<0>(future.result()));
    }
}
