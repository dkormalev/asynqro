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

// Normally this file shouldn't be included directly. asynqro/tasks.h already has it included
// Moved to separate header only to keep files smaller
#ifndef ASYNQRO_TASKS_DISPATCHER_H
#define ASYNQRO_TASKS_DISPATCHER_H

#include "asynqro/future.h"
#include "asynqro/impl/asynqro_export.h"
#include "asynqro/impl/typetraits.h"

namespace asynqro::tasks {
namespace detail {
using namespace asynqro::detail;
template <typename T>
using ValueTypeIfFuture_T = ValueTypeIfNeeded_T<detail::IsSpecialization_V<T, Future>, T>;

template <bool returnInner, typename T, typename DefaultFailure>
struct FailureTypeIfFuture;

template <typename T, typename DefaultFailure>
struct FailureTypeIfFuture<true, T, DefaultFailure>
{
    using type = typename T::Failure;
};

template <typename T, typename DefaultFailure>
struct FailureTypeIfFuture<false, T, DefaultFailure>
{
    using type = DefaultFailure;
};

template <typename T, typename DefaultFailure>
using FailureTypeIfFuture_T = typename FailureTypeIfFuture<detail::IsSpecialization_V<T, Future>, T, DefaultFailure>::type;

} // namespace detail

enum class TaskType : uint8_t
{
    Custom = 0,
    Intensive = 1,
    ThreadBound = 2
};

enum TaskPriority : uint8_t
{
    Emergency = 0x0,
    Regular = 0x0F,
    Background = 0xFF
};

class TasksDispatcherPrivate;
class Worker;
class ASYNQRO_EXPORT TasksDispatcher
{
public:
    static TasksDispatcher *instance();
    TasksDispatcher(const TasksDispatcher &) = delete;
    TasksDispatcher(const TasksDispatcher &&) = delete;
    TasksDispatcher &operator=(const TasksDispatcher &) = delete;
    TasksDispatcher &operator=(const TasksDispatcher &&) = delete;

    int32_t capacity() const;
    int32_t subPoolCapacity(TaskType type, int32_t tag = 0) const;
    void setCapacity(int32_t capacity);
    void addCustomTag(int32_t tag, int32_t capacity);
    void setBoundCapacity(int32_t capacity);

    int_fast32_t idleLoopsAmount() const;
    void setIdleLoopsAmount(int_fast32_t amount);

    int_fast32_t instantUsage() const;

    void preHeatPool(double amount = 1.0);
    void preHeatIntensivePool();

private:
    friend class TasksDispatcherPrivate;
    friend class Worker;
    template <typename FailureType>
    friend struct TaskRunner;
    TasksDispatcher();
    ~TasksDispatcher();
    void insertTaskInfo(std::function<void()> &&wrappedTask, TaskType type, int32_t tag, TaskPriority priority) noexcept;

    std::unique_ptr<TasksDispatcherPrivate> d_ptr;
};

/*
 * struct RunnerInfo {
 *      type PlainFailure;
 *
 *      constexpr static bool deferredFailureShouldBeConverted;
 *
 *      //Not needed if deferredFailureShouldBeConverted == false
 *      template <typename DeferredFailure>
 *      static PlainFailure toPlainFailure(const DeferredFailure &deferred);
 * };
 */

template <typename RunnerInfo>
struct TaskRunner
{
    using Info = RunnerInfo;
    template <typename Task>
    static auto run(Task &&task, TaskType type, int32_t tag, TaskPriority priority) noexcept
    {
        using RawResult = typename std::invoke_result_t<Task>;
        using NonVoidResult = detail::ValueTypeIfFuture_T<RawResult>;
        using FinalFailure =
            std::conditional_t<RunnerInfo::deferredFailureShouldBeConverted, typename RunnerInfo::PlainFailure,
                               detail::FailureTypeIfFuture_T<RawResult, typename RunnerInfo::PlainFailure>>;
        Promise<std::conditional_t<std::is_same_v<RawResult, void>, bool, NonVoidResult>, FinalFailure> promise;

        //TODO: MSVC2019+: move constexpr if back into lambda when MSFT will fix their issue with constexpr in lambdas
        if constexpr (detail::IsSpecialization_V<RawResult, Future> && RunnerInfo::deferredFailureShouldBeConverted) {
            std::function<void()> f = [promise, task = std::forward<Task>(task)]() noexcept
            {
                if (promise.isFilled())
                    return;
                detail::invalidateLastFailure();
                try {
                    task()
                        .onSuccess([promise](const auto &result) noexcept { promise.success(result); })
                        .onFailure([promise](const auto &failure) noexcept {
                            promise.failure(RunnerInfo::toPlainFailure(failure));
                        });
                } catch (const std::exception &e) {
                    promise.failure(detail::exceptionFailure<FinalFailure>(e));
                } catch (...) {
                    promise.failure(detail::exceptionFailure<FinalFailure>());
                }
            };
            TasksDispatcher::instance()->insertTaskInfo(std::move(f), type, tag, priority);
        } else if constexpr (detail::IsSpecialization_V<RawResult, Future>) {
            // !TaskRunnerDescriptor::deferredFailureShouldBeConverted
            std::function<void()> f = [promise, task = std::forward<Task>(task)]() noexcept
            {
                if (promise.isFilled())
                    return;
                detail::invalidateLastFailure();
                try {
                    task()
                        .onSuccess([promise](const auto &result) noexcept { promise.success(result); })
                        .onFailure([promise](const FinalFailure &failure) noexcept { promise.failure(failure); });
                } catch (const std::exception &e) {
                    promise.failure(detail::exceptionFailure<FinalFailure>(e));
                } catch (...) {
                    promise.failure(detail::exceptionFailure<FinalFailure>());
                }
            };
            TasksDispatcher::instance()->insertTaskInfo(std::move(f), type, tag, priority);
        } else if constexpr (std::is_same_v<RawResult, void>) {
            std::function<void()> f = [promise, task = std::forward<Task>(task)]() noexcept
            {
                if (promise.isFilled())
                    return;
                detail::invalidateLastFailure();
                try {
                    task();
                    promise.success(true);
                } catch (const std::exception &e) {
                    promise.failure(detail::exceptionFailure<FinalFailure>(e));
                } catch (...) {
                    promise.failure(detail::exceptionFailure<FinalFailure>());
                }
            };
            TasksDispatcher::instance()->insertTaskInfo(std::move(f), type, tag, priority);
        } else {
            std::function<void()> f = [promise, task = std::forward<Task>(task)]() noexcept
            {
                if (promise.isFilled())
                    return;
                detail::invalidateLastFailure();
                try {
                    promise.success(task());
                } catch (const std::exception &e) {
                    promise.failure(detail::exceptionFailure<FinalFailure>(e));
                } catch (...) {
                    promise.failure(detail::exceptionFailure<FinalFailure>());
                }
            };
            TasksDispatcher::instance()->insertTaskInfo(std::move(f), type, tag, priority);
        }
        return CancelableFuture<>::create(promise);
    }

    template <typename Task>
    static void runAndForget(Task &&task, TaskType type, int32_t tag, TaskPriority priority) noexcept
    {
        TasksDispatcher::instance()->insertTaskInfo(
            [task = std::forward<Task>(task)]() noexcept {
                try {
                    task();
                } catch (...) {
                }
            },
            type, tag, priority);
    }
};

} // namespace asynqro::tasks

#endif // ASYNQRO_TASKS_DISPATCHER_H
