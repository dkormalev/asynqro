#include "futurebasetest.h"

#include <vector>

class FutureInnerMorphismsTest : public FutureBaseTest
{};

TEST_F(FutureInnerMorphismsTest, innerReduce)
{
    TestPromise<std::vector<int>> promise;
    auto future = createFuture(promise);
    TestFuture<int> reducedFuture = future.innerReduce([](int acc, int x) { return acc + x; }, 0);
    EXPECT_FALSE(reducedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    std::vector<int> result = future.result();
    ASSERT_EQ(5, result.size());
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(i + 1, result[i]);
    ASSERT_TRUE(reducedFuture.isCompleted());
    EXPECT_TRUE(reducedFuture.isSucceeded());
    EXPECT_FALSE(reducedFuture.isFailed());
    EXPECT_EQ(15, reducedFuture.result());
}

TEST_F(FutureInnerMorphismsTest, innerMap)
{
    TestPromise<std::vector<int>> promise;
    auto future = createFuture(promise);
    TestFuture<std::vector<int>> mappedFuture = future.innerMap([](int x) { return x + 10; }, std::vector<int>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    std::vector<int> result = mappedFuture.result();
    ASSERT_EQ(5, result.size());
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(i + 11, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerMapShort)
{
    TestPromise<std::vector<int>> promise;
    auto future = createFuture(promise);
    TestFuture<std::vector<int>> mappedFuture = future.innerMap([](int x) { return x + 10; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    std::vector<int> result = mappedFuture.result();
    ASSERT_EQ(5, result.size());
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(i + 11, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerMapOtherType)
{
    TestPromise<std::vector<int>> promise;
    auto future = createFuture(promise);
    TestFuture<std::vector<double>> mappedFuture = future.innerMap([](int x) -> double { return x + 10.0; },
                                                                   std::vector<double>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    std::vector<double> result = mappedFuture.result();
    ASSERT_EQ(5, result.size());
    for (size_t i = 0; i < 5; ++i)
        EXPECT_DOUBLE_EQ(i + 11, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerMapOtherTypeShort)
{
    TestPromise<std::vector<int>> promise;
    auto future = createFuture(promise);
    TestFuture<std::vector<double>> mappedFuture = future.innerMap([](int x) -> double { return x + 10.0; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    std::vector<double> result = mappedFuture.result();
    ASSERT_EQ(5, result.size());
    for (size_t i = 0; i < 5; ++i)
        EXPECT_DOUBLE_EQ(i + 11, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerMapOtherContainer)
{
    TestPromise<std::vector<int>> promise;
    auto future = createFuture(promise);
    TestFuture<std::map<int, bool>> mappedFuture = future.innerMap([](int x) { return std::make_pair(x, x % 2); },
                                                                   std::map<int, bool>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    std::map<int, bool> result = mappedFuture.result();
    ASSERT_EQ(5, result.size());
    for (int i = 1; i <= 5; ++i) {
        EXPECT_TRUE(result.count(i));
        EXPECT_EQ(i % 2, result[i]);
    }
}

TEST_F(FutureInnerMorphismsTest, innerMapWithIndex)
{
    TestPromise<std::vector<int>> promise;
    auto future = createFuture(promise);
    TestFuture<std::vector<int>> mappedFuture = future.innerMap([](long long index, int x) { return x + index * 10; },
                                                                std::vector<int>());
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    std::vector<int> result = mappedFuture.result();
    ASSERT_EQ(5, result.size());
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(i * 10 + (i + 1), result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerMapWithIndexShort)
{
    TestPromise<std::vector<int>> promise;
    auto future = createFuture(promise);
    TestFuture<std::vector<long long>> mappedFuture = future.innerMap(
        [](long long index, int x) { return x + index * 10; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    std::vector<long long> result = mappedFuture.result();
    ASSERT_EQ(5, result.size());
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(i * 10 + (i + 1), result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerFilter)
{
    TestPromise<std::vector<int>> promise;
    auto future = createFuture(promise);
    TestFuture<std::vector<int>> mappedFuture = future.innerFilter([](int x) { return x % 2; });
    EXPECT_FALSE(mappedFuture.isCompleted());
    promise.success({1, 2, 3, 4, 5});
    ASSERT_TRUE(mappedFuture.isCompleted());
    EXPECT_TRUE(mappedFuture.isSucceeded());
    EXPECT_FALSE(mappedFuture.isFailed());
    std::vector<int> result = mappedFuture.result();
    ASSERT_EQ(3, result.size());
    for (size_t i = 0; i < 3; ++i)
        EXPECT_EQ(i * 2 + 1, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerFlatten)
{
    TestPromise<std::vector<std::vector<int>>> promise;
    auto future = createFuture(promise);
    TestFuture<std::vector<int>> flattenedFuture = future.innerFlatten(std::vector<int>());
    EXPECT_FALSE(flattenedFuture.isCompleted());
    promise.success({{1, 2}, {3}, {4, 5}});
    ASSERT_TRUE(flattenedFuture.isCompleted());
    EXPECT_TRUE(flattenedFuture.isSucceeded());
    EXPECT_FALSE(flattenedFuture.isFailed());
    std::vector<int> result = flattenedFuture.result();
    ASSERT_EQ(5, result.size());
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(i + 1, result[i]);
}

TEST_F(FutureInnerMorphismsTest, innerFlattenShort)
{
    TestPromise<std::vector<std::vector<int>>> promise;
    auto future = createFuture(promise);
    TestFuture<std::vector<int>> flattenedFuture = future.innerFlatten();
    EXPECT_FALSE(flattenedFuture.isCompleted());
    promise.success({{1, 2}, {3}, {4, 5}});
    ASSERT_TRUE(flattenedFuture.isCompleted());
    EXPECT_TRUE(flattenedFuture.isSucceeded());
    EXPECT_FALSE(flattenedFuture.isFailed());
    std::vector<int> result = flattenedFuture.result();
    ASSERT_EQ(5, result.size());
    for (size_t i = 0; i < 5; ++i)
        EXPECT_EQ(i + 1, result[i]);
}
