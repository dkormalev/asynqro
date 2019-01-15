#include "asynqro/impl/containers_traverse.h"

#include "gtest/gtest.h"

#include <QLinkedList>
#include <QList>
#include <QMap>
#include <QSet>
#include <QVector>

#include <list>
#include <set>
#include <unordered_set>
#include <vector>

using namespace asynqro;

template <typename, typename>
struct InnerTypeChanger;
template <template <typename...> typename C, typename NewType, typename... Args>
struct InnerTypeChanger<C<Args...>, NewType>
{
    using type = C<NewType>;
};

template <typename Container>
auto toVector(const Container &x, bool sort = false)
{
    std::vector<typename Container::value_type> result;
    std::copy(x.begin(), x.end(), std::back_inserter(result));
    if (sort)
        std::sort(result.begin(), result.end());
    return result;
}

using SingleSocketedContainersTypes = ::testing::Types<QVector<int>, QList<int>, QLinkedList<int>, std::vector<int>,
                                                       std::set<int>, std::multiset<int>, std::list<int>>;
using SingleSocketedUnorderedContainersTypes =
    ::testing::Types<QSet<int>, std::unordered_set<int>, std::unordered_multiset<int>>;

template <typename Container>
class SingleSocketedInputContainersMapTest : public testing::Test
{
public:
    using Source = Container;
};

template <typename Container>
class SingleSocketedOutputContainersMapTest : public testing::Test
{
public:
    using Source = Container;
};

template <typename Container>
class SingleSocketedUnorderedInputContainersMapTest : public testing::Test
{
public:
    using Source = Container;
};

template <typename Container>
class SingleSocketedUnorderedOutputContainersMapTest : public testing::Test
{
public:
    using Source = Container;
};

TYPED_TEST_CASE(SingleSocketedInputContainersMapTest, SingleSocketedContainersTypes);

TYPED_TEST(SingleSocketedInputContainersMapTest, mapEmpty)
{
    typename TestFixture::Source emptyContainer;
    QVector<int> result = traverse::map(emptyContainer, [](int x) -> int { return x * 2; }, QVector<int>());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(SingleSocketedInputContainersMapTest, map)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    QVector<int> result = traverse::map(testContainer, [](int x) { return x * 2; }, QVector<int>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, result[i - 1]) << i;
}

TYPED_TEST(SingleSocketedInputContainersMapTest, mapToDoubleSocketed)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    QMap<int, bool> result = traverse::map(testContainer, [](int x) { return qMakePair(x * 2, x % 2); },
                                           QMap<int, bool>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_TRUE(result.contains(key)) << key;
        EXPECT_EQ(i % 2, result[key]) << key;
    }
}

TYPED_TEST(SingleSocketedInputContainersMapTest, mapWithAnotherType)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    QVector<double> result = traverse::map(testContainer, [](int x) { return x * 2.0; }, QVector<double>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i)
        EXPECT_DOUBLE_EQ(i * 2.0, result[i - 1]) << i;
}

TYPED_TEST(SingleSocketedInputContainersMapTest, mapWithIndices)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    QVector<long long> result = traverse::map(testContainer, [](long long index, int x) { return x * index; },
                                              QVector<long long>());
    ASSERT_EQ(9, result.size());
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(i * (i + 1), result[i]) << i;
}

TYPED_TEST(SingleSocketedInputContainersMapTest, mapShortEmpty)
{
    typename TestFixture::Source emptyContainer;
    typename TestFixture::Source result = traverse::map(emptyContainer, [](int x) -> int { return x * 2; });
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(SingleSocketedInputContainersMapTest, mapShort)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    typename TestFixture::Source result = traverse::map(testContainer, [](int x) { return x * 2; });
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, false);
    for (int i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, converted[i - 1]) << i;
}

TYPED_TEST(SingleSocketedInputContainersMapTest, mapWithAnotherTypeShort)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    typename InnerTypeChanger<typename TestFixture::Source, double>::type result = traverse::map(testContainer, [](int x) {
        return x * 2.0;
    });
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, false);
    for (int i = 1; i <= 9; ++i)
        EXPECT_DOUBLE_EQ(i * 2.0, converted[i - 1]) << i;
}

TYPED_TEST(SingleSocketedInputContainersMapTest, mapWithIndicesShort)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    typename InnerTypeChanger<typename TestFixture::Source, long long>::type result =
        traverse::map(testContainer, [](long long index, int x) { return x * index; });
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, false);
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(i * (i + 1), converted[i]) << i;
}

TYPED_TEST_CASE(SingleSocketedOutputContainersMapTest, SingleSocketedContainersTypes);

TYPED_TEST(SingleSocketedOutputContainersMapTest, mapEmpty)
{
    QVector<int> emptyContainer;
    typename TestFixture::Source result = traverse::map(emptyContainer, [](int x) -> int { return x * 2; },
                                                        typename TestFixture::Source());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(SingleSocketedOutputContainersMapTest, map)
{
    QVector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    typename TestFixture::Source result = traverse::map(testContainer, [](int x) { return x * 2; },
                                                        typename TestFixture::Source());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, false);
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, converted[i - 1]) << i;
}

