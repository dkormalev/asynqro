#include "asynqro/impl/containers_traverse.h"

#include "gtest/gtest.h"

#include <QHash>
#include <QLinkedList>
#include <QList>
#include <QMap>
#include <QSet>
#include <QVector>

#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace asynqro;

template <typename>
struct FlattenTypeExtractor;
template <template <typename...> typename Outer, template <typename...> typename Inner, typename MainArg,
          typename... Args, typename... InnerArgs>
struct FlattenTypeExtractor<Outer<Inner<MainArg, InnerArgs...>, Args...>>
{
    using type = Outer<MainArg>;
};

template <typename Container>
class ContainersTraverseFlattenTest : public testing::Test
{
public:
    using Source = Container;
    using Result = typename FlattenTypeExtractor<Container>::type;

    static inline const Source testContainer = {{1, 2, 3}, {4, 5}, {}, {6, 7, 8, 9}};

    template <typename T>
    auto toVector(const T &x, bool sort = false)
    {
        std::vector<typename Result::value_type> result;
        std::copy(x.begin(), x.end(), std::back_inserter(result));
        if (sort)
            std::sort(result.begin(), result.end());
        return result;
    }
};

template <typename Container>
class UnorderedContainersTraverseFlattenTest : public testing::Test
{
public:
    using Source = Container;
    using Result = typename FlattenTypeExtractor<Container>::type;

    static inline const Source testContainer = {{1, 2, 3}, {4, 5}, {}, {6, 7, 8, 9}};

    template <typename T>
    auto toVector(const T &x)
    {
        std::vector<typename Result::value_type> result;
        std::copy(x.begin(), x.end(), std::back_inserter(result));
        std::sort(result.begin(), result.end());
        return result;
    }
};

using ContainersTraverseFlattenTyps = ::testing::Types<
    QVector<QVector<int>>, QVector<QList<int>>, QVector<std::vector<int>>, QVector<std::set<int>>,
    QVector<std::multiset<int>>, QVector<std::list<int>>, QList<QVector<int>>, QList<QList<int>>,
    QList<std::vector<int>>, QList<std::set<int>>, QList<std::multiset<int>>, QList<std::list<int>>,
    std::vector<QVector<int>>, std::vector<QList<int>>, std::vector<std::vector<int>>, std::vector<std::set<int>>,
    std::vector<std::multiset<int>>, std::vector<std::list<int>>, std::set<QVector<int>>, std::set<QList<int>>,
    std::set<std::vector<int>>, std::set<std::set<int>>, std::set<std::multiset<int>>, std::set<std::list<int>>,
    std::multiset<QVector<int>>, std::multiset<QList<int>>, std::multiset<std::vector<int>>, std::multiset<std::set<int>>,
    std::multiset<std::multiset<int>>, std::multiset<std::list<int>>, std::list<QVector<int>>, std::list<QList<int>>,
    std::list<std::vector<int>>, std::list<std::set<int>>, std::list<std::multiset<int>>, std::list<std::list<int>>>;
TYPED_TEST_CASE(ContainersTraverseFlattenTest, ContainersTraverseFlattenTyps);

TYPED_TEST(ContainersTraverseFlattenTest, flattenToQVector)
{
    QVector<int> result = traverse::flatten(TestFixture::testContainer, QVector<int>());
    ASSERT_EQ(9, result.size());
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, result[i]) << i;
}

TYPED_TEST(ContainersTraverseFlattenTest, flattenToQList)
{
    QList<int> result = traverse::flatten(TestFixture::testContainer, QList<int>());
    ASSERT_EQ(9, result.size());
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, result[i]) << i;
}

TYPED_TEST(ContainersTraverseFlattenTest, flattenToQSet)
{
    QSet<int> result = traverse::flatten(TestFixture::testContainer, QSet<int>());
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result, true);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(ContainersTraverseFlattenTest, flattenToVector)
{
    std::vector<int> result = traverse::flatten(TestFixture::testContainer, std::vector<int>());
    ASSERT_EQ(9, result.size());
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, result[i]) << i;
}

