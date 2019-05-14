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

#include <cmath>

namespace asynqro::tasks {
namespace detail {
using namespace asynqro::detail;

template <typename Runner, typename Task, typename C, typename T>
struct SequenceHelper
{
    static constexpr bool isIndexed = std::is_invocable_v<Task, long long, T>;

private:
    using ArgsTuple = std::conditional_t<isIndexed, std::tuple<long long, T>, std::tuple<T>>;

public:
    using RawTaskResult = decltype(std::apply(std::declval<Task>(), ArgsTuple()));

private:
    static constexpr bool rawTaskResultIsVoid = std::is_same_v<RawTaskResult, void>;
    using FinalFailure =
        std::conditional_t<Runner::Info::deferredFailureShouldBeConverted, typename Runner::Info::PlainFailure,
                           detail::FailureTypeIfFuture_T<RawTaskResult, typename Runner::Info::PlainFailure>>;

public:
    using TaskResult = std::conditional_t<rawTaskResultIsVoid, bool, RawTaskResult>;
    using RunResult = std::conditional_t<rawTaskResultIsVoid, Future<bool, FinalFailure>,
                                         Future<ValueTypeIfFuture_T<TaskResult>, FinalFailure>>;
    using SequenceInnerResult = Future<WithInnerType_T<C, ValueTypeIfFuture_T<TaskResult>>, FinalFailure>;
    using SequenceFinalResult = std::conditional_t<rawTaskResultIsVoid, Future<bool, FinalFailure>, SequenceInnerResult>;

