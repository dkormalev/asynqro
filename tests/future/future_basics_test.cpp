#include "futurebasetest.h"

#include <QCoreApplication>
#include <QThread>

class FutureBasicsTest : public FutureBaseTest
{};

TEST_F(FutureBasicsTest, successful)
{
    Future<int> future = Future<int>::successful(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureBasicsTest, successfulEmpty)
{
    Future<int> future = Future<int>::successful();
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(0, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureBasicsTest, successfulNoT)
{
    Future<int> future = Future<>::successful(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureBasicsTest, fail)
{
    Future<int> future = Future<int>::failed("isFailed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("isFailed", future.failureReason().toString());
}

TEST_F(FutureBasicsTest, success)
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

TEST_F(FutureBasicsTest, failure)
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

TEST_F(FutureBasicsTest, successAfterSuccess)
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
    promise.success(24);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
}

TEST_F(FutureBasicsTest, failureAfterFailure)
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
    promise.failure("Again");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("isFailed", future.failureReason().toString());
}

TEST_F(FutureBasicsTest, failureAfterSuccess)
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
    promise.failure("isFailed");
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
}

TEST_F(FutureBasicsTest, successAfterFailure)
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
    promise.success(42);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_FALSE(future.isSucceeded());
    EXPECT_TRUE(future.isFailed());
    EXPECT_TRUE(future.failureReason().isValid());
    EXPECT_EQ("isFailed", future.failureReason().toString());
}

TEST_F(FutureBasicsTest, waitForever)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    QThread::create([promise]() {
        QThread::msleep(500);
        promise.success(42);
    })
        ->start();
    bool result = future.wait();
    EXPECT_TRUE(result);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
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
    Promise<MovableOnly> promise;
    Future<MovableOnly> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    QThread::create([promise]() {
        QThread::msleep(500);
        promise.success(MovableOnly{42});
    })
        ->start();
    bool result = future.wait();
    EXPECT_TRUE(result);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.resultRef().result);
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureBasicsTest, waitForeverInQApp)
{
    int argc = 0;
    char **argv = new char *[0];
    {
        QCoreApplication a(argc, argv);
        Promise<int> promise;
        Future<int> future = promise.future();
        EXPECT_FALSE(future.isCompleted());
        QThread::create([promise]() {
            QThread::msleep(500);
            promise.success(42);
        })
            ->start();
        bool result = future.wait();
        EXPECT_TRUE(result);
        ASSERT_TRUE(future.isCompleted());
        EXPECT_TRUE(future.isSucceeded());
        EXPECT_FALSE(future.isFailed());
        EXPECT_EQ(42, future.result());
        EXPECT_FALSE(future.failureReason().isValid());
    }
    delete[] argv;
}

TEST_F(FutureBasicsTest, waitTimedPositive)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    QThread::create([promise]() {
        QThread::msleep(500);
        promise.success(42);
    })
        ->start();
    bool result = future.wait(30000);
    EXPECT_TRUE(result);
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureBasicsTest, waitTimedNegative)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    bool result = future.wait(500);
    EXPECT_FALSE(result);
    EXPECT_FALSE(future.isCompleted());
}

TEST_F(FutureBasicsTest, waitInResult)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    QThread::create([promise]() {
        QThread::msleep(500);
        promise.success(42);
    })
        ->start();
    future.result();
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureBasicsTest, waitInResultRef)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    QThread::create([promise]() {
        QThread::msleep(500);
        promise.success(42);
    })
        ->start();
    future.resultRef();
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureBasicsTest, waitInFailureReason)
{
    Promise<int> promise;
    Future<int> future = promise.future();
    EXPECT_FALSE(future.isCompleted());
    QThread::create([promise]() {
        QThread::msleep(500);
        promise.success(42);
    })
        ->start();
    future.failureReason();
    ASSERT_TRUE(future.isCompleted());
    EXPECT_TRUE(future.isSucceeded());
    EXPECT_FALSE(future.isFailed());
    EXPECT_EQ(42, future.result());
    EXPECT_FALSE(future.failureReason().isValid());
}

TEST_F(FutureBasicsTest, isValid)
{
    Future<int> invalid;
    EXPECT_FALSE(invalid.isValid());
    Promise<int> promise1;
    Future<int> validFromPromise = promise1.future();
    EXPECT_TRUE(validFromPromise.isValid());
    Promise<int> promise2;
    Future<int> validPromiseCtor(promise2);
    EXPECT_TRUE(validFromPromise.isValid());
}
