/* Copyright 2019, Denis Kormalev
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *     * Neither the name of the copyright holders nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef ASYNQRO_TASKSLIST_P_H
#define ASYNQRO_TASKSLIST_P_H

#include "asynqro/impl/tasksdispatcher.h"

#include <algorithm>
#include <list>
#include <map>

namespace asynqro::tasks {

struct TaskInfo
{
    static TaskInfo &empty()
    {
        static TaskInfo x;
        return x;
    }
    TaskInfo() noexcept {}
    TaskInfo(std::function<void()> &&task, TaskType type, int32_t tag, TaskPriority priority)
        : task(std::move(task)), tag(tag), type(type), priority(priority)
    {}
    ~TaskInfo() = default;
    TaskInfo(const TaskInfo &) = delete;
    TaskInfo &operator=(const TaskInfo &) = delete;
    //TODO: C++20: mark as noexcept
    TaskInfo(TaskInfo &&) = default;
    TaskInfo &operator=(TaskInfo &&) = default;
    bool isValid() const noexcept { return static_cast<bool>(task); }
    std::function<void()> task;
    int32_t tag = 0;
    TaskType type = TaskType::Intensive;
    TaskPriority priority = TaskPriority::Regular;
};

constexpr uint64_t packPoolInfo(TaskType type, int32_t tag)
{
    return (static_cast<uint64_t>(type) << 32u) | static_cast<uint64_t>(std::max(0, tag));
}
uint64_t packPoolInfo(const TaskInfo &taskInfo)
{
    return packPoolInfo(taskInfo.type, taskInfo.tag);
}

class TasksList
{
    using List = std::list<TaskInfo>;
    using Map = std::map<uint_fast8_t, List>;

public:
    struct iterator
    {
        iterator &operator++()
        {
            if (mapIt == owner->m_lists.end())
                return *this;
            ++listIt;
            if (listIt == mapIt->second.end()) {
                ++mapIt;
                if (mapIt != owner->m_lists.end())
                    listIt = mapIt->second.begin();
            }
            return *this;
        }
        bool operator==(const iterator &other) const
        {
            return owner == other.owner && mapIt == other.mapIt
                   && (mapIt == owner->m_lists.end() || listIt == other.listIt);
        }
        bool operator!=(const iterator &other) const { return !(*this == other); }
        TaskInfo &operator*() const
        {
            if (mapIt == owner->m_lists.end())
                return TaskInfo::empty();
            const List &list = mapIt->second;
            if (listIt == list.end())
                return TaskInfo::empty();
            return *listIt;
        }
        TaskInfo *operator->() const { return &operator*(); }

        const TasksList *owner;
        Map::iterator mapIt;
        List::iterator listIt;
    };

    void insert(std::function<void()> &&task, TaskType type, int32_t tag, TaskPriority priority)
    {
        m_lists[priority].emplace_back(std::move(task), type, tag, priority);
        ++m_size;
    }

    void insert(TaskInfo &&taskInfo)
    {
        m_lists[taskInfo.priority].push_back(std::move(taskInfo));
        ++m_size;
    }

    iterator erase(const iterator &it)
    {
        if (it.mapIt != m_lists.end()) {
            List &list = m_lists[it.mapIt->first];
            if (it.listIt != list.end()) {
                --m_size;
                iterator newIt = it;
                newIt.listIt = list.erase(newIt.listIt);
                if (newIt.listIt == list.end())
                    ++newIt.mapIt;
                return newIt;
            }
        }
        return it;
    }
    bool empty() const { return !size(); }
    size_t size() const { return m_size; }

    iterator begin()
    {
        if (!empty()) {
            for (auto mapIt = m_lists.begin(); mapIt != m_lists.end(); ++mapIt) {
                if (!mapIt->second.empty())
                    return iterator{this, mapIt, mapIt->second.begin()};
            }
        }
        return end();
    }

    iterator end() { return iterator{this, m_lists.end(), List::iterator()}; }

private:
    Map m_lists;
    size_t m_size = 0;
};
} // namespace asynqro::tasks
#endif // ASYNQRO_TASKSLIST_P_H
