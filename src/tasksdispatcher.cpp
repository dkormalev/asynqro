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
#include "asynqro/impl/containers_traverse.h"
#include "asynqro/impl/spinlock.h"
#include "asynqro/impl/taskslist_p.h"
#include "asynqro/tasks.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

static const int32_t INTENSIVE_CAPACITY = static_cast<int32_t>(std::max(std::thread::hardware_concurrency(), 1u));
static const int32_t DEFAULT_CUSTOM_CAPACITY = INTENSIVE_CAPACITY;
static const int32_t DEFAULT_TOTAL_CAPACITY = std::max(64, INTENSIVE_CAPACITY * 8);
static const int32_t DEFAULT_BOUND_CAPACITY = DEFAULT_TOTAL_CAPACITY / 4;

namespace asynqro::tasks {
static constexpr uint64_t INTENSIVE_SUBPOOL = packPoolInfo(TaskType::Intensive, 0);

class Worker;
class TasksDispatcherPrivate
{
    friend class TasksDispatcher;

public:
    void taskFinished(int32_t workerId, const TaskInfo &task, bool askingForNext);

private:
    void schedule(int32_t workerId = -1) noexcept;
    // All private methods below should always be called under mainLock
    bool createNewWorkerIfPossible() noexcept;
    bool scheduleSingleTask(const TaskInfo &task, int32_t workerId) noexcept;

    int32_t customTagCapacity(int32_t tag) const;

    std::unordered_map<uint64_t, int32_t> subPoolsUsage; // pool info -> amount
    std::unordered_map<int32_t, int32_t> customTagCapacities; // tag -> capacity

    TasksList tasksQueue; // All except bound ones to known workers

    std::vector<Worker *> allWorkers;
    std::unordered_set<int32_t> availableWorkers; // Indices in allWorkers vector, except bound

    std::unordered_map<int32_t, int32_t> tagToWorkerBindings; // tag -> index in allWorkers vector
    std::unordered_map<int32_t, int> workersBindingsCount; // Index in allWorkers vector -> amount of tags bound

    TasksDispatcher *q_ptr = nullptr;

    int32_t capacity = DEFAULT_TOTAL_CAPACITY;
    int32_t boundCapacity = DEFAULT_BOUND_CAPACITY;
    detail::SpinLock mainLock;
    std::atomic_bool poisoningStarted{false};

public:
    std::atomic_int_fast32_t instantUsage{0};
    std::atomic_int_fast32_t idleLoopsAmount{1024};
};

class Worker
{
public:
    explicit Worker(int32_t id);
    Worker(const Worker &) = delete;
    Worker(Worker &&) noexcept = delete;
    Worker &operator=(const Worker &) = delete;
    Worker &operator=(Worker &&) noexcept = delete;
    ~Worker();

    void start();

    void addTask(TaskInfo &&task) noexcept;
    void poisonPill();

protected:
    void run();

private:
    std::mutex waitingLock;
    std::condition_variable waiter;
    TasksList workerTasks;
    int32_t id = 0;
    int_fast32_t idleLoopsAmount = 0;
    std::thread myself;

