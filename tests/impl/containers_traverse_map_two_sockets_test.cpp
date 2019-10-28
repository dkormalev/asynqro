#include "asynqro/impl/containers_traverse.h"

#include "gtest/gtest.h"

#ifdef ASYNQRO_QT_SUPPORT
#    include <QHash>
#    include <QMap>
#    include <QVector>
#endif

#include <algorithm>
#include <map>
#include <unordered_map>
#include <vector>

using namespace asynqro;

template <typename, typename, typename>
struct InnerTypeChanger;
template <template <typename...> typename C, typename NewType1, typename NewType2, typename... Args>
struct InnerTypeChanger<C<Args...>, NewType1, NewType2>
{
    using type = C<NewType1, NewType2>;
};

#ifdef ASYNQRO_QT_SUPPORT
template <typename Container>
inline constexpr bool isQMap =
    detail::IsSpecialization_V<Container, QMap> || detail::IsSpecialization_V<Container, QMultiMap>;
template <typename Container>
inline constexpr bool isQHash =
    detail::IsSpecialization_V<Container, QHash> || detail::IsSpecialization_V<Container, QMultiHash>;
#else
template <typename Container>
inline constexpr bool isQMap = false;
template <typename Container>
inline constexpr bool isQHash = false;
#endif

using DoubleSocketedContainersTypes = ::testing::Types<std::map<int, bool>, std::multimap<int, bool>
#ifdef ASYNQRO_QT_SUPPORT
                                                       ,
                                                       QMap<int, bool>, QMultiMap<int, bool>
#endif
                                                       >;

using DoubleSocketedUnorderedContainersTypes =
    ::testing::Types<std::unordered_map<int, bool>, std::unordered_multimap<int, bool>
#ifdef ASYNQRO_QT_SUPPORT
                     ,
                     QHash<int, bool>, QMultiHash<int, bool>
#endif
                     >;

template <typename Container>
class DoubleSocketedInputContainersMapTest : public testing::Test
{
public:
    using Source = Container;
};

template <typename Container>
class DoubleSocketedOutputContainersMapTest : public testing::Test
{
public:
    using Source = Container;
};

template <typename Container>
class DoubleSocketedUnorderedInputContainersMapTest : public testing::Test
{
public:
    using Source = Container;
};

template <typename Container>
class DoubleSocketedUnorderedOutputContainersMapTest : public testing::Test
{
public:
    using Source = Container;
};

TYPED_TEST_SUITE(DoubleSocketedInputContainersMapTest, DoubleSocketedContainersTypes);

TYPED_TEST(DoubleSocketedInputContainersMapTest, mapEmpty)
{
    typename TestFixture::Source emptyContainer;
    std::map<int, bool> result = traverse::map(
        emptyContainer, [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); }, std::map<int, bool>());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(DoubleSocketedInputContainersMapTest, map)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    ;
    std::map<int, bool> result = traverse::map(
        testContainer, [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); }, std::map<int, bool>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        EXPECT_EQ(!(i % 3), result[key]) << key;
    }
}

TYPED_TEST(DoubleSocketedInputContainersMapTest, mapToSingleSocketed)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    ;
    std::vector<int> result = traverse::map(
        testContainer, [](int x, bool) { return x * 2; }, std::vector<int>());
    ASSERT_EQ(9, result.size());
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, result[i - 1]) << i;
}

TYPED_TEST(DoubleSocketedInputContainersMapTest, mapWithAnotherType)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};

    std::map<int, double> result = traverse::map(
        testContainer, [](int x, bool) { return std::make_pair(x * 2, x * 2.0); }, std::map<int, double>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        EXPECT_DOUBLE_EQ(i * 2.0, result[key]) << key;
    }
}

TYPED_TEST(DoubleSocketedInputContainersMapTest, mapShortEmpty)
{
    typename TestFixture::Source emptyContainer;
    typename TestFixture::Source result = traverse::map(emptyContainer,
                                                        [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); });
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(DoubleSocketedInputContainersMapTest, mapShort)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    ;
    typename TestFixture::Source result = traverse::map(testContainer,
                                                        [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); });
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        if constexpr (isQMap<typename TestFixture::Source> || isQHash<typename TestFixture::Source>)
            EXPECT_EQ(!(i % 3), result.find(key).value()) << key;
        else
            EXPECT_EQ(!(i % 3), result.find(key)->second) << key;
    }
}

TYPED_TEST(DoubleSocketedInputContainersMapTest, mapWithAnotherTypeShort)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    ;
    typename InnerTypeChanger<typename TestFixture::Source, int, double>::type result =
        traverse::map(testContainer, [](int x, bool) { return std::make_pair(x * 2, x * 2.0); });
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        if constexpr (isQMap<typename TestFixture::Source> || isQHash<typename TestFixture::Source>)
            EXPECT_DOUBLE_EQ(i * 2.0, result.find(key).value()) << key;
        else
            EXPECT_DOUBLE_EQ(i * 2.0, result.find(key)->second) << key;
    }
}

TYPED_TEST_SUITE(DoubleSocketedOutputContainersMapTest, DoubleSocketedContainersTypes);

TYPED_TEST(DoubleSocketedOutputContainersMapTest, mapEmpty)
{
    std::map<int, bool> emptyContainer;
    typename TestFixture::Source result = traverse::map(
        emptyContainer, [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); }, typename TestFixture::Source());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(DoubleSocketedOutputContainersMapTest, map)
{
    std::map<int, bool> testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                         {6, false}, {7, true},  {8, false}, {9, true}};
    typename TestFixture::Source result = traverse::map(
        testContainer, [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); }, typename TestFixture::Source());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        if constexpr (isQMap<typename TestFixture::Source> || isQHash<typename TestFixture::Source>)
            EXPECT_EQ(!(i % 3), result.find(key).value()) << key;
        else
            EXPECT_EQ(!(i % 3), result.find(key)->second) << key;
    }
}

