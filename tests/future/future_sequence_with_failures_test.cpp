#include "copycountcontainers.h"
#include "futurebasetest.h"

#ifdef ASYNQRO_QT_SUPPORT
#    include <QLinkedList>
#    include <QList>
#    include <QVector>
#endif

#include <list>
#include <vector>

using namespace std::chrono_literals;

template <typename, typename>
struct InnerTypeChanger;
template <template <typename...> typename C, typename NewType, typename... Args>
struct InnerTypeChanger<C<Args...>, NewType>
{
    using type = C<NewType>;
};

template <typename Sequence>
class FutureSequenceWithFailuresTest : public FutureBaseTest
{
public:
    using Source = typename InnerTypeChanger<Sequence, TestFuture<int>>::type;
    static inline int N = 100;
};

template <typename Sequence>
class FutureMoveSequenceWithFailuresTest : public FutureBaseTest
{
public:
    using Source = typename InnerTypeChanger<Sequence, TestFuture<int>>::type;
    static inline int N = 100;

protected:
    void SetUp() override
    {
        FutureBaseTest::SetUp();
        Source::copyCounter = 0;
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

TYPED_TEST_SUITE(FutureSequenceWithFailuresTest, SequenceTypes);

template <typename TestFixture, template <typename...> typename Container, typename Func>
void sequenceHelper(Func &&sequencer)
{
    std::vector<TestPromise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, TestPromise<int>());
    typename TestFixture::Source futures = traverse::map(promises, [](const auto &p) { return p.future(); },
                                                         typename TestFixture::Source());
    auto sequencedFuture = sequencer(futures);
    int i = 0;
    for (auto it = futures.cbegin(); it != futures.cend(); ++it, ++i)
        EXPECT_FALSE(it->isCompleted()) << i;

    for (size_t i = 0; i < TestFixture::N; ++i) {
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
        promises[i].success(i * 2);
    }
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());

    auto result = sequencedFuture.result().first;
    ASSERT_EQ(TestFixture::N, result.size());
    traverse::map(result,
                  [](auto index, auto value) {
                      EXPECT_EQ(index * 2, value) << index;
                      return true;
                  },
                  std::set<bool>());

    auto failures = sequencedFuture.result().second;
    ASSERT_TRUE(failures.empty());
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(result)>, Container>);
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(failures)>, Container>);
}

TYPED_TEST(FutureSequenceWithFailuresTest, sequence)
{
    sequenceHelper<TestFixture, std::unordered_map>(
        [](const typename TestFixture::Source &futures) { return TestFuture<int>::sequenceWithFailures(futures); });
}

TYPED_TEST(FutureSequenceWithFailuresTest, sequenceMap)
{
    sequenceHelper<TestFixture, std::map>([](const typename TestFixture::Source &futures) {
        return TestFuture<int>::sequenceWithFailures<std::map>(futures);
    });
}

#ifdef ASYNQRO_QT_SUPPORT
TYPED_TEST(FutureSequenceWithFailuresTest, sequenceQMap)
{
    sequenceHelper<TestFixture, QMap>([](const typename TestFixture::Source &futures) {
        return TestFuture<int>::sequenceWithFailures<QMap>(futures);
    });
}

TYPED_TEST(FutureSequenceWithFailuresTest, sequenceQHash)
{
    sequenceHelper<TestFixture, QHash>([](const typename TestFixture::Source &futures) {
        return TestFuture<int>::sequenceWithFailures<QHash>(futures);
    });
}
#endif

template <typename TestFixture, template <typename...> typename Container, typename Func>
void sequenceNegativeHelper(Func &&sequencer)
{
    std::vector<TestPromise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, TestPromise<int>());
    typename TestFixture::Source futures = traverse::map(promises, [](const auto &p) { return p.future(); },
                                                         typename TestFixture::Source());
    auto sequencedFuture = sequencer(futures);
    int i = 0;
    for (auto it = futures.cbegin(); it != futures.cend(); ++it, ++i)
        EXPECT_FALSE(it->isCompleted()) << i;

    for (size_t i = 0; i < TestFixture::N; ++i) {
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
        if (i == TestFixture::N - 2 || i == TestFixture::N - 4)
            promises[i].failure("failed");
        else
            promises[i].success(i * 2);
    }
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());

    auto result = sequencedFuture.result().first;
    ASSERT_EQ(TestFixture::N - 2, result.size());
    ASSERT_FALSE(result.count(TestFixture::N - 2));
    ASSERT_FALSE(result.count(TestFixture::N - 4));

    traverse::map(result,
                  [](auto index, auto value) {
                      EXPECT_EQ(index * 2, value) << index;
                      return true;
                  },
                  std::set<bool>());

    auto failures = sequencedFuture.result().second;
    ASSERT_EQ(2, failures.size());
    ASSERT_TRUE(failures.count(TestFixture::N - 4));
    EXPECT_EQ("failed", failures[TestFixture::N - 4]);
    ASSERT_TRUE(failures.count(TestFixture::N - 2));
    EXPECT_EQ("failed", failures[TestFixture::N - 2]);
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(result)>, Container>);
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(failures)>, Container>);
}

