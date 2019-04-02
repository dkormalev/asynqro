#include "futurebasetest.h"

#ifdef ASYNQRO_QT_SUPPORT
#    include <QCoreApplication>

class TestQObject : public QObject
{
    Q_OBJECT
signals:
    void testSignalNoParams();
    void testSignalSingleParam(int x);
    void testSignalMultiParams(int x, double y);
    void testSignalTupleParam(std::tuple<int, double> x);
};
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

#ifdef ASYNQRO_QT_SUPPORT
TEST_F(FutureBasicsTest, fromQtSignalEmpty)
{
    std::unique_ptr<TestQObject> obj(new TestQObject);
    TestFuture<bool> f = TestFuture<bool>::fromQtSignal(obj.get(), &TestQObject::testSignalNoParams);
    ASSERT_FALSE(f.isCompleted());
    emit obj->testSignalNoParams();
    ASSERT_TRUE(f.isCompleted());
    EXPECT_TRUE(f.isSucceeded());
    EXPECT_FALSE(f.isFailed());
    EXPECT_TRUE(f.result());
}

TEST_F(FutureBasicsTest, fromQtSignalSingleParam)
{
    std::unique_ptr<TestQObject> obj(new TestQObject);
    TestFuture<int> f = TestFuture<int>::fromQtSignal(obj.get(), &TestQObject::testSignalSingleParam);
    ASSERT_FALSE(f.isCompleted());
    emit obj->testSignalSingleParam(42);
    ASSERT_TRUE(f.isCompleted());
    EXPECT_TRUE(f.isSucceeded());
    EXPECT_FALSE(f.isFailed());
    EXPECT_EQ(42, f.result());
}

TEST_F(FutureBasicsTest, fromQtSignalMultiParams)
{
    std::unique_ptr<TestQObject> obj(new TestQObject);
    TestFuture<std::tuple<int, double>> f =
        TestFuture<std::tuple<int, double>>::fromQtSignal(obj.get(), &TestQObject::testSignalMultiParams);
    ASSERT_FALSE(f.isCompleted());
    emit obj->testSignalMultiParams(42, 5.5);
    ASSERT_TRUE(f.isCompleted());
    EXPECT_TRUE(f.isSucceeded());
    EXPECT_FALSE(f.isFailed());
    EXPECT_EQ(42, std::get<0>(f.result()));
    EXPECT_DOUBLE_EQ(5.5, std::get<1>(f.result()));
}

TEST_F(FutureBasicsTest, fromQtSignalSingleParamTupled)
{
    std::unique_ptr<TestQObject> obj(new TestQObject);
    TestFuture<std::tuple<int>> f = TestFuture<std::tuple<int>>::fromQtSignal(obj.get(),
                                                                              &TestQObject::testSignalSingleParam);
    ASSERT_FALSE(f.isCompleted());
    emit obj->testSignalSingleParam(42);
    ASSERT_TRUE(f.isCompleted());
    EXPECT_TRUE(f.isSucceeded());
    EXPECT_FALSE(f.isFailed());
    EXPECT_EQ(42, std::get<0>(f.result()));
}

TEST_F(FutureBasicsTest, fromQtSignalSingleParamBooled)
{
    std::unique_ptr<TestQObject> obj(new TestQObject);
    TestFuture<bool> f = TestFuture<bool>::fromQtSignal(obj.get(), &TestQObject::testSignalSingleParam);
    ASSERT_FALSE(f.isCompleted());
    emit obj->testSignalSingleParam(42);
    ASSERT_TRUE(f.isCompleted());
    EXPECT_TRUE(f.isSucceeded());
    EXPECT_FALSE(f.isFailed());
    EXPECT_TRUE(f.result());
}

TEST_F(FutureBasicsTest, fromQtSignalTupleParam)
{
    std::unique_ptr<TestQObject> obj(new TestQObject);
    TestFuture<std::tuple<int, double>> f =
        TestFuture<std::tuple<int, double>>::fromQtSignal(obj.get(), &TestQObject::testSignalTupleParam);
    ASSERT_FALSE(f.isCompleted());
    emit obj->testSignalTupleParam(std::make_tuple(42, 5.5));
    ASSERT_TRUE(f.isCompleted());
    EXPECT_TRUE(f.isSucceeded());
    EXPECT_FALSE(f.isFailed());
    EXPECT_EQ(42, std::get<0>(f.result()));
    EXPECT_DOUBLE_EQ(5.5, std::get<1>(f.result()));
}

TEST_F(FutureBasicsTest, fromQtSignalDestroyed)
{
    TestFuture<bool> f;
    {
        std::unique_ptr<TestQObject> obj(new TestQObject);
        f = TestFuture<bool>::fromQtSignal(obj.get(), &TestQObject::testSignalNoParams);
        ASSERT_FALSE(f.isCompleted());
    }
    ASSERT_TRUE(f.isCompleted());
    EXPECT_FALSE(f.isSucceeded());
    EXPECT_TRUE(f.isFailed());
    EXPECT_EQ("Destroyed", f.failureReason());
}
#endif

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
    TestFuture<int> invalid2 = TestFuture<int>();
    EXPECT_FALSE(invalid2.isValid());
    TestPromise<int> promise1;
    TestFuture<int> validFromPromise = promise1.future();
    EXPECT_TRUE(validFromPromise.isValid());
    TestPromise<int> promise2;
    TestFuture<int> validPromiseCtor(promise2);
    EXPECT_TRUE(validFromPromise.isValid());
}

TEST_F(FutureBasicsTest, castFailureFromSingleType)
{
    Promise<int, int> promise;
    Future<int, int> future = promise.future();
    Future<int, std::variant<int, double>> first = future;
    Future<int, std::variant<double, int>> second = future;
    promise.failure(42);
    EXPECT_EQ(42, std::get<int>(first.failureReason()));
    EXPECT_EQ(42, std::get<int>(second.failureReason()));
}

TEST_F(FutureBasicsTest, castFailureFromVariant)
{
    Promise<int, std::variant<int, std::string>> promise;
    Future<int, std::variant<int, std::string>> future = promise.future();
    Future<int, std::variant<int, double, std::string>> first = future;
    Future<int, std::variant<double, std::string, int>> second = future;
    Future<int, std::variant<std::string, double, int>> third = future;
    Future<int, std::variant<std::string, int>> fourth = future;
    promise.failure("abc");
    EXPECT_EQ("abc", std::get<std::string>(first.failureReason()));
    EXPECT_EQ("abc", std::get<std::string>(second.failureReason()));
    EXPECT_EQ("abc", std::get<std::string>(third.failureReason()));
    EXPECT_EQ("abc", std::get<std::string>(fourth.failureReason()));
}

#ifdef ASYNQRO_QT_SUPPORT
#    include "future_basics_test.moc"
#endif