    static inline SequenceFinalResult finalizeResult(const SequenceInnerResult &result)
    {
        if constexpr (rawTaskResultIsVoid)
            return result.andThenValue(true);
        else // NOLINT(readability-else-after-return)
            return result;
    }
};

struct DefaultRunnerInfo
{
    using PlainFailure = std::string;
    constexpr static bool deferredFailureShouldBeConverted = false;
};
using DefaultRunner = TaskRunner<DefaultRunnerInfo>;

} // namespace detail

template <typename Runner = detail::DefaultRunner, typename Task, typename = std::enable_if_t<std::is_invocable_v<Task>>>
auto run(Task &&task, TaskType type = TaskType::Intensive, int32_t tag = 0,
         TaskPriority priority = TaskPriority::Regular) noexcept
{
    return Runner::run(std::forward<Task>(task), type, tag, priority);
}

template <typename Runner = detail::DefaultRunner, typename Task, typename = std::enable_if_t<std::is_invocable_v<Task>>>
auto run(TaskType type, int32_t tag, TaskPriority priority, Task &&task) noexcept
{
    return Runner::run(std::forward<Task>(task), type, tag, priority);
}

template <typename Runner = detail::DefaultRunner, typename Task, typename = std::enable_if_t<std::is_invocable_v<Task>>>
auto run(TaskType type, int32_t tag, Task &&task) noexcept
{
    return Runner::run(std::forward<Task>(task), type, tag, TaskPriority::Regular);
}

template <typename Runner = detail::DefaultRunner, typename Task, typename = std::enable_if_t<std::is_invocable_v<Task>>>
auto run(TaskPriority priority, Task &&task) noexcept
{
    return Runner::run(std::forward<Task>(task), TaskType::Intensive, 0, priority);
}

template <typename Runner = detail::DefaultRunner, typename Task, typename = std::enable_if_t<std::is_invocable_v<Task>>>
void runAndForget(Task &&task, TaskType type = TaskType::Intensive, int32_t tag = 0,
                  TaskPriority priority = TaskPriority::Regular) noexcept
{
    return Runner::runAndForget(std::forward<Task>(task), type, tag, priority);
}

template <typename Runner = detail::DefaultRunner, typename Task, typename = std::enable_if_t<std::is_invocable_v<Task>>>
void runAndForget(TaskType type, int32_t tag, TaskPriority priority, Task &&task) noexcept
{
    return Runner::runAndForget(std::forward<Task>(task), type, tag, priority);
}

template <typename Runner = detail::DefaultRunner, typename Task, typename = std::enable_if_t<std::is_invocable_v<Task>>>
void runAndForget(TaskType type, int32_t tag, Task &&task) noexcept
{
    return Runner::runAndForget(std::forward<Task>(task), type, tag, TaskPriority::Regular);
}

template <typename Runner = detail::DefaultRunner, typename Task, typename = std::enable_if_t<std::is_invocable_v<Task>>>
void runAndForget(TaskPriority priority, Task &&task) noexcept
{
    return Runner::runAndForget(std::forward<Task>(task), TaskType::Intensive, 0, priority);
}

template <typename Runner = detail::DefaultRunner, typename C, typename T = detail::InnerType_T<C>, typename Task,
          typename = std::enable_if_t<std::is_invocable_v<Task, T> || std::is_invocable_v<Task, long long, T>>>
auto run(const C &data, Task &&f, TaskType type = TaskType::Intensive, int32_t tag = 0,
         TaskPriority priority = TaskPriority::Regular) noexcept
{
    using Helper = detail::SequenceHelper<Runner, Task, C, T>;

    if (data.empty())
        return Helper::SequenceFinalResult::successful();

    detail::WithInnerType_T<C, typename Helper::RunResult> futures;
    if constexpr (Helper::isIndexed) {
        futures = traverse::map(data, [f = std::forward<Task>(f), type, tag, priority](long long index, const T &x) {
            return Runner::run([index, x, f]() { return f(index, x); }, type, tag, priority).future();
        });
    } else {
        futures = traverse::map(data, [f = std::forward<Task>(f), type, tag, priority](const T &x) {
            return Runner::run([x, f]() { return f(x); }, type, tag, priority).future();
        });
    }

    return Helper::finalizeResult(Helper::RunResult::sequence(futures));
}

template <typename Runner = detail::DefaultRunner, typename C, typename T = detail::InnerType_T<C>, typename Task,
          typename = std::enable_if_t<std::is_invocable_v<Task, T>>>
auto clusteredRun(const C &data, Task &&f, int64_t minClusterSize = 1, TaskType type = TaskType::Intensive,
                  int32_t tag = 0, TaskPriority priority = TaskPriority::Regular) noexcept
{
    C dataCopy = data;
    return clusteredRun<Runner>(std::move(dataCopy), std::forward<Task>(f), minClusterSize, type, tag, priority);
}

template <typename Runner = detail::DefaultRunner, typename C, typename T = detail::InnerType_T<C>, typename Task,
          typename Result = detail::WithInnerType_T<C, typename std::invoke_result_t<Task, T>>,
          typename ResultFuture = Future<Result, typename Runner::Info::PlainFailure>>
ResultFuture clusteredRun(C &&data, Task &&f, int64_t minClusterSize = 1, TaskType type = TaskType::Intensive,
                          int32_t tag = 0, TaskPriority priority = TaskPriority::Regular) noexcept
{
    if (data.empty())
        return ResultFuture::successful();
    if (minClusterSize <= 0)
        minClusterSize = 1;

    return Runner::run(
        [data = std::forward<C>(data), f = std::forward<Task>(f), minClusterSize, type, tag, priority]() -> Result {
            int64_t amount = data.size();
            int32_t capacity = TasksDispatcher::instance()->subPoolCapacity(type, tag);
            capacity = std::min(static_cast<int32_t>(ceil(amount / static_cast<double>(minClusterSize))), capacity);
            int32_t clusterSize = static_cast<int32_t>(amount / capacity);
            --capacity; // Last cluster will be processed in managing thread
            Result result;
            result.resize(amount);
            std::vector<Future<bool, typename Runner::Info::PlainFailure>> futures;
            futures.reserve(static_cast<uint32_t>(capacity));
            for (int32_t job = 0; job < capacity; ++job) {
                futures.push_back(Runner::run(
                    [&data, &f, &result, job, clusterSize]() {
                        const auto right = (job + 1) * clusterSize;
                        for (int32_t i = job * clusterSize; i < right && !detail::hasLastFailure(); ++i)
                            result[i] = f(data[i]);
                    },
                    type, tag, priority));
            }
            typename Runner::Info::PlainFailure localFailure;
            bool localFailureHappened = false;
            try {
                for (int32_t i = capacity * clusterSize; i < amount && !detail::hasLastFailure(); ++i)
                    result[i] = f(data[i]);
                localFailure = detail::lastFailure<typename Runner::Info::PlainFailure>();
                localFailureHappened = detail::hasLastFailure();
            } catch (const std::exception &e) {
                localFailure = detail::exceptionFailure<typename Runner::Info::PlainFailure>(e);
                localFailureHappened = true;
            } catch (...) {
                localFailure = detail::exceptionFailure<typename Runner::Info::PlainFailure>();
                localFailureHappened = true;
            }
            detail::invalidateLastFailure();
            for (const auto &future : futures)
                future.wait();
            if (localFailureHappened)
                return WithFailure<typename Runner::Info::PlainFailure>(localFailure);
            for (const auto &future : futures) {
                if (future.isFailed())
                    return WithFailure<typename Runner::Info::PlainFailure>(future.failureReason());
            }
            return result;
        },
        type, tag, priority);
}

} // namespace asynqro::tasks

#endif // ASYNQRO_TASKS_H
