#include "futurebasetest.h"

#ifdef ASYNQRO_QT_SUPPORT
#    include <QCoreApplication>
#endif

class FutureBasicsTest : public FutureBaseTest
{};

TEST_F(FutureBasicsTest, successful)
{
    TestFuture<int> future = TestFuture<int>::successful(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_TRUE(future.failureReason().empty());
}

TEST_F(FutureBasicsTest, successfulRef)
{
    int value = 42;
    TestFuture<int> future = TestFuture<int>::successful(value);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_TRUE(future.failureReason().empty());
}

TEST_F(FutureBasicsTest, successfulEmpty)
{
    TestFuture<int> future = TestFuture<int>::successful();
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(0, future.result());
    EXPECT_TRUE(future.failureReason().empty());
}

TEST_F(FutureBasicsTest, fail)
{
    TestFuture<int> future = TestFuture<int>::failed("isFailed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("isFailed", future.failureReason());
}

TEST_F(FutureBasicsTest, success)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    promise.success(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_TRUE(future.failureReason().empty());
}

TEST_F(FutureBasicsTest, failure)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    promise.failure("isFailed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("isFailed", future.failureReason());
}

TEST_F(FutureBasicsTest, successAfterSuccess)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    promise.success(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_TRUE(future.failureReason().empty());
    promise.success(24);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
}

TEST_F(FutureBasicsTest, failureAfterFailure)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    promise.failure("isFailed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("isFailed", future.failureReason());
    promise.failure("Again");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("isFailed", future.failureReason());
}

TEST_F(FutureBasicsTest, failureAfterSuccess)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    promise.success(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_TRUE(future.failureReason().empty());
    promise.failure("isFailed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
}

TEST_F(FutureBasicsTest, successAfterFailure)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    promise.failure("isFailed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("isFailed", future.failureReason());
    promise.success(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_EQ("isFailed", future.failureReason());
}

TEST_F(FutureBasicsTest, waitForever)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    std::thread([promise]() {
        std::this_thread::sleep_for(500ms);
        promise.success(42);
    })
        .detach();
    bool result = future.wait();
    EXPECT_TRUE(result);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_TRUE(future.failureReason().empty());
}

TEST_F(FutureBasicsTest, waitForeverForMovableOnly)
{
    struct MovableOnly
    {
        MovableOnly() = default;
        MovableOnly(const MovableOnly &) = delete;
        MovableOnly(MovableOnly &&) = default;
        MovableOnly &operator=(const MovableOnly &) = delete;
        MovableOnly &operator=(MovableOnly &&) = default;
        int result = 0;
    };
    TestPromise<MovableOnly> promise;
    TestFuture<MovableOnly> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    std::thread([promise]() {
        std::this_thread::sleep_for(500ms);
        promise.success(MovableOnly{42});
    })
        .detach();
    bool result = future.wait();
    EXPECT_TRUE(result);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.resultRef().result);
    EXPECT_TRUE(future.failureReason().empty());
}

#ifdef ASYNQRO_QT_SUPPORT
TEST_F(FutureBasicsTest, waitForeverInQApp)
{
    int argc = 0;
    char **argv = new char *[0];
    {
        QCoreApplication a(argc, argv);
        TestPromise<int> promise;
        TestFuture<int> future = promise.future();
        EXPECT_FALSE(future.isCompleted());
        std::thread([promise]() {
            std::this_thread::sleep_for(500ms);
            promise.success(42);
        })
            .detach();
        bool result = future.wait();
        EXPECT_TRUE(result);
        ASSERT_TRUE(future.isCompleted());
        EXPECT_TRUE(future.isSucceeded());
        EXPECT_FALSE(future.isFailed());
        EXPECT_EQ(42, future.result());
        EXPECT_TRUE(future.failureReason().empty());
    }
    delete[] argv;
}
#endif

TEST_F(FutureBasicsTest, waitTimedPositive)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    std::thread([promise]() {
        std::this_thread::sleep_for(500ms);
        promise.success(42);
    })
        .detach();
    bool result = future.wait(30000);
    EXPECT_TRUE(result);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_TRUE(future.failureReason().empty());
}

TEST_F(FutureBasicsTest, waitTimedNegative)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    bool result = future.wait(500);
    EXPECT_FALSE(result);
    EXPECT_FALSE(future.isCompleted());
}

TEST_F(FutureBasicsTest, waitInResult)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    std::thread([promise]() {
        std::this_thread::sleep_for(500ms);
        promise.success(42);
    })
        .detach();
    future.result();
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_TRUE(future.failureReason().empty());
}

TEST_F(FutureBasicsTest, waitInResultRef)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    std::thread([promise]() {
        std::this_thread::sleep_for(500ms);
        promise.success(42);
    })
        .detach();
    future.resultRef();
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_TRUE(future.failureReason().empty());
}

TEST_F(FutureBasicsTest, waitInFailureReason)
{
    TestPromise<int> promise;
    TestFuture<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    std::thread([promise]() {
        std::this_thread::sleep_for(500ms);
        promise.success(42);
    })
        .detach();
    future.failureReason();
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_TRUE(future.failureReason().empty());
}

TEST_F(FutureBasicsTest, isValid)
{
    TestFuture<int> invalid;
    EXPECT_FALSE(invalid.isValid());
    TestPromise<int> promise1;
    TestFuture<int> validFromPromise = promise1.future();
    EXPECT_TRUE(validFromPromise.isValid());
    TestPromise<int> promise2;
    TestFuture<int> validPromiseCtor(promise2);
    EXPECT_TRUE(validFromPromise.isValid());
}