    std::atomic_bool poisoned{false};
    detail::SpinLock tasksLock;
};

TasksDispatcher::TasksDispatcher() : d_ptr(new TasksDispatcherPrivate)
{
    d_ptr->q_ptr = this; // lgtm [cpp/stack-address-escape]
    d_ptr->allWorkers.reserve(static_cast<size_t>(capacity()));
    d_ptr->customTagCapacities[0] = d_ptr->capacity;
}

TasksDispatcher::~TasksDispatcher()
{
    d_ptr->poisoningStarted.store(true, std::memory_order_relaxed);
    detail::SpinLockHolder lock(&d_ptr->mainLock);
    for (auto worker : d_ptr->allWorkers)
        delete worker;
    d_ptr->allWorkers.clear();
}

TasksDispatcher *TasksDispatcher::instance() noexcept
{
    static TasksDispatcher tasksDispatcher;
    return &tasksDispatcher;
}

int32_t TasksDispatcher::capacity() const
{
    return d_ptr->capacity;
}

int32_t TasksDispatcher::subPoolCapacity(TaskType type, int32_t tag) const
{
    if (type == TaskType::ThreadBound)
        return d_ptr->boundCapacity;
    if (type == TaskType::Intensive)
        return INTENSIVE_CAPACITY;
    if (tag <= 0)
        return capacity();
    detail::SpinLockHolder lock(&d_ptr->mainLock, d_ptr->poisoningStarted);
    return d_ptr->customTagCapacity(tag);
}

void TasksDispatcher::setCapacity(int32_t capacity)
{
    detail::SpinLockHolder lock(&d_ptr->mainLock, d_ptr->poisoningStarted);
    if (!lock.isLocked())
        return;
    capacity = std::max(INTENSIVE_CAPACITY, capacity);
    capacity = std::max(static_cast<int32_t>(d_ptr->allWorkers.size()), capacity);
    d_ptr->capacity = capacity;
    d_ptr->allWorkers.reserve(static_cast<size_t>(capacity));
    d_ptr->customTagCapacities[0] = capacity;
    d_ptr->boundCapacity = std::min(d_ptr->boundCapacity, capacity);
}

void TasksDispatcher::addCustomTag(int32_t tag, int32_t capacity)
{
    if (tag <= 0)
        return;
    capacity = std::clamp(capacity, 1, d_ptr->capacity);
    detail::SpinLockHolder lock(&d_ptr->mainLock, d_ptr->poisoningStarted);
    if (!lock.isLocked())
        return;
    d_ptr->customTagCapacities[tag] = capacity;
}

void TasksDispatcher::setBoundCapacity(int32_t capacity)
{
    detail::SpinLockHolder lock(&d_ptr->mainLock, d_ptr->poisoningStarted);
    if (!lock.isLocked())
        return;
    d_ptr->boundCapacity = std::max(static_cast<int32_t>(d_ptr->workersBindingsCount.size()), capacity);
}

int_fast32_t TasksDispatcher::idleLoopsAmount() const
{
    return d_ptr->idleLoopsAmount.load(std::memory_order_relaxed);
}

void TasksDispatcher::setIdleLoopsAmount(int_fast32_t amount)
{
    d_ptr->idleLoopsAmount.store(amount, std::memory_order_relaxed);
}

int_fast32_t TasksDispatcher::instantUsage() const
{
    return d_ptr->instantUsage.load(std::memory_order_relaxed);
}

void TasksDispatcher::preHeatPool(double amount)
{
    int32_t desiredCapacity = std::clamp(static_cast<int32_t>(std::round(amount * capacity())), 1, capacity());
    detail::SpinLockHolder lock(&d_ptr->mainLock, d_ptr->poisoningStarted);
    if (!lock.isLocked())
        return;
    while (desiredCapacity > static_cast<int32_t>(d_ptr->allWorkers.size()))
        d_ptr->createNewWorkerIfPossible();
}

void TasksDispatcher::preHeatIntensivePool()
{
    detail::SpinLockHolder lock(&d_ptr->mainLock, d_ptr->poisoningStarted);
    if (!lock.isLocked())
        return;
    while (INTENSIVE_CAPACITY > static_cast<int32_t>(d_ptr->allWorkers.size()))
        d_ptr->createNewWorkerIfPossible();
}

void TasksDispatcher::insertTaskInfo(std::function<void()> &&wrappedTask, TaskType type, int32_t tag,
                                     TaskPriority priority) noexcept
{
    // We consider all intensive tasks as under single tag
    tag = type == TaskType::Intensive ? 0 : std::max(0, tag);
    TaskInfo taskInfo = TaskInfo(std::move(wrappedTask), type, tag, priority);
    detail::SpinLockHolder lock(&d_ptr->mainLock, d_ptr->poisoningStarted);
    if (!lock.isLocked())
        return;
    // We keep all already known bindings in separate lists
    if (type == TaskType::ThreadBound) {
        auto boundWorker = d_ptr->tagToWorkerBindings.find(tag);
        if (boundWorker != d_ptr->tagToWorkerBindings.cend()) {
            d_ptr->allWorkers[static_cast<size_t>(boundWorker->second)]->addTask(std::move(taskInfo));
            return;
        }
    } else if (!d_ptr->availableWorkers.empty() && d_ptr->tasksQueue.empty()) {
        int32_t workerId = *d_ptr->availableWorkers.begin();
        if (d_ptr->scheduleSingleTask(taskInfo, workerId)) {
            lock.unlock();
            d_ptr->allWorkers[static_cast<size_t>(workerId)]->addTask(std::move(taskInfo));
            return;
        }
    }

    try {
        d_ptr->tasksQueue.insert(std::move(taskInfo));
    } catch (...) {
        try {
            taskInfo.task();
        } catch (...) {
        }
        return;
    }
    if (!d_ptr->availableWorkers.empty() || static_cast<int32_t>(d_ptr->allWorkers.size()) < capacity()) {
        if (type == TaskType::Intensive && d_ptr->subPoolsUsage[INTENSIVE_SUBPOOL] >= INTENSIVE_CAPACITY)
            return;

        lock.unlock();
        d_ptr->schedule();
    }
}

void TasksDispatcherPrivate::taskFinished(int32_t workerId, const TaskInfo &task, bool askingForNext)
{
    detail::SpinLockHolder lock(&mainLock, poisoningStarted);
    if (!lock.isLocked())
        return;
    if (task.type != TaskType::ThreadBound) {
        uint64_t poolInfo = packPoolInfo(task);
        if (--subPoolsUsage[poolInfo] <= 0) {
            if (task.tag)
                subPoolsUsage.erase(poolInfo);
            else
                subPoolsUsage[poolInfo] = 0;
        }
    }
    if (askingForNext) {
        availableWorkers.insert(workerId);
        lock.unlock();
        schedule(workerId);
    }
}

void TasksDispatcherPrivate::schedule(int32_t workerId) noexcept
{
    detail::SpinLockHolder lock(&mainLock, poisoningStarted);
    if (!lock.isLocked())
        return;
    if (tasksQueue.empty())
        return;
    if (availableWorkers.empty() && !createNewWorkerIfPossible())
        return;

    workerId = workerId < 0 || !availableWorkers.count(workerId) ? *availableWorkers.cbegin() : workerId;

    int32_t boundWorkerId = -1;
    bool newBoundTask = false;
    for (auto it = tasksQueue.begin(); it != tasksQueue.end();) {
        TaskInfo &task = *it;
        // We need to check again for tag binding, it could happen while task was in queue
        if (task.type == TaskType::ThreadBound) {
            // We need to evenly distibute bindings across all workers
            // If we have any non-bound workers yet.
            if (tagToWorkerBindings.count(task.tag)) {
                newBoundTask = false;
                boundWorkerId = tagToWorkerBindings[task.tag];
            } else if (static_cast<int32_t>(workersBindingsCount.size()) < boundCapacity) {
                newBoundTask = true;
                if (workersBindingsCount.count(workerId)) {
                    boundWorkerId = traverse::findIf(
                        availableWorkers, [this](int32_t x) { return !workersBindingsCount.count(x); }, -1);
                    if (boundWorkerId < 0 && createNewWorkerIfPossible())
                        boundWorkerId = static_cast<int32_t>(allWorkers.size()) - 1;
                } else {
                    boundWorkerId = workerId;
                }
            } else {
                newBoundTask = true;
                auto minimizer = [this](int32_t acc, int32_t x) {
                    auto xIt = workersBindingsCount.find(x);
                    auto accIt = workersBindingsCount.find(acc);
                    if (workersBindingsCount.cend() == xIt)
                        return acc;
                    if (workersBindingsCount.cend() == accIt)
                        return x;
                    return accIt->second > xIt->second ? x : acc;
                };
                boundWorkerId = traverse::reduce(availableWorkers, minimizer, -1);
            }
            if (boundWorkerId >= 0) {
                if (newBoundTask) {
                    ++workersBindingsCount[boundWorkerId];
                    tagToWorkerBindings[task.tag] = boundWorkerId;
                }
                availableWorkers.erase(boundWorkerId);
                allWorkers[static_cast<size_t>(boundWorkerId)]->addTask(std::move(task));
                it = tasksQueue.erase(it);
                if (boundWorkerId == workerId)
                    break;
                continue;
            }
        } /* not threadbound */ else if (scheduleSingleTask(task, workerId)) {
            TaskInfo selectedTask = std::move(task);
            tasksQueue.erase(it);
            lock.unlock();
            allWorkers[static_cast<size_t>(workerId)]->addTask(std::move(selectedTask));
            break;
        }
        ++it;
    }
}

bool TasksDispatcherPrivate::createNewWorkerIfPossible() noexcept
{
    int32_t newWorkerId = static_cast<int32_t>(allWorkers.size());
    if (newWorkerId < capacity) {
        try {
            availableWorkers.insert(newWorkerId);
        } catch (...) {
            return false;
        }
        auto worker = new Worker(newWorkerId);
        try {
            allWorkers.push_back(worker);
        } catch (...) {
            delete worker;
            return false;
        }
        worker->start();
        return true;
    }
    return false;
}

bool TasksDispatcherPrivate::scheduleSingleTask(const TaskInfo &task, int32_t workerId) noexcept
{
    if (workerId < 0 || task.type == TaskType::ThreadBound)
        return false;

    int32_t capacityLeft = capacity;

    switch (task.type) {
    case TaskType::Intensive:
        capacityLeft = INTENSIVE_CAPACITY;
        break;
    case TaskType::Custom:
        capacityLeft = customTagCapacity(task.tag);
        break;
    default:
        break;
    }
    uint64_t poolInfo = packPoolInfo(task);
    auto usageIt = subPoolsUsage.find(poolInfo);
    capacityLeft -= subPoolsUsage.cend() == usageIt ? 0 : usageIt->second;
    if (capacityLeft <= 0)
        return false;
    ++subPoolsUsage[poolInfo];
    availableWorkers.erase(workerId);
    return true;
}

int32_t TasksDispatcherPrivate::customTagCapacity(int32_t tag) const
{
    auto capacityIt = customTagCapacities.find(tag);
    return customTagCapacities.cend() == capacityIt ? DEFAULT_CUSTOM_CAPACITY : capacityIt->second;
}

Worker::Worker(int32_t id) : id(id)
{
    idleLoopsAmount = TasksDispatcher::instance()->d_ptr->idleLoopsAmount.load(std::memory_order_relaxed);
}

Worker::~Worker()
{
    poisonPill();
    if (myself.joinable()) {
        try {
            myself.join();
        } catch (...) {
        }
    }
}

void Worker::start()
{
    std::thread t(&Worker::run, this);
    std::swap(myself, t);
}

void Worker::addTask(TaskInfo &&task) noexcept
{
    try {
        detail::SpinLockHolder lock(&tasksLock);
        bool wasEmpty = workerTasks.empty();
        workerTasks.insert(std::move(task));
        if (wasEmpty) {
            std::lock_guard waitLock(waitingLock);
            waiter.notify_all();
        }
    } catch (...) {
    }
    TasksDispatcher::instance()->d_ptr->instantUsage.fetch_add(1, std::memory_order_relaxed);
}

void Worker::poisonPill()
{
    poisoned.store(true, std::memory_order_relaxed);
    std::lock_guard lock(waitingLock);
    waiter.notify_all();
}

void Worker::run()
{
    TaskInfo task;
    bool taskFound = false;
    bool taskObserved = false;
    long long noTasksTicks = 0;
    while (!poisoned.load(std::memory_order_relaxed)) {
        taskFound = false;
        tasksLock.lock();
        if (!workerTasks.empty()) {
            auto first = workerTasks.begin();
            task = std::move(*first);
            workerTasks.erase(first);
            taskFound = true;
            taskObserved = true;
        }
        if (!taskFound) {
            if (taskObserved && ++noTasksTicks < idleLoopsAmount) {
                tasksLock.unlock();
                std::this_thread::yield();
                continue;
            }
            std::unique_lock lock(waitingLock);
            tasksLock.unlock();
            task = TaskInfo();
            waiter.wait(lock);
            idleLoopsAmount = TasksDispatcher::instance()->d_ptr->idleLoopsAmount.load(std::memory_order_relaxed);
            taskObserved = false;
            noTasksTicks = 0;
            continue;
        }
        tasksLock.unlock();
        task.task();
        tasksLock.lock();
        bool askingForNext = workerTasks.empty();
        tasksLock.unlock();
        TasksDispatcher::instance()->d_ptr->taskFinished(id, task, askingForNext);
        TasksDispatcher::instance()->d_ptr->instantUsage.fetch_sub(1, std::memory_order_relaxed);
    }
}

} // namespace asynqro::tasks