TYPED_TEST(FutureSequenceWithFailuresTest, sequenceNegative)
{
    sequenceNegativeHelper<TestFixture, std::unordered_map>(
        [](const typename TestFixture::Source &futures) { return TestFuture<int>::sequenceWithFailures(futures); });
}

TYPED_TEST(FutureSequenceWithFailuresTest, sequenceNegativeMap)
{
    sequenceNegativeHelper<TestFixture, std::map>([](const typename TestFixture::Source &futures) {
        return TestFuture<int>::sequenceWithFailures<std::map>(futures);
    });
}

#ifdef ASYNQRO_QT_SUPPORT
TYPED_TEST(FutureSequenceWithFailuresTest, sequenceNegativeQMap)
{
    sequenceNegativeHelper<TestFixture, QMap>([](const typename TestFixture::Source &futures) {
        return TestFuture<int>::sequenceWithFailures<QMap>(futures);
    });
}

TYPED_TEST(FutureSequenceWithFailuresTest, sequenceNegativeQHash)
{
    sequenceNegativeHelper<TestFixture, QHash>([](const typename TestFixture::Source &futures) {
        return TestFuture<int>::sequenceWithFailures<QHash>(futures);
    });
}
#endif

template <typename TestFixture, template <typename...> typename Container, typename Func>
void sequenceAllNegativeHelper(Func &&sequencer)
{
    std::vector<TestPromise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, TestPromise<int>());
    typename TestFixture::Source futures = traverse::map(promises, [](const auto &p) { return p.future(); },
                                                         typename TestFixture::Source());
    auto sequencedFuture = sequencer(futures);
    int i = 0;
    for (auto it = futures.cbegin(); it != futures.cend(); ++it, ++i)
        EXPECT_FALSE(it->isCompleted()) << i;

    for (size_t i = 0; i < TestFixture::N; ++i) {
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
        if (i == TestFixture::N - 2)
            promises[i].failure("other");
        else
            promises[i].failure("failed");
    }
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());

    auto result = sequencedFuture.result().first;
    ASSERT_TRUE(result.empty());

    auto failures = sequencedFuture.result().second;
    ASSERT_EQ(TestFixture::N, failures.size());

    traverse::map(failures,
                  [](auto index, const std::string &value) {
                      if (index == TestFixture::N - 2)
                          EXPECT_EQ("other", value) << index;
                      else
                          EXPECT_EQ("failed", value) << index;
                      return true;
                  },
                  std::set<bool>());
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(result)>, Container>);
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(failures)>, Container>);
}

TYPED_TEST(FutureSequenceWithFailuresTest, sequenceAllNegative)
{
    sequenceAllNegativeHelper<TestFixture, std::unordered_map>(
        [](const typename TestFixture::Source &futures) { return TestFuture<int>::sequenceWithFailures(futures); });
}

TYPED_TEST(FutureSequenceWithFailuresTest, sequenceAllNegativeMap)
{
    sequenceAllNegativeHelper<TestFixture, std::map>([](const typename TestFixture::Source &futures) {
        return TestFuture<int>::sequenceWithFailures<std::map>(futures);
    });
}

#ifdef ASYNQRO_QT_SUPPORT
TYPED_TEST(FutureSequenceWithFailuresTest, sequenceAllNegativeQMap)
{
    sequenceAllNegativeHelper<TestFixture, QMap>([](const typename TestFixture::Source &futures) {
        return TestFuture<int>::sequenceWithFailures<QMap>(futures);
    });
}

TYPED_TEST(FutureSequenceWithFailuresTest, sequenceAllNegativeQHash)
{
    sequenceAllNegativeHelper<TestFixture, QHash>([](const typename TestFixture::Source &futures) {
        return TestFuture<int>::sequenceWithFailures<QHash>(futures);
    });
}
#endif

