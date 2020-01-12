#include "asynqro/impl/taskslist_p.h"

#include "gtest/gtest.h"

using namespace asynqro::tasks;

using TraverseOrderTestData = std::tuple<std::vector<uint8_t>, std::vector<int>>;
class TraverseOrderTasksListTest : public testing::TestWithParam<TraverseOrderTestData>
{};

using RemovalTestInnerData = std::tuple<int, int, std::vector<int>>;
using RemovalTestData = std::tuple<int, std::vector<RemovalTestInnerData>>;
class RemovalTasksListTest : public testing::TestWithParam<RemovalTestData>
{};

class SizesTasksListTest : public testing::TestWithParam<int>
{};

// clang-format off
INSTANTIATE_TEST_SUITE_P(
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
    std::vector<uint8_t> insert = std::get<0>(GetParam());
    std::vector<int> expected = std::get<1>(GetParam());
    ASSERT_EQ(expected.size(), insert.size());
    std::vector<int> result;
    TasksList list;
    for (int i = 0; i < static_cast<int>(insert.size()); ++i)
        list.insert([&result, i]() { result.push_back(i); }, TaskType::Custom, 0,
                    static_cast<TaskPriority>(insert[static_cast<size_t>(i)]));
    for (const auto &taskInfo : list)
        taskInfo.task();
    EXPECT_EQ(expected, result);
}

INSTANTIATE_TEST_SUITE_P(IncrementTasksTestParameters, SizesTasksListTest, testing::Values(0, 1, 2, 5, 10));
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
INSTANTIATE_TEST_SUITE_P(RemovalTasksTestParameters, RemovalTasksListTest,
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
    std::vector<int> result;
    for (int i = 0; i < size; ++i)
        list.insert([&result, i]() { result.push_back(i); }, TaskType::Custom, i, TaskPriority::Regular);

    std::vector<RemovalTestInnerData> testData = std::get<1>(GetParam());
    EXPECT_EQ(size, list.size());
    for (const auto &[del, next, others] : testData) {
        ASSERT_EQ(list.size() - 1, others.size());
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
        EXPECT_EQ(list.size(), others.size());
        result.clear();
        for (const auto &taskInfo : list)
            taskInfo.task();
        EXPECT_EQ(others, result);
    }
}

TEST(TasksListTest, sizeAndEmptiness)
{
    int n = 100;
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

TEST(TasksListTest, sizeAndEmptinessWithDifferentPriorities)
{
    int n = 100;
    TasksList list;
    EXPECT_EQ(0, list.size());
    EXPECT_TRUE(list.empty());

    for (int i = 0; i < n; ++i) {
        TaskPriority priority = TaskPriority::Regular;
        if (!(n % 5))
            priority = TaskPriority::Background;
        if (!(n % 7))
            priority = TaskPriority::Emergency;
        list.insert([]() {}, TaskType::Custom, 0, priority);
        EXPECT_EQ(i + 1, list.size());
        EXPECT_FALSE(list.empty());
    }

    auto iterations = 0;
    auto it = list.begin();
    for (int i = n; it != list.end(); --i) {
        EXPECT_EQ(i, list.size());
        EXPECT_FALSE(list.empty());
        it = list.erase(it);
        ++iterations;
        ASSERT_LE(iterations, n);
    }

    for (int i = n; i > 0; --i) {
    }
    EXPECT_EQ(0, list.size());
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(list.end(), list.erase(list.end()));
}

TEST(TasksListTest, differentPrioritiesIteratorIncrement)
{
    TasksList list;
    list.insert([]() {}, TaskType::Custom, 4, TaskPriority::Background);
    list.insert([]() {}, TaskType::Custom, 2, TaskPriority::Regular);
    list.insert([]() {}, TaskType::Custom, 0, TaskPriority::Emergency);
    list.insert([]() {}, TaskType::Custom, 3, TaskPriority::Regular);
    list.insert([]() {}, TaskType::Custom, 1, TaskPriority::Emergency);

    auto it = list.begin();
    for (int i = 0; i < 5; ++i) {
        ASSERT_NE(list.end(), it);
        EXPECT_TRUE(it->isValid());
        EXPECT_NE(&TaskInfo::empty(), &*it);
        EXPECT_EQ(it->tag, i);
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

TEST(TasksListTest, iteratorIncrementOverGaps)
{
    TasksList list;
    list.insert([]() {}, TaskType::Custom, 2, TaskPriority::Background);
    list.insert([]() {}, TaskType::Custom, 4, TaskPriority::Regular);
    list.insert([]() {}, TaskType::Custom, 0, TaskPriority::Emergency);
    list.insert([]() {}, TaskType::Custom, 1, TaskPriority::Emergency);
    list.insert([]() {}, TaskType::Custom, 3, TaskPriority::Background);
    list.insert([]() {}, TaskType::Custom, 4, TaskPriority::Regular);

    auto iterations = 0;
    for (auto it = list.begin(); it != list.end();) {
        if (it->priority == TaskPriority::Regular)
            it = list.erase(it);
        else
            ++it;
        ++iterations;
        ASSERT_LE(iterations, 6);
    }

    auto it = list.begin();
    for (int i = 0; i < list.size(); ++i) {
        ASSERT_NE(list.end(), it);
        EXPECT_TRUE(it->isValid());
        EXPECT_NE(&TaskInfo::empty(), &*it);
        EXPECT_EQ(it->tag, i);
        EXPECT_NE(it->priority, TaskPriority::Regular);
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