TYPED_TEST(ContainersTraverseFlattenTest, flattenToSet)
{
    std::set<int> result = traverse::flatten(TestFixture::testContainer, std::set<int>());
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(ContainersTraverseFlattenTest, flattenToMultiSet)
{
    std::multiset<int> result = traverse::flatten(TestFixture::testContainer, std::multiset<int>());
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(ContainersTraverseFlattenTest, flattenToList)
{
    std::list<int> resultList = traverse::flatten(TestFixture::testContainer, std::list<int>());
    ASSERT_EQ(9, resultList.size());
    auto converted = this->toVector(resultList);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(ContainersTraverseFlattenTest, flattenToUnorderedSet)
{
    std::unordered_set<int> result = traverse::flatten(TestFixture::testContainer, std::unordered_set<int>());
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result, true);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(ContainersTraverseFlattenTest, flattenToUnorderedMultiSet)
{
    std::unordered_multiset<int> result = traverse::flatten(TestFixture::testContainer, std::unordered_multiset<int>());
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result, true);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(ContainersTraverseFlattenTest, flattenShort)
{
    typename TestFixture::Result result = traverse::flatten(TestFixture::testContainer);
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result);
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

using UnorderedContainersTraverseFlattenTyps =
    ::testing::Types<QVector<QSet<int>>, QVector<std::unordered_set<int>>, QVector<std::unordered_multiset<int>>,
                     QList<QSet<int>>, QList<std::unordered_set<int>>, QList<std::unordered_multiset<int>>,
                     std::vector<QSet<int>>, std::vector<std::unordered_set<int>>, std::vector<std::unordered_multiset<int>>,
                     std::list<QSet<int>>, std::list<std::unordered_set<int>>, std::list<std::unordered_multiset<int>>>;
TYPED_TEST_CASE(UnorderedContainersTraverseFlattenTest, UnorderedContainersTraverseFlattenTyps);

TYPED_TEST(UnorderedContainersTraverseFlattenTest, flattenToQVector)
{
    QVector<int> result = traverse::flatten(TestFixture::testContainer, QVector<int>());
    std::sort(result.begin(), result.end());
    ASSERT_EQ(9, result.size());
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, result[i]) << i;
}

TYPED_TEST(UnorderedContainersTraverseFlattenTest, flattenToQList)
{
    QList<int> result = traverse::flatten(TestFixture::testContainer, QList<int>());
    std::sort(result.begin(), result.end());
    ASSERT_EQ(9, result.size());
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, result[i]) << i;
}

TYPED_TEST(UnorderedContainersTraverseFlattenTest, flattenToQSet)
{
    QSet<int> result = traverse::flatten(TestFixture::testContainer, QSet<int>());
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(UnorderedContainersTraverseFlattenTest, flattenToVector)
{
    std::vector<int> result = traverse::flatten(TestFixture::testContainer, std::vector<int>());
    std::sort(result.begin(), result.end());
    ASSERT_EQ(9, result.size());
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, result[i]) << i;
}

TYPED_TEST(UnorderedContainersTraverseFlattenTest, flattenToSet)
{
    std::set<int> result = traverse::flatten(TestFixture::testContainer, std::set<int>());
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(UnorderedContainersTraverseFlattenTest, flattenToMultiSet)
{
    std::multiset<int> result = traverse::flatten(TestFixture::testContainer, std::multiset<int>());
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(UnorderedContainersTraverseFlattenTest, flattenToList)
{
    std::list<int> resultList = traverse::flatten(TestFixture::testContainer, std::list<int>());
    ASSERT_EQ(9, resultList.size());
    auto converted = this->toVector(resultList);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(UnorderedContainersTraverseFlattenTest, flattenToUnorderedSet)
{
    std::unordered_set<int> result = traverse::flatten(TestFixture::testContainer, std::unordered_set<int>());
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(UnorderedContainersTraverseFlattenTest, flattenToUnorderedMultiSet)
{
    std::unordered_multiset<int> result = traverse::flatten(TestFixture::testContainer, std::unordered_multiset<int>());
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result);
    for (size_t i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TYPED_TEST(UnorderedContainersTraverseFlattenTest, flattenShort)
{
    typename TestFixture::Result result = traverse::flatten(TestFixture::testContainer);
    ASSERT_EQ(9, result.size());
    auto converted = this->toVector(result);
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(i + 1, converted[i]) << i;
}

TEST(ContainersTraverseFlattenExtraTest, flattenShortWithBigInput)
{
    QVector<QVector<int>> testContainer;
    testContainer.reserve(500);
    int last = 0;
    for (int i = 0; i < 500; ++i) {
        int size = i % 10;
        QVector<int> inner;
        inner.reserve(size);
        for (int j = 0; j < size; ++j)
            inner << ++last;
        testContainer << inner;
    }
    QVector<int> result = traverse::flatten(testContainer);
    ASSERT_EQ(last, result.size());
    for (int i = 0; i < last; ++i)
        EXPECT_EQ(i + 1, result[i]) << i;
}