TYPED_TEST(FutureSequenceWithFailuresTest, sequenceEmpty)
{
    typename TestFixture::Source futures;
    auto sequencedFuture = TestFuture<int>::sequenceWithFailures(futures);
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());
    auto result = sequencedFuture.result();
    EXPECT_TRUE(result.first.empty());
    EXPECT_TRUE(result.second.empty());
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(result.first)>, std::unordered_map>);
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(result.second)>, std::unordered_map>);
}

TYPED_TEST_SUITE(FutureMoveSequenceWithFailuresTest, CopyCountSequenceTypes);

template <typename TestFixture, template <typename...> typename Container, typename Func>
void sequenceMoveHelper(Func &&sequencer)
{
    using Result = std::decay_t<decltype(sequencer(typename TestFixture::Source()).resultRef().first)>;
    using FailuresResult = std::decay_t<decltype(sequencer(typename TestFixture::Source()).resultRef().first)>;
    Result::copyCounter = 0;
    Result::createCounter = 0;
    FailuresResult::copyCounter = 0;
    FailuresResult::createCounter = 0;

    std::vector<TestPromise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, TestPromise<int>());
    auto sequencedFuture = sequencer(
        traverse::map(promises, [](const auto &p) { return p.future(); }, std::move(typename TestFixture::Source())));

    for (size_t i = 0; i < TestFixture::N; ++i) {
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
        promises[i].success(i * 2);
    }
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());

    const auto &result = sequencedFuture.resultRef().first;
    ASSERT_EQ(TestFixture::N, result.size());
    traverse::map(result,
                  [](auto index, auto value) {
                      EXPECT_EQ(index * 2, value) << index;
                      return true;
                  },
                  std::set<bool>());

    const auto &failures = sequencedFuture.resultRef().second;
    ASSERT_TRUE(failures.empty());

    EXPECT_EQ(0, Result::copyCounter);
    EXPECT_EQ(1, Result::createCounter);
    EXPECT_EQ(0, FailuresResult::copyCounter);
    EXPECT_EQ(1, FailuresResult::createCounter);
    EXPECT_EQ(0, TestFixture::Source::copyCounter);
    EXPECT_EQ(1, TestFixture::Source::createCounter);
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(result)>, Container>);
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(failures)>, Container>);
}

TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequence)
{
    sequenceMoveHelper<TestFixture, CopyCountUnorderedMap>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountUnorderedMap>(std::move(futures));
    });
}

TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequenceMap)
{
    sequenceMoveHelper<TestFixture, CopyCountMap>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountMap>(std::move(futures));
    });
}

#ifdef ASYNQRO_QT_SUPPORT
TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequenceQMap)
{
    sequenceMoveHelper<TestFixture, CopyCountQMap>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountQMap>(std::move(futures));
    });
}

TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequenceQHash)
{
    sequenceMoveHelper<TestFixture, CopyCountQHash>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountQHash>(std::move(futures));
    });
}
#endif

template <typename TestFixture, template <typename...> typename Container, typename Func>
void sequenceNegativeMoveHelper(Func &&sequencer)
{
    using Result = std::decay_t<decltype(sequencer(typename TestFixture::Source()).resultRef().first)>;
    using FailuresResult = std::decay_t<decltype(sequencer(typename TestFixture::Source()).resultRef().first)>;
    Result::copyCounter = 0;
    Result::createCounter = 0;
    FailuresResult::copyCounter = 0;
    FailuresResult::createCounter = 0;

    std::vector<TestPromise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, TestPromise<int>());
    auto sequencedFuture = sequencer(
        traverse::map(promises, [](const auto &p) { return p.future(); }, std::move(typename TestFixture::Source())));

    for (size_t i = 0; i < TestFixture::N; ++i) {
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
        if (i == TestFixture::N - 2 || i == TestFixture::N - 4)
            promises[i].failure("failed");
        else
            promises[i].success(i * 2);
    }
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());

    const auto &result = sequencedFuture.resultRef().first;
    ASSERT_EQ(TestFixture::N - 2, result.size());
    ASSERT_FALSE(result.count(TestFixture::N - 2));
    ASSERT_FALSE(result.count(TestFixture::N - 4));
    traverse::map(result,
                  [](auto index, auto value) {
                      EXPECT_EQ(index * 2, value) << index;
                      return true;
                  },
                  std::set<bool>());

    const auto &failures = sequencedFuture.resultRef().second;
    ASSERT_EQ(2, failures.size());
    ASSERT_TRUE(failures.count(TestFixture::N - 4));
    EXPECT_EQ("failed", failures[TestFixture::N - 4]);
    ASSERT_TRUE(failures.count(TestFixture::N - 2));
    EXPECT_EQ("failed", failures[TestFixture::N - 2]);

    EXPECT_EQ(0, Result::copyCounter);
    EXPECT_EQ(1, Result::createCounter);
    EXPECT_EQ(0, FailuresResult::copyCounter);
    EXPECT_EQ(1, FailuresResult::createCounter);
    EXPECT_EQ(0, TestFixture::Source::copyCounter);
    EXPECT_EQ(1, TestFixture::Source::createCounter);
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(result)>, Container>);
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(failures)>, Container>);
}

TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequenceNegative)
{
    sequenceNegativeMoveHelper<TestFixture, CopyCountUnorderedMap>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountUnorderedMap>(std::move(futures));
    });
}

TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequenceNegativeMap)
{
    sequenceNegativeMoveHelper<TestFixture, CopyCountMap>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountMap>(std::move(futures));
    });
}

#ifdef ASYNQRO_QT_SUPPORT
TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequenceNegativeQMap)
{
    sequenceNegativeMoveHelper<TestFixture, CopyCountQMap>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountQMap>(std::move(futures));
    });
}

TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequenceNegativeQHash)
{
    sequenceNegativeMoveHelper<TestFixture, CopyCountQHash>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountQHash>(std::move(futures));
    });
}
#endif

template <typename TestFixture, template <typename...> typename Container, typename Func>
void sequenceAllNegativeMoveHelper(Func &&sequencer)
{
    using Result = std::decay_t<decltype(sequencer(typename TestFixture::Source()).resultRef().first)>;
    using FailuresResult = std::decay_t<decltype(sequencer(typename TestFixture::Source()).resultRef().first)>;
    Result::copyCounter = 0;
    Result::createCounter = 0;
    FailuresResult::copyCounter = 0;
    FailuresResult::createCounter = 0;

    std::vector<TestPromise<int>> promises;
    for (int i = 0; i < TestFixture::N; ++i)
        asynqro::traverse::detail::containers::add(promises, TestPromise<int>());
    auto sequencedFuture = sequencer(
        traverse::map(promises, [](const auto &p) { return p.future(); }, std::move(typename TestFixture::Source())));

    for (size_t i = 0; i < TestFixture::N; ++i) {
        EXPECT_FALSE(sequencedFuture.isCompleted()) << i;
        if (i == TestFixture::N - 2)
            promises[i].failure("other");
        else
            promises[i].failure("failed");
    }
    ASSERT_TRUE(sequencedFuture.isCompleted());
    EXPECT_TRUE(sequencedFuture.isSucceeded());
    EXPECT_FALSE(sequencedFuture.isFailed());

    const auto &result = sequencedFuture.resultRef().first;
    ASSERT_TRUE(result.empty());

    const auto &failures = sequencedFuture.resultRef().second;
    ASSERT_EQ(TestFixture::N, failures.size());
    traverse::map(failures,
                  [](auto index, const std::string &value) {
                      if (index == TestFixture::N - 2)
                          EXPECT_EQ("other", value) << index;
                      else
                          EXPECT_EQ("failed", value) << index;
                      return true;
                  },
                  std::set<bool>());

    EXPECT_EQ(0, Result::copyCounter);
    EXPECT_EQ(1, Result::createCounter);
    EXPECT_EQ(0, FailuresResult::copyCounter);
    EXPECT_EQ(1, FailuresResult::createCounter);
    EXPECT_EQ(0, TestFixture::Source::copyCounter);
    EXPECT_EQ(1, TestFixture::Source::createCounter);
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(result)>, Container>);
    static_assert(detail::IsSpecialization_V<std::decay_t<decltype(failures)>, Container>);
}

TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequenceAllNegative)
{
    sequenceAllNegativeMoveHelper<TestFixture, CopyCountUnorderedMap>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountUnorderedMap>(std::move(futures));
    });
}

TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequenceAllNegativeMap)
{
    sequenceAllNegativeMoveHelper<TestFixture, CopyCountMap>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountMap>(std::move(futures));
    });
}

#ifdef ASYNQRO_QT_SUPPORT
TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequenceAllNegativeQMap)
{
    sequenceAllNegativeMoveHelper<TestFixture, CopyCountQMap>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountQMap>(std::move(futures));
    });
}

TYPED_TEST(FutureMoveSequenceWithFailuresTest, sequenceAllNegativeQHash)
{
    sequenceAllNegativeMoveHelper<TestFixture, CopyCountQHash>([](typename TestFixture::Source &&futures) {
        return TestFuture<int>::sequenceWithFailures<CopyCountQHash>(std::move(futures));
    });
}
#endif
