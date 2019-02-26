#include "asynqro/impl/containers_traverse.h"

#include "gtest/gtest.h"

#ifdef ASYNQRO_QT_SUPPORT
#    include <QHash>
#    include <QLinkedList>
#    include <QList>
#    include <QMap>
#    include <QSet>
#    include <QVector>
#endif

#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace asynqro;

namespace {
struct ReduceResult
{
    ReduceResult(int s, int m) : sum(s), mult(m) { ++counter; }
    ReduceResult() : ReduceResult(-1, -1) {}
    ReduceResult(const ReduceResult &other)
    {
        sum = other.sum;
        mult = other.mult;
        ++counter;
    }
    ReduceResult &operator=(const ReduceResult &other)
    {
        sum = other.sum;
        mult = other.mult;
        ++counter;
        return *this;
    }
    ReduceResult(ReduceResult &&other) = default;
    ReduceResult &operator=(ReduceResult &&other) = default;
    int sum;
    int mult;
    static inline int counter = 0;
};

} // namespace

template <typename Container>
class SingleSocketedContainersReduceTest : public testing::Test
{
public:
    using Source = Container;

    static constexpr auto simpleReducer = [](int acc, int x) -> int { return acc * x; };
    static constexpr auto complexReducer = [](ReduceResult acc, int x) {
        acc.sum += x;
        acc.mult *= x;
        return acc;
    };
    static constexpr auto complexRVReducer = [](ReduceResult &&acc, int x) {
        acc.sum += x;
        acc.mult *= x;
        return std::move(acc);
    };

#ifdef ASYNQRO_QT_SUPPORT
    static constexpr auto complexCOWReducer = [](QVector<ReduceResult> acc, int x) {
        acc[0].sum += x;
        acc[0].mult *= x;
        return acc;
    };
#endif
};

template <typename Container>
class DoubleSocketedContainersReduceTest : public testing::Test
{
public:
    using Source = Container;

    static constexpr auto simpleReducer = [](int acc, int x, bool) -> int { return acc * x; };
    static constexpr auto complexReducer = [](ReduceResult acc, int x, bool y) {
        acc.sum += y ? x : 0;
        acc.mult *= x;
        return acc;
    };
    static constexpr auto complexRVReducer = [](ReduceResult &&acc, int x, bool y) {
        acc.sum += y ? x : 0;
        acc.mult *= x;
        return std::move(acc);
    };

#ifdef ASYNQRO_QT_SUPPORT
    static constexpr auto complexCOWReducer = [](QVector<ReduceResult> acc, int x, bool y) {
        acc[0].sum += y ? x : 0;
        acc[0].mult *= x;
        return acc;
    };
#endif
};

using SingleSocketedContainersTypes = ::testing::Types<std::vector<int>, std::set<int>, std::unordered_set<int>,
                                                       std::multiset<int>, std::unordered_multiset<int>, std::list<int>
#ifdef ASYNQRO_QT_SUPPORT
                                                       ,
                                                       QVector<int>, QList<int>, QLinkedList<int>, QSet<int>
#endif
                                                       >;
TYPED_TEST_SUITE(SingleSocketedContainersReduceTest, SingleSocketedContainersTypes);
TYPED_TEST(SingleSocketedContainersReduceTest, reduce)
{
    ReduceResult::counter = 0;
    int n = 100;
    int referenceSum = 0;
    int referenceMult = 1;
    typename TestFixture::Source testContainer;
    for (int i = 1; i <= n; ++i) {
        traverse::detail::containers::add(testContainer, i);
        referenceSum += i;
        referenceMult *= i;
    }
    int result;
    ReduceResult complexResult;
    ASSERT_EQ(1, ReduceResult::counter);

    result = traverse::reduce(testContainer, TestFixture::simpleReducer, 1);
    EXPECT_EQ(referenceMult, result);
    complexResult = traverse::reduce(testContainer, TestFixture::complexReducer, ReduceResult{0, 1});
    EXPECT_EQ(2, ReduceResult::counter);
    EXPECT_EQ(referenceSum, complexResult.sum);
    EXPECT_EQ(referenceMult, complexResult.mult);

    complexResult = traverse::reduce(testContainer, TestFixture::complexRVReducer, ReduceResult{0, 1});
    EXPECT_EQ(3, ReduceResult::counter);
    EXPECT_EQ(referenceSum, complexResult.sum);
    EXPECT_EQ(referenceMult, complexResult.mult);

#ifdef ASYNQRO_QT_SUPPORT
    auto initial = QVector<ReduceResult>();
    initial.append({0, 1});
    QVector<ReduceResult> complexCOWResult = traverse::reduce(testContainer, TestFixture::complexCOWReducer,
                                                              std::move(initial));
    EXPECT_EQ(4, ReduceResult::counter);
    EXPECT_EQ(referenceSum, complexCOWResult.constFirst().sum);
    EXPECT_EQ(referenceMult, complexCOWResult.constFirst().mult);
#endif
}

