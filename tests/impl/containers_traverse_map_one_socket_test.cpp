#include "asynqro/impl/containers_traverse.h"

#include "gtest/gtest.h"

#ifdef ASYNQRO_QT_SUPPORT
#    include <QLinkedList>
#    include <QList>
#    include <QMap>
#    include <QSet>
#    include <QVector>
#endif

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

using SingleSocketedContainersTypes = ::testing::Types<std::vector<int>, std::set<int>, std::multiset<int>, std::list<int>
#ifdef ASYNQRO_QT_SUPPORT
                                                       ,
                                                       QVector<int>, QList<int>, QLinkedList<int>
#endif
                                                       >;
using SingleSocketedUnorderedContainersTypes = ::testing::Types<std::unordered_set<int>, std::unordered_multiset<int>
#ifdef ASYNQRO_QT_SUPPORT
                                                                ,
                                                                QSet<int>
#endif
                                                                >;

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

TYPED_TEST_SUITE(SingleSocketedInputContainersMapTest, SingleSocketedContainersTypes);

TYPED_TEST(SingleSocketedInputContainersMapTest, mapEmpty)
{
    typename TestFixture::Source emptyContainer;
    std::vector<int> result = traverse::map(
        emptyContainer, [](int x) -> int { return x * 2; }, std::vector<int>());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(SingleSocketedInputContainersMapTest, map)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> result = traverse::map(
        testContainer, [](int x) { return x * 2; }, std::vector<int>());
    ASSERT_EQ(9, result.size());
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, result[i - 1]) << i;
}

TYPED_TEST(SingleSocketedInputContainersMapTest, mapToDoubleSocketed)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::map<int, bool> result = traverse::map(
        testContainer, [](int x) { return std::make_pair(x * 2, x % 2); }, std::map<int, bool>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_TRUE(result.count(key)) << key;
        EXPECT_EQ(i % 2, result[key]) << key;
    }
}

TYPED_TEST(SingleSocketedInputContainersMapTest, mapWithAnotherType)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<double> result = traverse::map(
        testContainer, [](int x) { return x * 2.0; }, std::vector<double>());
    ASSERT_EQ(9, result.size());
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_DOUBLE_EQ(i * 2.0, result[i - 1]) << i;
}

TYPED_TEST(SingleSocketedInputContainersMapTest, mapWithIndices)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<long long> result = traverse::map(
        testContainer, [](long long index, int x) { return x * index; }, std::vector<long long>());
    ASSERT_EQ(9, result.size());
    for (size_t i = 0; i < 9; ++i)
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

TYPED_TEST_SUITE(SingleSocketedOutputContainersMapTest, SingleSocketedContainersTypes);

TYPED_TEST(SingleSocketedOutputContainersMapTest, mapEmpty)
{
    std::vector<int> emptyContainer;
    typename TestFixture::Source result = traverse::map(
        emptyContainer, [](int x) -> int { return x * 2; }, typename TestFixture::Source());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(SingleSocketedOutputContainersMapTest, map)
{
    std::vector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    typename TestFixture::Source result = traverse::map(
        testContainer, [](int x) { return x * 2; }, typename TestFixture::Source());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, false);
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, converted[i - 1]) << i;
}

TYPED_TEST(SingleSocketedOutputContainersMapTest, mapWithAnotherType)
{
    using ResultType = typename InnerTypeChanger<typename TestFixture::Source, double>::type;
    std::vector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    ResultType result = traverse::map(
        testContainer, [](int x) { return x * 2.0; }, ResultType());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, false);
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_DOUBLE_EQ(i * 2.0, converted[i - 1]) << i;
}

TYPED_TEST(SingleSocketedOutputContainersMapTest, mapWithIndices)
{
    using ResultType = typename InnerTypeChanger<typename TestFixture::Source, long long>::type;
    std::vector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    ResultType result = traverse::map(
        testContainer, [](long long index, int x) { return x * index; }, ResultType());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, false);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i * testContainer[static_cast<int>(i)], converted[i]) << i;
}

TYPED_TEST_SUITE(SingleSocketedUnorderedInputContainersMapTest, SingleSocketedUnorderedContainersTypes);

TYPED_TEST(SingleSocketedUnorderedInputContainersMapTest, mapEmpty)
{
    typename TestFixture::Source emptyContainer;
    std::vector<int> result = traverse::map(
        emptyContainer, [](int x) -> int { return x * 2; }, std::vector<int>());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(SingleSocketedUnorderedInputContainersMapTest, map)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<int> result = traverse::map(
        testContainer, [](int x) { return x * 2; }, std::vector<int>());
    ASSERT_EQ(9, result.size());
    std::sort(result.begin(), result.end());
    for (int i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, result[i - 1]) << i;
}

TYPED_TEST(SingleSocketedUnorderedInputContainersMapTest, mapWithAnotherType)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<double> result = traverse::map(
        testContainer, [](int x) { return x * 2.0; }, std::vector<double>());
    ASSERT_EQ(9, result.size());
    std::sort(result.begin(), result.end());
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_DOUBLE_EQ(i * 2.0, result[i - 1]) << i;
}

TYPED_TEST(SingleSocketedUnorderedInputContainersMapTest, mapToDoubleSocketed)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::map<int, bool> result = traverse::map(
        testContainer, [](int x) { return std::make_pair(x * 2, x % 2); }, std::map<int, bool>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_TRUE(result.count(key)) << key;
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

TYPED_TEST_SUITE(SingleSocketedUnorderedOutputContainersMapTest, SingleSocketedUnorderedContainersTypes);

TYPED_TEST(SingleSocketedUnorderedOutputContainersMapTest, mapEmpty)
{
    std::vector<int> emptyContainer;
    typename TestFixture::Source result = traverse::map(
        emptyContainer, [](int x) -> int { return x * 2; }, typename TestFixture::Source());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(SingleSocketedUnorderedOutputContainersMapTest, map)
{
    std::vector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    typename TestFixture::Source result = traverse::map(
        testContainer, [](int x) { return x * 2; }, typename TestFixture::Source());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, true);
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, converted[i - 1]) << i;
}

TYPED_TEST(SingleSocketedUnorderedOutputContainersMapTest, mapWithAnotherType)
{
    using ResultType = typename InnerTypeChanger<typename TestFixture::Source, double>::type;
    std::vector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    ResultType result = traverse::map(
        testContainer, [](int x) { return x * 2.0; }, ResultType());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, true);
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_DOUBLE_EQ(i * 2.0, converted[i - 1]) << i;
}

TYPED_TEST(SingleSocketedUnorderedOutputContainersMapTest, mapWithIndices)
{
    using ResultType = typename InnerTypeChanger<typename TestFixture::Source, long long>::type;
    std::vector<int> testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    ResultType result = traverse::map(
        testContainer, [](long long index, int x) { return x * index; }, ResultType());
    ASSERT_EQ(9, result.size());
    auto converted = toVector(result, true);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i * testContainer[static_cast<int>(i)], converted[i]) << i;
}
