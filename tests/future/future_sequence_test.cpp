#include "copycountcontainers.h"
#include "futurebasetest.h"

#include <QDebug>
#include <QLinkedList>
#include <QList>
#include <QVector>

#include <list>
#include <vector>

template <typename, typename>
struct InnerTypeChanger;
template <template <typename...> typename C, typename NewType, typename... Args>
struct InnerTypeChanger<C<Args...>, NewType>
{
    using type = C<NewType>;
};

template <typename Sequence>
class FutureSequenceTest : public FutureBaseTest
{
public:
    using Source = typename InnerTypeChanger<Sequence, Future<int>>::type;
    using ResultFuture = Future<Sequence>;
    using Result = Sequence;
    static inline int N = 100;
};

template <typename Sequence>
class FutureMoveSequenceTest : public FutureBaseTest
{
public:
    using Source = typename InnerTypeChanger<Sequence, Future<int>>::type;
    using ResultFuture = Future<Sequence>;
    using Result = Sequence;
    static inline int N = 100;

protected:
    void SetUp() override
    {
        FutureBaseTest::SetUp();
        Result::copyCounter = 0;
        Source::copyCounter = 0;
        Result::createCounter = 0;
        Source::createCounter = 0;
    }
};

using SequenceTypes = ::testing::Types<QVector<int>, QList<int>, QLinkedList<int>, std::vector<int>, std::list<int>>;
using CopyCountSequenceTypes = ::testing::Types<CopyCountQVector<int>, CopyCountQList<int>, CopyCountQLinkedList<int>,
                                                CopyCountVector<int>, CopyCountList<int>>;

TYPED_TEST_CASE(FutureSequenceTest, SequenceTypes);

TYPED_TEST(FutureSequenceTest, sequence)
{
    QVector<Promise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, Promise<int>());
    typename TestFixture::Source futures = traverse::map(promises, [](const auto &p) { return p.future(); },
                                                         typename TestFixture::Source());
    typename TestFixture::ResultFuture sequencedFuture = Future<int>::sequence(futures);
    int i = 0;

    i = 0;
    for (auto it = futures.cbegin(); it != futures.cend(); ++it, ++i)
        EXPECT_FALSE(it->isCompleted()) << i;

    for (int i = 0; i < TestFixture::N; ++i) {
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
        promises[i].success(i * 2);
    }
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());
    typename TestFixture::Result result = sequencedFuture.result();
    ASSERT_EQ(TestFixture::N, result.size());
    i = 0;
    for (auto it = result.cbegin(); it != result.cend(); ++it, ++i)
        EXPECT_EQ(i * 2, *it) << i;
    ASSERT_EQ(TestFixture::N, i);
}

TYPED_TEST(FutureSequenceTest, sequenceNoT)
{
    QVector<Promise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, Promise<int>());
    typename TestFixture::Source futures = traverse::map(promises, [](const auto &p) { return p.future(); },
                                                         typename TestFixture::Source());
    typename TestFixture::ResultFuture sequencedFuture = Future<>::sequence(futures);
    int i = 0;

    i = 0;
    for (auto it = futures.cbegin(); it != futures.cend(); ++it, ++i)
        EXPECT_FALSE(it->isCompleted()) << i;

    for (int i = 0; i < TestFixture::N; ++i) {
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
        promises[i].success(i * 2);
    }
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());
    typename TestFixture::Result result = sequencedFuture.result();
    ASSERT_EQ(TestFixture::N, result.size());
    i = 0;
    for (auto it = result.cbegin(); it != result.cend(); ++it, ++i)
        EXPECT_EQ(i * 2, *it) << i;
    ASSERT_EQ(TestFixture::N, i);
}