TYPED_TEST(SingleSocketedOutputContainersMapTest, mapWithAnotherType)
{
    using ResultType = typename InnerTypeChanger<typename TestFixture::Source, double>::type;
    QVector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    ResultType result = traverse::map(testContainer, [](int x) { return x * 2.0; }, ResultType());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, false);
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_DOUBLE_EQ(i * 2.0, converted[i - 1]) << i;
}

TYPED_TEST(SingleSocketedOutputContainersMapTest, mapWithIndices)
{
    using ResultType = typename InnerTypeChanger<typename TestFixture::Source, long long>::type;
    QVector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    ResultType result = traverse::map(testContainer, [](long long index, int x) { return x * index; }, ResultType());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, false);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i * testContainer[i], converted[i]) << i;
}

TYPED_TEST_CASE(SingleSocketedUnorderedInputContainersMapTest, SingleSocketedUnorderedContainersTypes);

TYPED_TEST(SingleSocketedUnorderedInputContainersMapTest, mapEmpty)
{
    typename TestFixture::Source emptyContainer;
    QVector<int> result = traverse::map(emptyContainer, [](int x) -> int { return x * 2; }, QVector<int>());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(SingleSocketedUnorderedInputContainersMapTest, map)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    QVector<int> result = traverse::map(testContainer, [](int x) { return x * 2; }, QVector<int>());
    ASSERT_EQ(9, result.size());
    std::sort(result.begin(), result.end());
    for (int i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, result[i - 1]) << i;
}

TYPED_TEST(SingleSocketedUnorderedInputContainersMapTest, mapWithAnotherType)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    QVector<double> result = traverse::map(testContainer, [](int x) { return x * 2.0; }, QVector<double>());
    ASSERT_EQ(9, result.size());
    std::sort(result.begin(), result.end());
    for (int i = 1; i <= 9; ++i)
        EXPECT_DOUBLE_EQ(i * 2.0, result[i - 1]) << i;
}

TYPED_TEST(SingleSocketedUnorderedInputContainersMapTest, mapToDoubleSocketed)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    QMap<int, bool> result = traverse::map(testContainer, [](int x) { return qMakePair(x * 2, x % 2); },
                                           QMap<int, bool>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_TRUE(result.contains(key)) << key;
        EXPECT_EQ(i % 2, result[key]) << key;
    }
}

TYPED_TEST(SingleSocketedUnorderedInputContainersMapTest, mapShortEmpty)
{
    typename TestFixture::Source emptyContainer;
    typename TestFixture::Source result = traverse::map(emptyContainer, [](int x) -> int { return x * 2; });
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(SingleSocketedUnorderedInputContainersMapTest, mapShort)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    typename TestFixture::Source result = traverse::map(testContainer, [](int x) { return x * 2; });
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, true);
    for (int i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, converted[i - 1]) << i;
}

TYPED_TEST(SingleSocketedUnorderedInputContainersMapTest, mapWithAnotherTypeShort)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    typename InnerTypeChanger<typename TestFixture::Source, double>::type result = traverse::map(testContainer, [](int x) {
        return x * 2.0;
    });
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, true);
    for (int i = 1; i <= 9; ++i)
        EXPECT_DOUBLE_EQ(i * 2.0, converted[i - 1]) << i;
}

TYPED_TEST_CASE(SingleSocketedUnorderedOutputContainersMapTest, SingleSocketedUnorderedContainersTypes);

TYPED_TEST(SingleSocketedUnorderedOutputContainersMapTest, mapEmpty)
{
    QVector<int> emptyContainer;
    typename TestFixture::Source result = traverse::map(emptyContainer, [](int x) -> int { return x * 2; },
                                                        typename TestFixture::Source());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(SingleSocketedUnorderedOutputContainersMapTest, map)
{
    QVector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    typename TestFixture::Source result = traverse::map(testContainer, [](int x) { return x * 2; },
                                                        typename TestFixture::Source());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, true);
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, converted[i - 1]) << i;
}

TYPED_TEST(SingleSocketedUnorderedOutputContainersMapTest, mapWithAnotherType)
{
    using ResultType = typename InnerTypeChanger<typename TestFixture::Source, double>::type;
    QVector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    ResultType result = traverse::map(testContainer, [](int x) { return x * 2.0; }, ResultType());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, true);
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_DOUBLE_EQ(i * 2.0, converted[i - 1]) << i;
}

TYPED_TEST(SingleSocketedUnorderedOutputContainersMapTest, mapWithIndices)
{
    using ResultType = typename InnerTypeChanger<typename TestFixture::Source, long long>::type;
    QVector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    ResultType result = traverse::map(testContainer, [](long long index, int x) { return x * index; }, ResultType());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, true);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i * testContainer[i], converted[i]) << i;
}