TYPED_TEST(SingleSocketedContainersReduceTest, reduceEmpty)
{
    ReduceResult::counter = 0;
    typename TestFixture::Source emptyContainer;
    ReduceResult complexResult;
    ASSERT_EQ(1, ReduceResult::counter);

    complexResult = traverse::reduce(emptyContainer, TestFixture::complexReducer, ReduceResult{0, 1});
    EXPECT_EQ(2, ReduceResult::counter);
    EXPECT_EQ(0, complexResult.sum);
    EXPECT_EQ(1, complexResult.mult);

    complexResult = traverse::reduce(emptyContainer, TestFixture::complexRVReducer, ReduceResult{0, 1});
    EXPECT_EQ(3, ReduceResult::counter);
    EXPECT_EQ(0, complexResult.sum);
    EXPECT_EQ(1, complexResult.mult);

#ifdef ASYNQRO_QT_SUPPORT
    auto initial = QVector<ReduceResult>();
    initial.append({0, 1});
    QVector<ReduceResult> complexCOWResult = traverse::reduce(emptyContainer, TestFixture::complexCOWReducer,
                                                              std::move(initial));
    EXPECT_EQ(4, ReduceResult::counter);
    EXPECT_EQ(0, complexCOWResult.constFirst().sum);
    EXPECT_EQ(1, complexCOWResult.constFirst().mult);
#endif
}

using DoubleSocketedContainersTypes = ::testing::Types<
    std::map<int, bool>, std::unordered_map<int, bool>, std::multimap<int, bool>, std::unordered_multimap<int, bool>
#ifdef ASYNQRO_QT_SUPPORT
    ,
    QMap<int, bool>, QMultiMap<int, bool>, QHash<int, bool>, QMultiHash<int, bool>
#endif
    >;
TYPED_TEST_SUITE(DoubleSocketedContainersReduceTest, DoubleSocketedContainersTypes);
TYPED_TEST(DoubleSocketedContainersReduceTest, reduce)
{
    ReduceResult::counter = 0;
    int n = 100;
    int referenceSum = 0;
    int referenceMult = 1;
    typename TestFixture::Source testContainer;
    for (int i = 1; i <= n; ++i) {
        traverse::detail::containers::add(testContainer, std::make_pair(i, (i % 2)));
        referenceSum += (i % 2) ? i : 0;
        referenceMult *= i;
    }
    int result;
    ReduceResult complexResult;
    ASSERT_EQ(1, ReduceResult::counter);

    result = traverse::reduce(testContainer, TestFixture::simpleReducer, 1);
    EXPECT_EQ(referenceMult, result);
    complexResult = traverse::reduce(testContainer, TestFixture::complexReducer, ReduceResult{0, 1});
    EXPECT_EQ(2, ReduceResult::counter);
    EXPECT_EQ(referenceSum, complexResult.sum);
    EXPECT_EQ(referenceMult, complexResult.mult);

    complexResult = traverse::reduce(testContainer, TestFixture::complexRVReducer, ReduceResult{0, 1});
    EXPECT_EQ(3, ReduceResult::counter);
    EXPECT_EQ(referenceSum, complexResult.sum);
    EXPECT_EQ(referenceMult, complexResult.mult);

#ifdef ASYNQRO_QT_SUPPORT
    auto initial = QVector<ReduceResult>();
    initial.append({0, 1});
    QVector<ReduceResult> complexCOWResult = traverse::reduce(testContainer, TestFixture::complexCOWReducer,
                                                              std::move(initial));
    EXPECT_EQ(4, ReduceResult::counter);
    EXPECT_EQ(referenceSum, complexCOWResult.constFirst().sum);
    EXPECT_EQ(referenceMult, complexCOWResult.constFirst().mult);
#endif
}

TYPED_TEST(DoubleSocketedContainersReduceTest, reduceEmpty)
{
    ReduceResult::counter = 0;
    typename TestFixture::Source emptyContainer;
    ReduceResult complexResult;
    ASSERT_EQ(1, ReduceResult::counter);

    complexResult = traverse::reduce(emptyContainer, TestFixture::complexReducer, ReduceResult{0, 1});
    EXPECT_EQ(2, ReduceResult::counter);
    EXPECT_EQ(0, complexResult.sum);
    EXPECT_EQ(1, complexResult.mult);

    complexResult = traverse::reduce(emptyContainer, TestFixture::complexRVReducer, ReduceResult{0, 1});
    EXPECT_EQ(3, ReduceResult::counter);
    EXPECT_EQ(0, complexResult.sum);
    EXPECT_EQ(1, complexResult.mult);

#ifdef ASYNQRO_QT_SUPPORT
    auto initial = QVector<ReduceResult>();
    initial.append({0, 1});
    QVector<ReduceResult> complexCOWResult = traverse::reduce(emptyContainer, TestFixture::complexCOWReducer,
                                                              std::move(initial));
    EXPECT_EQ(4, ReduceResult::counter);
    EXPECT_EQ(0, complexCOWResult.constFirst().sum);
    EXPECT_EQ(1, complexCOWResult.constFirst().mult);
#endif
}
