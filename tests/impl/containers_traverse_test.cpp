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

template <typename Container>
class SingleSocketedContainersTraverseTest : public testing::Test
{
public:
    using Source = Container;
};

template <typename Container>
class DoubleSocketedContainersTraverseTest : public testing::Test
{
public:
    using Source = Container;

    auto toVector(const Container &x)
    {
        std::vector<typename Container::key_type> result;
        for (auto it = x.cbegin(); it != x.cend(); ++it) {
            if constexpr (detail::IsSpecialization_V<Container, QMap> || detail::IsSpecialization_V<Container, QMultiMap>)
                result.push_back(it.key());
            else
                result.push_back(it->first);
        }
        return result;
    }
};

template <typename Container>
class SingleSocketedUnorderedContainersTraverseTest : public testing::Test
{
public:
    using Source = Container;

    auto toVector(const Container &x)
    {
        std::vector<typename Container::value_type> result;
        std::copy(x.begin(), x.end(), std::back_inserter(result));
        std::sort(result.begin(), result.end());
        return result;
    }
};

template <typename Container>
class DoubleSocketedUnorderedContainersTraverseTest : public testing::Test
{
public:
    using Source = Container;

    auto toVector(const Container &x)
    {
        std::vector<typename Container::key_type> result;
        for (auto it = x.cbegin(); it != x.cend(); ++it) {
            if constexpr (detail::IsSpecialization_V<Container, QHash> || detail::IsSpecialization_V<Container, QMultiHash>)
                result.push_back(it.key());
            else
                result.push_back(it->first);
        }
        std::sort(result.begin(), result.end());
        return result;
    }
};

using SingleSocketedContainersTypes = ::testing::Types<QVector<int>, QList<int>, QLinkedList<int>, std::vector<int>,
                                                       std::set<int>, std::multiset<int>, std::list<int>>;
TYPED_TEST_CASE(SingleSocketedContainersTraverseTest, SingleSocketedContainersTypes);

TYPED_TEST(SingleSocketedContainersTraverseTest, findIf)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    int result;

    auto oddPredicate = [](int x) -> bool { return x % 2; };
    auto bigValuePredicate = [](int x) -> bool { return x > 42; };
    auto equalPredicate = [](int x) -> bool { return x == 5; };

    result = traverse::findIf(testContainer, oddPredicate, -1);
    ASSERT_EQ(1, result);

    result = traverse::findIf(testContainer, bigValuePredicate, -1);
    ASSERT_EQ(-1, result);
    result = traverse::findIf(testContainer, bigValuePredicate, 50);
    ASSERT_EQ(50, result);
    result = traverse::findIf(testContainer, bigValuePredicate);
    ASSERT_EQ(0, result);

    result = traverse::findIf(testContainer, equalPredicate);
    EXPECT_EQ(5, result);
}

TYPED_TEST(SingleSocketedContainersTraverseTest, findIfEmpty)
{
    typename TestFixture::Source emptyContainer;
    int result = traverse::findIf(emptyContainer, [](int x) -> bool { return x % 2; }, -1);
    EXPECT_EQ(-1, result);
}

TYPED_TEST(SingleSocketedContainersTraverseTest, filter)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    typename TestFixture::Source result;

    auto oddPredicate = [](int x) -> bool { return x % 2; };
    auto bigValuePredicate = [](int x) -> bool { return x > 42; };
    auto equalPredicate = [](int x) -> bool { return x == 5; };

    result = traverse::filter(testContainer, oddPredicate);
    ASSERT_EQ(5, result.size());
    auto resultIt = result.cbegin();
    EXPECT_EQ(1, *resultIt);
    EXPECT_EQ(3, *(++resultIt));
    EXPECT_EQ(5, *(++resultIt));
    EXPECT_EQ(7, *(++resultIt));
    EXPECT_EQ(9, *(++resultIt));

    result = traverse::filter(testContainer, bigValuePredicate);
    ASSERT_EQ(0, result.size());

    result = traverse::filter(testContainer, equalPredicate);
    ASSERT_EQ(1, result.size());
    EXPECT_EQ(5, *result.cbegin());
}

TYPED_TEST(SingleSocketedContainersTraverseTest, filterEmpty)
{
    typename TestFixture::Source emptyContainer;
    typename TestFixture::Source result = traverse::filter(emptyContainer, [](int x) -> bool { return x % 2; });
    EXPECT_EQ(0, result.size());
}

using SingleSocketedUnorderedContainersTypes =
    ::testing::Types<QSet<int>, std::unordered_set<int>, std::unordered_multiset<int>>;
TYPED_TEST_CASE(SingleSocketedUnorderedContainersTraverseTest, SingleSocketedUnorderedContainersTypes);