TYPED_TEST(FutureSequenceTest, sequenceNegative)
{
    QVector<Promise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, Promise<int>());
    typename TestFixture::Source futures = traverse::map(promises, [](const auto &p) { return p.future(); },
                                                         typename TestFixture::Source());
    typename TestFixture::ResultFuture sequencedFuture = Future<int>::sequence(futures);
    int i = 0;
    for (auto it = futures.cbegin(); it != futures.cend(); ++it, ++i)
        EXPECT_FALSE(it->isCompleted()) << i;
    EXPECT_FALSE(sequencedFuture.isCompleted());

    for (int i = 0; i < TestFixture::N - 2; ++i) {
        promises[i].success(i * 2);
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
    }
    promises[TestFixture::N - 2].failure("failed");
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_FALSE(sequencedFuture.isSucceeded());
    EXPECT_TRUE(sequencedFuture.isFailed());
    EXPECT_EQ("failed", sequencedFuture.failureReason().toString());
    EXPECT_TRUE(sequencedFuture.result().empty());
}

TYPED_TEST(FutureSequenceTest, sequenceEmpty)
{
    typename TestFixture::Source futures;
    typename TestFixture::ResultFuture sequencedFuture = Future<int>::sequence(futures);
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());
    typename TestFixture::Result result = sequencedFuture.result();
    EXPECT_TRUE(result.empty());
}

TYPED_TEST_CASE(FutureMoveSequenceTest, CopyCountSequenceTypes);

TYPED_TEST(FutureMoveSequenceTest, sequenceMove)
{
    QVector<Promise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, Promise<int>());
    typename TestFixture::ResultFuture sequencedFuture = Future<int>::sequence(
        traverse::map(promises, [](const auto &p) { return p.future(); }, std::move(typename TestFixture::Source())));

    for (int i = 0; i < TestFixture::N; ++i) {
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
        promises[i].success(i * 2);
    }
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());
    const typename TestFixture::Result &result = sequencedFuture.resultRef();
    ASSERT_EQ(TestFixture::N, result.size());
    int i = 0;
    for (auto it = result.cbegin(); it != result.cend(); ++it, ++i)
        EXPECT_EQ(i * 2, *it) << i;
    ASSERT_EQ(TestFixture::N, i);
    EXPECT_EQ(0, TestFixture::Result::copyCounter);
    EXPECT_EQ(0, TestFixture::Source::copyCounter);
    EXPECT_EQ(1, TestFixture::Result::createCounter);
    EXPECT_EQ(1, TestFixture::Source::createCounter);
}

TYPED_TEST(FutureMoveSequenceTest, sequenceMoveNoT)
{
    QVector<Promise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, Promise<int>());
    typename TestFixture::ResultFuture sequencedFuture = Future<>::sequence(
        traverse::map(promises, [](const auto &p) { return p.future(); }, std::move(typename TestFixture::Source())));

    for (int i = 0; i < TestFixture::N; ++i) {
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
        promises[i].success(i * 2);
    }
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());
    const typename TestFixture::Result &result = sequencedFuture.resultRef();
    ASSERT_EQ(TestFixture::N, result.size());
    int i = 0;
    for (auto it = result.cbegin(); it != result.cend(); ++it, ++i)
        EXPECT_EQ(i * 2, *it) << i;
    ASSERT_EQ(TestFixture::N, i);
    EXPECT_EQ(0, TestFixture::Result::copyCounter);
    EXPECT_EQ(0, TestFixture::Source::copyCounter);
    EXPECT_EQ(1, TestFixture::Result::createCounter);
    EXPECT_EQ(1, TestFixture::Source::createCounter);
}

TYPED_TEST(FutureMoveSequenceTest, sequenceNegative)
{
    QVector<Promise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, Promise<int>());

    typename TestFixture::ResultFuture sequencedFuture = Future<int>::sequence(
        traverse::map(promises, [](const auto &p) { return p.future(); }, std::move(typename TestFixture::Source())));

    EXPECT_FALSE(sequencedFuture.isCompleted());
    for (int i = 0; i < TestFixture::N - 2; ++i) {
        promises[i].success(i * 2);
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
    }
    promises[TestFixture::N - 2].failure("failed");
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_FALSE(sequencedFuture.isSucceeded());
    EXPECT_TRUE(sequencedFuture.isFailed());
    EXPECT_EQ("failed", sequencedFuture.failureReason().toString());

    EXPECT_EQ(0, TestFixture::Result::copyCounter);
    EXPECT_EQ(0, TestFixture::Source::copyCounter);
    EXPECT_EQ(1, TestFixture::Result::createCounter);
    EXPECT_EQ(1, TestFixture::Source::createCounter);
}
