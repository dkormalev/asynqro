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
#ifndef ASYNQRO_TASKS_H
#define ASYNQRO_TASKS_H

#include "asynqro/future.h"
#include "asynqro/impl/containers_traverse.h"
#include "asynqro/impl/tasksdispatcher.h"

#include <QVector>

#include <cmath>

namespace asynqro::tasks {
namespace detail {
using namespace asynqro::detail;

template <typename Task, template <typename...> typename C, typename T>
struct SequenceHelper
{
    static constexpr bool isIndexed = std::is_invocable_v<Task, long long, T>;

private:
    using ArgsTuple = std::conditional_t<isIndexed, std::tuple<long long, T>, std::tuple<T>>;

public:
    using RawTaskResult = decltype(std::apply(std::declval<Task>(), ArgsTuple()));

private:
    static constexpr bool rawTaskResultIsVoid = std::is_same_v<RawTaskResult, void>;

public:
    using TaskResult = std::conditional_t<rawTaskResultIsVoid, bool, RawTaskResult>;
    using RunResult = std::conditional_t<rawTaskResultIsVoid, Future<bool>, Future<ValueTypeIfFuture_T<TaskResult>>>;
    using SequenceInnerResult = Future<C<ValueTypeIfFuture_T<TaskResult>>>;
    using SequenceFinalResult = std::conditional_t<rawTaskResultIsVoid, Future<bool>, SequenceInnerResult>;

    static inline SequenceFinalResult finalizeResult(const SequenceInnerResult &result)
    {
        if constexpr (rawTaskResultIsVoid)
            return result.andThenValue(true);
        else
            return result;
    }
};
} // namespace detail

template <typename Task>
auto run(Task &&task, TaskType type = TaskType::Intensive, qint32 tag = 0,
         TaskPriority priority = TaskPriority::Regular) noexcept
{
    return TasksDispatcher::instance()->run(std::forward<Task>(task), type, tag, priority);
}

template <typename Task>
auto run(TaskType type, qint32 tag, TaskPriority priority, Task &&task) noexcept
{
    return TasksDispatcher::instance()->run(std::forward<Task>(task), type, tag, priority);
}

template <typename Task>
auto run(TaskType type, qint32 tag, Task &&task) noexcept
{
    return TasksDispatcher::instance()->run(std::forward<Task>(task), type, tag, TaskPriority::Regular);
}

template <typename Task>
auto run(TaskPriority priority, Task &&task) noexcept
{
    return TasksDispatcher::instance()->run(std::forward<Task>(task), TaskType::Intensive, 0, priority);
}

template <typename Task>
void runAndForget(Task &&task, TaskType type = TaskType::Intensive, qint32 tag = 0,
                  TaskPriority priority = TaskPriority::Regular) noexcept
{
    return TasksDispatcher::instance()->runAndForget(std::forward<Task>(task), type, tag, priority);
}

template <typename Task>
void runAndForget(TaskType type, qint32 tag, TaskPriority priority, Task &&task) noexcept
{
    return TasksDispatcher::instance()->runAndForget(std::forward<Task>(task), type, tag, priority);
}

template <typename Task>
void runAndForget(TaskType type, qint32 tag, Task &&task) noexcept
{
    return TasksDispatcher::instance()->runAndForget(std::forward<Task>(task), type, tag, TaskPriority::Regular);
}

template <typename Task>
void runAndForget(TaskPriority priority, Task &&task) noexcept
{
    return TasksDispatcher::instance()->runAndForget(std::forward<Task>(task), TaskType::Intensive, 0, priority);
}

template <template <typename...> typename C, typename T, typename Task>
auto run(const C<T> &data, Task &&f, TaskType type = TaskType::Intensive, qint32 tag = 0,
         TaskPriority priority = TaskPriority::Regular) noexcept
{
    using TD = TasksDispatcher;
    using Helper = detail::SequenceHelper<Task, C, T>;

    if (!data.size())
        return Helper::SequenceFinalResult::successful();

    C<typename Helper::RunResult> futures;
    if constexpr (Helper::isIndexed) {
        futures = traverse::map(data, [f = std::forward<Task>(f), type, tag, priority](long long index, const T &x) {
            return TD::instance()->run([index, x, f]() { return f(index, x); }, type, tag, priority).future();
        });
    } else {
        futures = traverse::map(data, [f = std::forward<Task>(f), type, tag, priority](const T &x) {
            return TD::instance()->run([x, f]() { return f(x); }, type, tag, priority).future();
        });
    }

    return Helper::finalizeResult(Future<>::sequence(futures));
}

template <template <typename...> typename C, typename T, typename Task,
          typename Result = C<typename std::invoke_result_t<Task, T>>>
Future<Result> clusteredRun(const C<T> &data, Task &&f, qint64 minClusterSize = 1, TaskType type = TaskType::Intensive,
                            qint32 tag = 0, TaskPriority priority = TaskPriority::Regular) noexcept
{
    if (!data.size())
        return Future<Result>::successful();
    if (minClusterSize <= 0)
        minClusterSize = 1;

    return TasksDispatcher::instance()->run(
        [data, f = std::forward<Task>(f), minClusterSize, type, tag, priority]() -> Result {
            qint64 amount = data.count();
            qint32 capacity = TasksDispatcher::instance()->subPoolCapacity(type, tag);
            capacity = qMin(static_cast<qint32>(ceil(amount / static_cast<double>(minClusterSize))), capacity);
            qint32 clusterSize = static_cast<qint32>(amount / capacity);
            --capacity; // Last cluster will be processed in managing thread
            Result result;
            result.resize(amount);
            QVector<Future<bool>> futures;
            futures.reserve(capacity);
            for (qint32 job = 0; job < capacity; ++job) {
                futures << TasksDispatcher::instance()->run(
                    [&data, &f, &result, job, clusterSize]() {
                        const auto right = (job + 1) * clusterSize + 1;
                        for (qint32 i = job * clusterSize; i < right; ++i)
                            result[i] = f(data[i]);
                    },
                    type, tag, priority);
            }
            for (qint32 i = capacity * clusterSize; i < amount; ++i)
                result[i] = f(data[i]);
            QVariant localFailure = detail::lastFailure();
            detail::invalidateLastFailure();
            for (const auto &future : qAsConst(futures))
                future.wait();
            if (localFailure.isValid())
                return WithFailure(localFailure);
            for (const auto &future : qAsConst(futures)) {
                if (future.isFailed())
                    return WithFailure(future.failureReason());
            }
            return result;
        },
        type, tag, priority);
}

} // namespace asynqro::tasks

#endif // ASYNQRO_TASKS_H
