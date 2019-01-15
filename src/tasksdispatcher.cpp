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

#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>

static const qint32 INTENSIVE_CAPACITY = qMax(QThread::idealThreadCount(), 1);
static const qint32 DEFAULT_CUSTOM_CAPACITY = INTENSIVE_CAPACITY;
static const qint32 DEFAULT_TOTAL_CAPACITY = qMax(64, INTENSIVE_CAPACITY * 8);
static const qint32 DEFAULT_BOUND_CAPACITY = DEFAULT_TOTAL_CAPACITY / 4;

namespace asynqro::tasks {
static constexpr qint64 INTENSIVE_SUBPOOL = packPoolInfo(TaskType::Intensive, 0);

class Worker;
class TasksDispatcherPrivate
{
    Q_DECLARE_PUBLIC(TasksDispatcher)
public:
    void taskFinished(qint32 workerId, const TaskInfo &task, bool askingForNext);

private:
    void schedule(qint32 workerId = -1);
    // All private methods below should always be called under mainLock
    bool createNewWorkerIfPossible();
    bool scheduleSingleTask(const TaskInfo &task, qint32 workerId);

    qint32 customTagCapacity(qint32 tag) const;

    std::unordered_map<quint64, qint32> subPoolsUsage; // pool info -> amount
    std::unordered_map<qint32, qint32> customTagCapacities; // tag -> capacity

    TasksList tasksQueue; // All except bound ones to known workers

    std::vector<Worker *> allWorkers;
    std::unordered_set<qint32> availableWorkers; // Indices in allWorkers vector, except bound

    std::unordered_map<qint32, qint32> tagToWorkerBindings; // tag -> index in allWorkers vector
    std::unordered_map<qint32, int> workersBindingsCount; // Index in allWorkers vector -> amount of tags bound

    TasksDispatcher *q_ptr;

    qint32 capacity = DEFAULT_TOTAL_CAPACITY;
    qint32 boundCapacity = DEFAULT_BOUND_CAPACITY;
    detail::SpinLock mainLock;

public:
    std::atomic_int_fast32_t instantUsage{0};
    std::atomic_int_fast32_t idleLoopsAmount{1024};
};

class Worker : public QThread
{
    Q_OBJECT

public:
    Worker(qint32 id);
    void addTask(TaskInfo &&task);
    void poisonPill();

protected:
    void run() override;

private:
    std::mutex waitingLock;
    std::condition_variable waiter;
    TasksList workerTasks;
    qint32 id = 0;
    qint32 idleLoopsAmount = 0;

