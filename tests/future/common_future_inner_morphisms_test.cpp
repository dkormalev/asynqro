#include "futurebasetest.h"

class FutureInnerMorphismsTest : public FutureBaseTest
{};

TEST_F(FutureInnerMorphismsTest, innerReduce)
{
    Promise<QVector<int>> promise;
    auto future = createFuture(promise);
    Future<int> reducedFuture = future.innerReduce([](int acc, int x) { return acc + x; }, 0);
    EXPECT_FALSE(reducedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    QVector<int> result = future.result();
    ASSERT_EQ(5, result.count());
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(i + 1, result[i]);
    ASSERT_TRUE(reducedFuture.isCompleted());
    EXPECT_TRUE(reducedFuture.isSucceeded());
    EXPECT_FALSE(reducedFuture.isFailed());
    EXPECT_EQ(15, reducedFuture.result());
}

TEST_F(FutureInnerMorphismsTest, innerMap)
{
    Promise<QVector<int>> promise;
    auto future = createFuture(promise);
    Future<QVector<int>> mappedFuture = future.innerMap([](int x) { return x + 10; }, QVector<int>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    QVector<int> result = mappedFuture.result();
    ASSERT_EQ(5, result.count());
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(i + 11, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerMapShort)
{
    Promise<QVector<int>> promise;
    auto future = createFuture(promise);
    Future<QVector<int>> mappedFuture = future.innerMap([](int x) { return x + 10; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    QVector<int> result = mappedFuture.result();
    ASSERT_EQ(5, result.count());
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(i + 11, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerMapOtherType)
{
    Promise<QVector<int>> promise;
    auto future = createFuture(promise);
    Future<QVector<double>> mappedFuture = future.innerMap([](int x) -> double { return x + 10.0; }, QVector<double>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    QVector<double> result = mappedFuture.result();
    ASSERT_EQ(5, result.count());
    for (int i = 0; i < 5; ++i)
        EXPECT_DOUBLE_EQ(i + 11, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerMapOtherTypeShort)
{
    Promise<QVector<int>> promise;
    auto future = createFuture(promise);
    Future<QVector<double>> mappedFuture = future.innerMap([](int x) -> double { return x + 10.0; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    QVector<double> result = mappedFuture.result();
    ASSERT_EQ(5, result.count());
    for (int i = 0; i < 5; ++i)
        EXPECT_DOUBLE_EQ(i + 11, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerMapOtherContainer)
{
    Promise<QVector<int>> promise;
    auto future = createFuture(promise);
    Future<QMap<int, bool>> mappedFuture = future.innerMap([](int x) { return qMakePair(x, x % 2); }, QMap<int, bool>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    QMap<int, bool> result = mappedFuture.result();
    ASSERT_EQ(5, result.count());
    for (int i = 1; i <= 5; ++i) {
        EXPECT_TRUE(result.contains(i));
        EXPECT_EQ(i % 2, result[i]);
    }
}

TEST_F(FutureInnerMorphismsTest, innerMapWithIndex)
{
    Promise<QVector<int>> promise;
    auto future = createFuture(promise);
    Future<QVector<int>> mappedFuture = future.innerMap([](long long index, int x) { return x + index * 10; },
                                                        QVector<int>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    QVector<int> result = mappedFuture.result();
    ASSERT_EQ(5, result.count());
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(i * 10 + (i + 1), result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerMapWithIndexShort)
{
    Promise<QVector<int>> promise;
    auto future = createFuture(promise);
    Future<QVector<long long>> mappedFuture = future.innerMap([](long long index, int x) { return x + index * 10; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    QVector<long long> result = mappedFuture.result();
    ASSERT_EQ(5, result.count());
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(i * 10 + (i + 1), result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerFilter)
{
    Promise<QVector<int>> promise;
    auto future = createFuture(promise);
    Future<QVector<int>> mappedFuture = future.innerFilter([](int x) { return x % 2; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    QVector<int> result = mappedFuture.result();
    ASSERT_EQ(3, result.count());
    for (int i = 0; i < 3; ++i)
        EXPECT_EQ(i * 2 + 1, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerFlatten)
{
    Promise<QVector<QVector<int>>> promise;
    auto future = createFuture(promise);
    Future<QVector<int>> flattenedFuture = future.innerFlatten(QVector<int>());
    EXPECT_FALSE(flattenedFuture.isCompleted());
    promise.success({{1, 2}, {3}, {4, 5}});
    ASSERT_TRUE(flattenedFuture.isCompleted());
    EXPECT_TRUE(flattenedFuture.isSucceeded());
    EXPECT_FALSE(flattenedFuture.isFailed());
    QVector<int> result = flattenedFuture.result();
    ASSERT_EQ(5, result.count());
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(i + 1, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerFlattenShort)
{
    Promise<QVector<QVector<int>>> promise;
    auto future = createFuture(promise);
    Future<QVector<int>> flattenedFuture = future.innerFlatten();
    EXPECT_FALSE(flattenedFuture.isCompleted());
    promise.success({{1, 2}, {3}, {4, 5}});
    ASSERT_TRUE(flattenedFuture.isCompleted());
    EXPECT_TRUE(flattenedFuture.isSucceeded());
    EXPECT_FALSE(flattenedFuture.isFailed());
    QVector<int> result = flattenedFuture.result();
    ASSERT_EQ(5, result.count());
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(i + 1, result[i]);
}