TYPED_TEST(DoubleSocketedOutputContainersMapTest, mapWithAnotherType)
{
    using ResultType = typename InnerTypeChanger<typename TestFixture::Source, int, double>::type;
    std::map<int, bool> testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                         {6, false}, {7, true},  {8, false}, {9, true}};
    ResultType result = traverse::map(
        testContainer, [](int x, bool) { return std::make_pair(x * 2, x * 2.0); }, ResultType());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        if constexpr (isQMap<typename TestFixture::Source> || isQHash<typename TestFixture::Source>)
            EXPECT_DOUBLE_EQ(i * 2.0, result.find(key).value()) << key;
        else
            EXPECT_DOUBLE_EQ(i * 2.0, result.find(key)->second) << key;
    }
}

TYPED_TEST_SUITE(DoubleSocketedUnorderedInputContainersMapTest, DoubleSocketedUnorderedContainersTypes);

TYPED_TEST(DoubleSocketedUnorderedInputContainersMapTest, mapEmpty)
{
    typename TestFixture::Source emptyContainer;
    std::map<int, bool> result = traverse::map(
        emptyContainer, [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); }, std::map<int, bool>());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(DoubleSocketedUnorderedInputContainersMapTest, map)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    ;
    std::map<int, bool> result = traverse::map(
        testContainer, [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); }, std::map<int, bool>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        EXPECT_EQ(!(i % 3), result[key]) << key;
    }
}

TYPED_TEST(DoubleSocketedUnorderedInputContainersMapTest, mapToSingleSocketed)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    ;
    std::vector<int> result = traverse::map(
        testContainer, [](int x, bool) { return x * 2; }, std::vector<int>());
    std::sort(result.begin(), result.end());
    ASSERT_EQ(9, result.size());
    for (size_t i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, result[i - 1]) << i;
}

TYPED_TEST(DoubleSocketedUnorderedInputContainersMapTest, mapWithAnotherType)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};

    std::map<int, double> result = traverse::map(
        testContainer, [](int x, bool) { return std::make_pair(x * 2, x * 2.0); }, std::map<int, double>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        EXPECT_DOUBLE_EQ(i * 2.0, result[key]) << key;
    }
}

TYPED_TEST(DoubleSocketedUnorderedInputContainersMapTest, mapShortEmpty)
{
    typename TestFixture::Source emptyContainer;
    typename TestFixture::Source result = traverse::map(emptyContainer,
                                                        [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); });
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(DoubleSocketedUnorderedInputContainersMapTest, mapShort)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    ;
    typename TestFixture::Source result = traverse::map(testContainer,
                                                        [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); });
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        if constexpr (isQMap<typename TestFixture::Source> || isQHash<typename TestFixture::Source>)
            EXPECT_EQ(!(i % 3), result.find(key).value()) << key;
        else
            EXPECT_EQ(!(i % 3), result.find(key)->second) << key;
    }
}

TYPED_TEST(DoubleSocketedUnorderedInputContainersMapTest, mapWithAnotherTypeShort)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    ;
    typename InnerTypeChanger<typename TestFixture::Source, int, double>::type result =
        traverse::map(testContainer, [](int x, bool) { return std::make_pair(x * 2, x * 2.0); });
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        if constexpr (isQMap<typename TestFixture::Source> || isQHash<typename TestFixture::Source>)
            EXPECT_DOUBLE_EQ(i * 2.0, result.find(key).value()) << key;
        else
            EXPECT_DOUBLE_EQ(i * 2.0, result.find(key)->second) << key;
    }
}

TYPED_TEST_SUITE(DoubleSocketedUnorderedOutputContainersMapTest, DoubleSocketedUnorderedContainersTypes);

TYPED_TEST(DoubleSocketedUnorderedOutputContainersMapTest, mapEmpty)
{
    std::map<int, bool> emptyContainer;
    typename TestFixture::Source result = traverse::map(
        emptyContainer, [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); }, typename TestFixture::Source());
    EXPECT_EQ(0, result.size());
}

TYPED_TEST(DoubleSocketedUnorderedOutputContainersMapTest, map)
{
    std::map<int, bool> testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                         {6, false}, {7, true},  {8, false}, {9, true}};
    typename TestFixture::Source result = traverse::map(
        testContainer, [](int x, bool) { return std::make_pair(x * 2, !(x % 3)); }, typename TestFixture::Source());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        if constexpr (isQMap<typename TestFixture::Source> || isQHash<typename TestFixture::Source>)
            EXPECT_EQ(!(i % 3), result.find(key).value()) << key;
        else
            EXPECT_EQ(!(i % 3), result.find(key)->second) << key;
    }
}

TYPED_TEST(DoubleSocketedUnorderedOutputContainersMapTest, mapWithAnotherType)
{
    using ResultType = typename InnerTypeChanger<typename TestFixture::Source, int, double>::type;
    std::map<int, bool> testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                         {6, false}, {7, true},  {8, false}, {9, true}};
    ResultType result = traverse::map(
        testContainer, [](int x, bool) { return std::make_pair(x * 2, x * 2.0); }, ResultType());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_EQ(1, result.count(key)) << key;
        if constexpr (isQMap<typename TestFixture::Source> || isQHash<typename TestFixture::Source>)
            EXPECT_DOUBLE_EQ(i * 2.0, result.find(key).value()) << key;
        else
            EXPECT_DOUBLE_EQ(i * 2.0, result.find(key)->second) << key;
    }
}