    std::atomic_bool poisoned{false};
    detail::SpinLock tasksLock;
};

TasksDispatcher::TasksDispatcher() : d_ptr(new TasksDispatcherPrivate)
{
    d_ptr->q_ptr = this;
    d_ptr->allWorkers.reserve(static_cast<size_t>(capacity()));
    d_ptr->customTagCapacities[0] = d_ptr->capacity;
}

TasksDispatcher::~TasksDispatcher()
{
    detail::SpinLockHolder lock(&d_ptr->mainLock);
    for (auto worker : d_ptr->allWorkers)
        worker->poisonPill();
    for (auto worker : d_ptr->allWorkers)
        worker->wait(100);
    for (auto worker : d_ptr->allWorkers) {
        if (worker->isRunning()) {
            worker->terminate();
            worker->wait(100);
        }
        delete worker;
    }
    d_ptr->allWorkers.clear();
}

TasksDispatcher *TasksDispatcher::instance()
{
    static TasksDispatcher tasksDispatcher;
    return &tasksDispatcher;
}

qint32 TasksDispatcher::capacity() const
{
    return d_ptr->capacity;
}

qint32 TasksDispatcher::subPoolCapacity(TaskType type, qint32 tag) const
{
    if (type == TaskType::ThreadBound)
        return d_ptr->boundCapacity;
    if (type == TaskType::Intensive)
        return INTENSIVE_CAPACITY;
    else if (tag <= 0)
        return capacity();
    detail::SpinLockHolder lock(&d_ptr->mainLock);
    return d_ptr->customTagCapacity(tag);
}

void TasksDispatcher::setCapacity(qint32 capacity)
{
    detail::SpinLockHolder lock(&d_ptr->mainLock);
    capacity = qMax(INTENSIVE_CAPACITY, capacity);
    capacity = qMax(static_cast<qint32>(d_ptr->allWorkers.size()), capacity);
    d_ptr->capacity = capacity;
    d_ptr->allWorkers.reserve(static_cast<size_t>(capacity));
    d_ptr->customTagCapacities[0] = capacity;
    d_ptr->boundCapacity = qMin(d_ptr->boundCapacity, capacity);
}

void TasksDispatcher::addCustomTag(qint32 tag, qint32 capacity)
{
    if (tag <= 0)
        return;
    capacity = qBound(1, capacity, d_ptr->capacity);
    detail::SpinLockHolder lock(&d_ptr->mainLock);
    d_ptr->customTagCapacities[tag] = capacity;
}

void TasksDispatcher::setBoundCapacity(qint32 capacity)
{
    detail::SpinLockHolder lock(&d_ptr->mainLock);
    d_ptr->boundCapacity = qMax(static_cast<qint32>(d_ptr->workersBindingsCount.size()), capacity);
}

qint32 TasksDispatcher::idleLoopsAmount() const
{
    return d_ptr->idleLoopsAmount.load(std::memory_order_relaxed);
}

void TasksDispatcher::setIdleLoopsAmount(qint32 amount)
{
    d_ptr->idleLoopsAmount.store(amount, std::memory_order_relaxed);
}

qint32 TasksDispatcher::instantUsage() const
{
    return d_ptr->instantUsage.load(std::memory_order_relaxed);
}

void TasksDispatcher::preHeatPool(double amount)
{
    qint32 desiredCapacity = qBound(1, static_cast<qint32>(qRound(amount * capacity())), capacity());
    detail::SpinLockHolder lock(&d_ptr->mainLock);
    while (desiredCapacity > static_cast<qint32>(d_ptr->allWorkers.size()))
        d_ptr->createNewWorkerIfPossible();
}

void TasksDispatcher::preHeatIntensivePool()
{
    detail::SpinLockHolder lock(&d_ptr->mainLock);
    while (INTENSIVE_CAPACITY > static_cast<qint32>(d_ptr->allWorkers.size()))
        d_ptr->createNewWorkerIfPossible();
}

void TasksDispatcher::insertTaskInfo(std::function<void()> &&wrappedTask, TaskType type, qint32 tag,
                                     TaskPriority priority) noexcept
{
    // We consider all intensive tasks as under single tag
    tag = type == TaskType::Intensive ? 0 : qMax(0, tag);
    TaskInfo taskInfo = TaskInfo(std::move(wrappedTask), type, tag, priority);
    detail::SpinLockHolder lock(&d_ptr->mainLock);
    // We keep all already known bindings in separate lists
    if (type == TaskType::ThreadBound) {
        auto boundWorker = d_ptr->tagToWorkerBindings.find(tag);
        if (boundWorker != d_ptr->tagToWorkerBindings.cend()) {
            d_ptr->allWorkers[static_cast<size_t>(boundWorker->second)]->addTask(std::move(taskInfo));
            return;
        }
    } else if (!d_ptr->availableWorkers.empty() && d_ptr->tasksQueue.empty()) {
        qint32 workerId = *d_ptr->availableWorkers.begin();
        if (d_ptr->scheduleSingleTask(taskInfo, workerId)) {
            lock.unlock();
            d_ptr->allWorkers[static_cast<size_t>(workerId)]->addTask(std::move(taskInfo));
            return;
        }
    }

    d_ptr->tasksQueue.insert(std::move(taskInfo));
    if (!d_ptr->availableWorkers.empty() || static_cast<qint32>(d_ptr->allWorkers.size()) < capacity()) {
        if (type == TaskType::Intensive && d_ptr->subPoolsUsage[INTENSIVE_SUBPOOL] >= INTENSIVE_CAPACITY)
            return;

        lock.unlock();
        d_ptr->schedule();
    }
}

void TasksDispatcherPrivate::taskFinished(qint32 workerId, const TaskInfo &task, bool askingForNext)
{
    detail::SpinLockHolder lock(&mainLock);
    if (task.type != TaskType::ThreadBound) {
        quint64 poolInfo = packPoolInfo(task);
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

void TasksDispatcherPrivate::schedule(qint32 workerId)
{
    detail::SpinLockHolder lock(&mainLock);
    if (tasksQueue.empty())
        return;
    if (availableWorkers.empty() && !createNewWorkerIfPossible())
        return;

    workerId = workerId < 0 || !availableWorkers.count(workerId) ? *availableWorkers.cbegin() : workerId;

    qint32 boundWorkerId = -1;
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
            } else if (static_cast<qint32>(workersBindingsCount.size()) < boundCapacity) {
                newBoundTask = true;
                if (workersBindingsCount.count(workerId)) {
                    boundWorkerId = traverse::findIf(availableWorkers,
                                                     [this](qint32 x) { return !workersBindingsCount.count(x); }, -1);
                } else {
                    boundWorkerId = workerId;
                }
            } else {
                newBoundTask = true;
                auto minimizer = [this](qint32 acc, qint32 x) {
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

bool TasksDispatcherPrivate::createNewWorkerIfPossible()
{
    qint32 newWorkerId = static_cast<qint32>(allWorkers.size());
    if (newWorkerId < capacity) {
        availableWorkers.insert(newWorkerId);
        auto worker = new Worker(newWorkerId);
        allWorkers.push_back(worker);
        worker->start();
        return true;
    }
    return false;
}

bool TasksDispatcherPrivate::scheduleSingleTask(const TaskInfo &task, qint32 workerId)
{
    if (workerId < 0 || task.type == TaskType::ThreadBound)
        return false;

    qint32 capacityLeft = capacity;

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
    quint64 poolInfo = packPoolInfo(task);
    auto usageIt = subPoolsUsage.find(poolInfo);
    capacityLeft -= subPoolsUsage.cend() == usageIt ? 0 : usageIt->second;
    if (capacityLeft <= 0)
        return false;
    ++subPoolsUsage[poolInfo];
    availableWorkers.erase(workerId);
    return true;
}

qint32 TasksDispatcherPrivate::customTagCapacity(qint32 tag) const
{
    auto capacityIt = customTagCapacities.find(tag);
    return customTagCapacities.cend() == capacityIt ? DEFAULT_CUSTOM_CAPACITY : capacityIt->second;
}

Worker::Worker(qint32 id) : QThread(nullptr), id(id)
{
    idleLoopsAmount = TasksDispatcher::instance()->d_ptr->idleLoopsAmount.load(std::memory_order_relaxed);
}

void Worker::addTask(TaskInfo &&task)
{
    tasksLock.lock();
    bool wasEmpty = workerTasks.empty();
    workerTasks.insert(std::move(task));
    if (wasEmpty) {
        std::lock_guard lock(waitingLock);
        waiter.notify_all();
    }
    tasksLock.unlock();
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
                QThread::yieldCurrentThread();
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

#include "tasksdispatcher.moc"