TYPED_TEST(SingleSocketedUnorderedContainersTraverseTest, findIf)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    int result;

    auto oddPredicate = [](int x) -> bool { return x % 2; };
    auto bigValuePredicate = [](int x) -> bool { return x > 42; };
    auto equalPredicate = [](int x) -> bool { return x == 5; };

    result = traverse::findIf(testContainer, oddPredicate, -1);
    ASSERT_TRUE((QSet<int>{1, 3, 5, 7, 9}).contains(result));

    result = traverse::findIf(testContainer, bigValuePredicate, -1);
    ASSERT_EQ(-1, result);
    result = traverse::findIf(testContainer, bigValuePredicate, 50);
    ASSERT_EQ(50, result);
    result = traverse::findIf(testContainer, bigValuePredicate);
    ASSERT_EQ(0, result);

    result = traverse::findIf(testContainer, equalPredicate);
    EXPECT_EQ(5, result);
}

TYPED_TEST(SingleSocketedUnorderedContainersTraverseTest, findIfEmpty)
{
    typename TestFixture::Source emptyContainer;
    int result = traverse::findIf(emptyContainer, [](int x) -> bool { return x % 2; }, -1);
    EXPECT_EQ(-1, result);
}

TYPED_TEST(SingleSocketedUnorderedContainersTraverseTest, filter)
{
    typename TestFixture::Source testContainer = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    typename TestFixture::Source result;

    auto oddPredicate = [](int x) -> bool { return x % 2; };
    auto bigValuePredicate = [](int x) -> bool { return x > 42; };
    auto equalPredicate = [](int x) -> bool { return x == 5; };

    result = traverse::filter(testContainer, oddPredicate);
    ASSERT_EQ(5, result.size());
    auto vectorizedResult = this->toVector(result);
    auto resultIt = vectorizedResult.cbegin();
    EXPECT_EQ(1, *resultIt);
    EXPECT_EQ(3, *(++resultIt));
    EXPECT_EQ(5, *(++resultIt));
    EXPECT_EQ(7, *(++resultIt));
    EXPECT_EQ(9, *(++resultIt));

    result = traverse::filter(testContainer, bigValuePredicate);
    ASSERT_EQ(0, result.size());

    result = traverse::filter(testContainer, equalPredicate);
    ASSERT_EQ(1, result.size());
    EXPECT_EQ(5, *result.cbegin());
}

TYPED_TEST(SingleSocketedUnorderedContainersTraverseTest, filterEmpty)
{
    typename TestFixture::Source emptyContainer;
    typename TestFixture::Source result = traverse::filter(emptyContainer, [](int x) -> bool { return x % 2; });
    EXPECT_EQ(0, result.size());
}

using DoubleSocketedContainersTypes =
    ::testing::Types<QMap<int, bool>, QMultiMap<int, bool>, std::map<int, bool>, std::multimap<int, bool>>;
TYPED_TEST_CASE(DoubleSocketedContainersTraverseTest, DoubleSocketedContainersTypes);

TYPED_TEST(DoubleSocketedContainersTraverseTest, findIf)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    QPair<int, bool> result;

    auto oddPredicate = [](int, bool y) -> bool { return y; };
    auto bigValuePredicate = [](int x, bool) -> bool { return x > 42; };
    auto equalPredicate = [](int x, bool) -> bool { return x == 5; };

    result = traverse::findIf(testContainer, oddPredicate, qMakePair(-1, false));
    ASSERT_EQ(1, result.first);
    ASSERT_TRUE(result.second);

    result = traverse::findIf(testContainer, bigValuePredicate, qMakePair(-1, false));
    EXPECT_EQ(-1, result.first);
    EXPECT_FALSE(result.second);
    result = traverse::findIf(testContainer, bigValuePredicate, qMakePair(50, true));
    EXPECT_EQ(50, result.first);
    EXPECT_TRUE(result.second);
    result = traverse::findIf(testContainer, bigValuePredicate);
    EXPECT_EQ(0, result.first);
    EXPECT_FALSE(result.second);

    result = traverse::findIf(testContainer, equalPredicate);
    EXPECT_EQ(5, result.first);
    EXPECT_TRUE(result.second);
}

TYPED_TEST(DoubleSocketedContainersTraverseTest, findIfEmpty)
{
    typename TestFixture::Source emptyContainer;
    QPair<int, bool> result = traverse::findIf(emptyContainer, [](int, bool y) -> bool { return y; },
                                               qMakePair(-1, false));
    EXPECT_EQ(-1, result.first);
    EXPECT_FALSE(result.second);
}

