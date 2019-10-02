#include "asynqro/impl/containers_traverse.h"

#include "gtest/gtest.h"

#include <QByteArray>
#include <QJsonArray>
#include <QMap>
#include <QVariantList>
#include <QVector>

using namespace asynqro;

TEST(ZeroSocketedInputContainersMapTest, mapQByteArrayEmpty)
{
    QByteArray emptyContainer;
    QVector<int> result = traverse::map(
        emptyContainer, [](char x) -> int { return (x - '0') * 2; }, QVector<int>());
    EXPECT_EQ(0, result.size());
}

TEST(ZeroSocketedInputContainersMapTest, mapQByteArray)
{
    QByteArray testContainer = "123456789";
    QVector<int> result = traverse::map(
        testContainer, [](char x) { return (x - '0') * 2; }, QVector<int>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, result[i - 1]) << i;
}

TEST(ZeroSocketedInputContainersMapTest, mapQByteArrayToDoubleSocketed)
{
    QByteArray testContainer = "123456789";
    QMap<int, bool> result = traverse::map(
        testContainer, [](char x) { return qMakePair((x - '0') * 2, (x - '0') % 2); }, QMap<int, bool>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_TRUE(result.contains(key)) << key;
        EXPECT_EQ(i % 2, result[key]) << key;
    }
}

TEST(ZeroSocketedInputContainersMapTest, mapQByteArrayWithIndices)
{
    QByteArray testContainer = "123456789";
    QVector<long long> result = traverse::map(
        testContainer, [](long long index, char x) { return (x - '0') * index; }, QVector<long long>());
    ASSERT_EQ(9, result.size());
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(i * (i + 1), result[i]) << i;
}

TEST(ZeroSocketedInputContainersMapTest, mapQJsonArrayEmpty)
{
    QJsonArray emptyContainer;
    QVector<int> result = traverse::map(
        emptyContainer, [](const QJsonValue &x) -> int { return x.toInt() * 2; }, QVector<int>());
    EXPECT_EQ(0, result.size());
}

TEST(ZeroSocketedInputContainersMapTest, mapQJsonArray)
{
    QJsonArray testContainer = QJsonArray::fromVariantList({1, 2, 3, 4, 5, 6, 7, 8, 9});
    QVector<int> result = traverse::map(
        testContainer, [](const QJsonValue &x) -> int { return x.toInt() * 2; }, QVector<int>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i)
        EXPECT_EQ(i * 2, result[i - 1]) << i;
}

TEST(ZeroSocketedInputContainersMapTest, mapQJsonArrayToDoubleSocketed)
{
    QJsonArray testContainer = QJsonArray::fromVariantList({1, 2, 3, 4, 5, 6, 7, 8, 9});
    QMap<int, bool> result = traverse::map(
        testContainer, [](const QJsonValue &x) { return qMakePair(x.toInt() * 2, x.toInt() % 2); }, QMap<int, bool>());
    ASSERT_EQ(9, result.size());
    for (int i = 1; i <= 9; ++i) {
        int key = i * 2;
        ASSERT_TRUE(result.contains(key)) << key;
        EXPECT_EQ(i % 2, result[key]) << key;
    }
}

TEST(ZeroSocketedInputContainersMapTest, mapQJsonArrayWithIndices)
{
    QJsonArray testContainer = QJsonArray::fromVariantList({1, 2, 3, 4, 5, 6, 7, 8, 9});
    QVector<long long> result = traverse::map(
        testContainer, [](long long index, const QJsonValue &x) { return x.toInt() * index; }, QVector<long long>());
    ASSERT_EQ(9, result.size());
    for (int i = 0; i < 9; ++i)
        EXPECT_EQ(i * (i + 1), result[i]) << i;
}
