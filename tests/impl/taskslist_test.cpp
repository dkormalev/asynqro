#include "asynqro/impl/taskslist_p.h"

#include "gtest/gtest.h"

#include <QVector>

using namespace asynqro::tasks;

using TraverseOrderTestData = std::tuple<QVector<quint8>, QVector<int>>;
class TraverseOrderTasksListTest : public testing::TestWithParam<TraverseOrderTestData>
{};

using RemovalTestInnerData = std::tuple<int, int, QVector<int>>;
using RemovalTestData = std::tuple<int, QVector<RemovalTestInnerData>>;
class RemovalTasksListTest : public testing::TestWithParam<RemovalTestData>
{};

class SizesTasksListTest : public testing::TestWithParam<int>
{};

// clang-format off
INSTANTIATE_TEST_CASE_P(
        OrderTasksTestParameters, TraverseOrderTasksListTest,
        testing::Values(
            TraverseOrderTestData{{}, {}},
            TraverseOrderTestData{{TaskPriority::Regular}, {0}},
            TraverseOrderTestData{{TaskPriority::Regular, TaskPriority::Regular, TaskPriority::Regular}, {0, 1, 2}},
            TraverseOrderTestData{{TaskPriority::Regular, TaskPriority::Regular, TaskPriority::Background}, {0, 1, 2}},
            TraverseOrderTestData{{TaskPriority::Background, TaskPriority::Regular, TaskPriority::Emergency}, {2, 1, 0}},
            TraverseOrderTestData{{TaskPriority::Regular, TaskPriority::Background, TaskPriority::Regular}, {0, 2, 1}},
            TraverseOrderTestData{{TaskPriority::Background, TaskPriority::Background, TaskPriority::Regular}, {2, 0, 1}},
            TraverseOrderTestData{{TaskPriority::Emergency, TaskPriority::Emergency, TaskPriority::Emergency}, {0, 1, 2}},
            TraverseOrderTestData{{1, 1, 1}, {0, 1, 2}},
            TraverseOrderTestData{{3, 2, 1}, {2, 1, 0}}));
// clang-format on
TEST_P(TraverseOrderTasksListTest, traverseOrder)
{
    QVector<quint8> insert = std::get<0>(GetParam());
    QVector<int> expected = std::get<1>(GetParam());
    ASSERT_EQ(expected.count(), insert.count());
    QVector<int> result;
    TasksList list;
    for (int i = 0; i < insert.count(); ++i)
        list.insert([&result, i]() { result << i; }, TaskType::Custom, 0, static_cast<TaskPriority>(insert[i]));
    for (const auto &taskInfo : list)
        taskInfo.task();
    EXPECT_EQ(expected, result);
}

INSTANTIATE_TEST_CASE_P(IncrementTasksTestParameters, SizesTasksListTest, testing::Values(0, 1, 2, 5, 10));
TEST_P(SizesTasksListTest, iteratorIncrement)
{
    int size = GetParam();
    TasksList list;
    for (int i = 0; i < size; ++i)
        list.insert([]() {}, TaskType::Custom, 0, TaskPriority::Regular);
    auto it = list.begin();
    for (int i = 0; i < size; ++i) {
        EXPECT_NE(list.end(), it);
        EXPECT_TRUE(it->isValid());
        EXPECT_NE(&TaskInfo::empty(), &*it);
        ++it;
    }
    EXPECT_EQ(list.end(), it);
    EXPECT_FALSE(it->isValid());
    EXPECT_EQ(&TaskInfo::empty(), &*it);
    ++it;
    EXPECT_EQ(list.end(), it);
    EXPECT_FALSE(it->isValid());
    EXPECT_EQ(&TaskInfo::empty(), &*it);
}

// clang-format off
INSTANTIATE_TEST_CASE_P(RemovalTasksTestParameters, RemovalTasksListTest,
                        testing::Values(
                            RemovalTestData{0, {}},
                            RemovalTestData{1, {{0, -1, {}}}},
                            RemovalTestData{2, {{0, 1, {1}}, {1, -1, {}}}},
                            RemovalTestData{2, {{1, -1, {0}}, {0, -1, {}}}},
                            RemovalTestData{5, {{0, 1, {1, 2, 3, 4}},
                                                {1, 2, {2, 3, 4}},
                                                {2, 3, {3, 4}},
                                                {3, 4, {4}},
                                                {4, -1, {}}}},
                            RemovalTestData{5, {{2, 3, {0, 1, 3, 4}},
                                                {1, 3, {0, 3, 4}},
                                                {3, 4, {0, 4}},
                                                {0, 4, {4}},
                                                {4, -1, {}}}},
                            RemovalTestData{5, {{4, -1, {0, 1, 2, 3}},
                                                {3, -1, {0, 1, 2}},
                                                {2, -1, {0, 1}},
                                                {1, -1, {0}},
                                                {0, -1, {}}}}
                            ));
// clang-format on
TEST_P(RemovalTasksListTest, erase)
{
    int size = std::get<0>(GetParam());
    TasksList list;
    QVector<int> result;
    for (int i = 0; i < size; ++i)
        list.insert([&result, i]() { result << i; }, TaskType::Custom, i, TaskPriority::Regular);

    QVector<RemovalTestInnerData> testData = std::get<1>(GetParam());
    EXPECT_EQ(size, list.size());
    for (const auto &[del, next, others] : testData) {
        ASSERT_EQ(list.size() - 1, others.count());
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (it->tag == del) {
                it = list.erase(it);
                if (next == -1) {
                    EXPECT_EQ(list.end(), it);
                } else {
                    EXPECT_NE(list.end(), it);
                    EXPECT_EQ(next, it->tag);
                }
                break;
            }
        }
        EXPECT_EQ(list.size(), others.count());
        result.clear();
        for (const auto &taskInfo : list)
            taskInfo.task();
        EXPECT_EQ(others, result);
    }
}

TEST(TasksListTest, sizeAndEmptiness)
{
    int n = 10;
    TasksList list;
    EXPECT_EQ(0, list.size());
    EXPECT_TRUE(list.empty());

    for (int i = 0; i < n; ++i) {
        list.insert([]() {}, TaskType::Custom, 0, TaskPriority::Regular);
        EXPECT_EQ(i + 1, list.size());
        EXPECT_FALSE(list.empty());
    }

    for (int i = n; i > 0; --i) {
        EXPECT_EQ(i, list.size());
        EXPECT_FALSE(list.empty());
        list.erase(list.begin());
    }
    EXPECT_EQ(0, list.size());
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.end(), list.erase(list.end()));
}