TYPED_TEST(DoubleSocketedContainersTraverseTest, filter)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    typename TestFixture::Source result;

    auto oddPredicate = [](int, bool y) -> bool { return y; };
    auto bigValuePredicate = [](int x, bool) -> bool { return x > 42; };
    auto equalPredicate = [](int x, bool) -> bool { return x == 5; };

    result = traverse::filter(testContainer, oddPredicate);
    ASSERT_EQ(5, result.size());
    auto vectorizedResult = this->toVector(result);
    auto resultIt = vectorizedResult.cbegin();
    EXPECT_EQ(1, *resultIt);
    EXPECT_EQ(3, *(++resultIt));
    EXPECT_EQ(5, *(++resultIt));
    EXPECT_EQ(7, *(++resultIt));
    EXPECT_EQ(9, *(++resultIt));

    result = traverse::filter(testContainer, bigValuePredicate);
    ASSERT_EQ(0, result.size());

    result = traverse::filter(testContainer, equalPredicate);
    ASSERT_EQ(1, result.size());
    vectorizedResult = this->toVector(result);
    EXPECT_EQ(5, *vectorizedResult.cbegin());
}

TYPED_TEST(DoubleSocketedContainersTraverseTest, filterEmpty)
{
    typename TestFixture::Source emptyContainer;
    typename TestFixture::Source result = traverse::filter(emptyContainer, [](int, bool y) -> bool { return y; });
    EXPECT_EQ(0, result.size());
}

using DoubleSocketedUnorderedContainersTypes = ::testing::Types<
    QHash<int, bool>, QMultiHash<int, bool>, std::unordered_map<int, bool>, std::unordered_multimap<int, bool>>;
TYPED_TEST_CASE(DoubleSocketedUnorderedContainersTraverseTest, DoubleSocketedUnorderedContainersTypes);

TYPED_TEST(DoubleSocketedUnorderedContainersTraverseTest, findIf)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    QPair<int, bool> result;

    auto oddPredicate = [](int, bool y) -> bool { return y; };
    auto bigValuePredicate = [](int x, bool) -> bool { return x > 42; };
    auto equalPredicate = [](int x, bool) -> bool { return x == 5; };

    result = traverse::findIf(testContainer, oddPredicate, qMakePair(-1, false));
    ASSERT_TRUE((QSet<int>{1, 3, 5, 7, 9}).contains(result.first));
    ASSERT_TRUE(result.second);

    result = traverse::findIf(testContainer, bigValuePredicate, qMakePair(-1, false));
    EXPECT_EQ(-1, result.first);
    EXPECT_FALSE(result.second);
    result = traverse::findIf(testContainer, bigValuePredicate, qMakePair(50, true));
    EXPECT_EQ(50, result.first);
    EXPECT_TRUE(result.second);
    result = traverse::findIf(testContainer, bigValuePredicate);
    EXPECT_EQ(0, result.first);
    EXPECT_FALSE(result.second);

    result = traverse::findIf(testContainer, equalPredicate);
    EXPECT_EQ(5, result.first);
    EXPECT_TRUE(result.second);
}

TYPED_TEST(DoubleSocketedUnorderedContainersTraverseTest, findIfEmpty)
{
    typename TestFixture::Source emptyContainer;
    QPair<int, bool> result = traverse::findIf(emptyContainer, [](int, bool y) -> bool { return y; },
                                               qMakePair(-1, false));
    EXPECT_EQ(-1, result.first);
    EXPECT_FALSE(result.second);
}

TYPED_TEST(DoubleSocketedUnorderedContainersTraverseTest, filter)
{
    typename TestFixture::Source testContainer = {{1, true},  {2, false}, {3, true},  {4, false}, {5, true},
                                                  {6, false}, {7, true},  {8, false}, {9, true}};
    typename TestFixture::Source result;

    auto oddPredicate = [](int, bool y) -> bool { return y; };
    auto bigValuePredicate = [](int x, bool) -> bool { return x > 42; };
    auto equalPredicate = [](int x, bool) -> bool { return x == 5; };

    result = traverse::filter(testContainer, oddPredicate);
    ASSERT_EQ(5, result.size());
    auto vectorizedResult = this->toVector(result);
    auto resultIt = vectorizedResult.cbegin();
    EXPECT_EQ(1, *resultIt);
    EXPECT_EQ(3, *(++resultIt));
    EXPECT_EQ(5, *(++resultIt));
    EXPECT_EQ(7, *(++resultIt));
    EXPECT_EQ(9, *(++resultIt));

    result = traverse::filter(testContainer, bigValuePredicate);
    ASSERT_EQ(0, result.size());

    result = traverse::filter(testContainer, equalPredicate);
    ASSERT_EQ(1, result.size());
    vectorizedResult = this->toVector(result);
    EXPECT_EQ(5, *vectorizedResult.cbegin());
}

TYPED_TEST(DoubleSocketedUnorderedContainersTraverseTest, filterEmpty)
{
    typename TestFixture::Source emptyContainer;
    typename TestFixture::Source result = traverse::filter(emptyContainer, [](int, bool y) -> bool { return y; });
    EXPECT_EQ(0, result.size());
}
