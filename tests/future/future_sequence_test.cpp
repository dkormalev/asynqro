#include "copycountcontainers.h"
#include "futurebasetest.h"

#ifdef ASYNQRO_QT_SUPPORT
#    include <QLinkedList>
#    include <QList>
#    include <QVector>
#endif

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
    using Source = typename InnerTypeChanger<Sequence, TestFuture<int>>::type;
    using ResultFuture = TestFuture<Sequence>;
    using Result = Sequence;
    static inline int N = 100;
};

template <typename Sequence>
class FutureMoveSequenceTest : public FutureBaseTest
{
public:
    using Source = typename InnerTypeChanger<Sequence, TestFuture<int>>::type;
    using ResultFuture = TestFuture<Sequence>;
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

using SequenceTypes = ::testing::Types<std::vector<int>, std::list<int>
#ifdef ASYNQRO_QT_SUPPORT
                                       ,
                                       QVector<int>, QList<int>, QLinkedList<int>
#endif
                                       >;
using CopyCountSequenceTypes = ::testing::Types<CopyCountVector<int>, CopyCountList<int>
#ifdef ASYNQRO_QT_SUPPORT
                                                ,
                                                CopyCountQVector<int>, CopyCountQList<int>, CopyCountQLinkedList<int>
#endif
                                                >;

TYPED_TEST_SUITE(FutureSequenceTest, SequenceTypes);

TYPED_TEST(FutureSequenceTest, sequence)
{
    std::vector<TestPromise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, TestPromise<int>());
    typename TestFixture::Source futures = traverse::map(promises, [](const auto &p) { return p.future(); },
                                                         typename TestFixture::Source());
    typename TestFixture::ResultFuture sequencedFuture = TestFuture<int>::sequence(futures);
    int i = 0;

    i = 0;
    for (auto it = futures.cbegin(); it != futures.cend(); ++it, ++i)
        EXPECT_FALSE(it->isCompleted()) << i;

    for (size_t i = 0; i < TestFixture::N; ++i) {
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
    std::vector<TestPromise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, TestPromise<int>());
    typename TestFixture::Source futures = traverse::map(promises, [](const auto &p) { return p.future(); },
                                                         typename TestFixture::Source());
    typename TestFixture::ResultFuture sequencedFuture = TestFuture<int>::sequence(futures);
    int i = 0;
    for (auto it = futures.cbegin(); it != futures.cend(); ++it, ++i)
        EXPECT_FALSE(it->isCompleted()) << i;
    EXPECT_FALSE(sequencedFuture.isCompleted());

    for (size_t i = 0; i < TestFixture::N - 2; ++i) {
        promises[i].success(i * 2);
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
    }
    promises[TestFixture::N - 2].failure("failed");
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_FALSE(sequencedFuture.isSucceeded());
    EXPECT_TRUE(sequencedFuture.isFailed());
    EXPECT_EQ("failed", sequencedFuture.failureReason());
    EXPECT_TRUE(sequencedFuture.result().empty());
}

TYPED_TEST(FutureSequenceTest, sequenceEmpty)
{
    typename TestFixture::Source futures;
    typename TestFixture::ResultFuture sequencedFuture = TestFuture<int>::sequence(futures);
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());
    typename TestFixture::Result result = sequencedFuture.result();
    EXPECT_TRUE(result.empty());
}

TYPED_TEST_SUITE(FutureMoveSequenceTest, CopyCountSequenceTypes);

TYPED_TEST(FutureMoveSequenceTest, sequenceMove)
{
    std::vector<TestPromise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, TestPromise<int>());
    typename TestFixture::ResultFuture sequencedFuture = TestFuture<int>::sequence(
        traverse::map(promises, [](const auto &p) { return p.future(); }, std::move(typename TestFixture::Source())));

    for (size_t i = 0; i < TestFixture::N; ++i) {
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
    std::vector<TestPromise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, TestPromise<int>());

    typename TestFixture::ResultFuture sequencedFuture = TestFuture<int>::sequence(
        traverse::map(promises, [](const auto &p) { return p.future(); }, std::move(typename TestFixture::Source())));

    EXPECT_FALSE(sequencedFuture.isCompleted());
    for (size_t i = 0; i < TestFixture::N - 2; ++i) {
        promises[i].success(i * 2);
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
    }
    promises[TestFixture::N - 2].failure("failed");
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_FALSE(sequencedFuture.isSucceeded());
    EXPECT_TRUE(sequencedFuture.isFailed());
    EXPECT_EQ("failed", sequencedFuture.failureReason());

    EXPECT_EQ(0, TestFixture::Result::copyCounter);
    EXPECT_EQ(0, TestFixture::Source::copyCounter);
    EXPECT_EQ(1, TestFixture::Result::createCounter);
    EXPECT_EQ(1, TestFixture::Source::createCounter);
}
