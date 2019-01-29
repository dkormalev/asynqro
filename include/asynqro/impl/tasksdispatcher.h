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

#include <QtGlobal>

namespace asynqro::tasks {
namespace detail {
using namespace asynqro::detail;
template <typename T>
using ValueTypeIfFuture_T = ValueTypeIfNeeded_T<detail::IsSpecialization_V<T, Future>, T>;
} // namespace detail

enum class TaskType : quint8
{
    Custom = 0,
    Intensive = 1,
    ThreadBound = 2
};

enum TaskPriority : quint8
{
    Emergency = 0x0,
    Regular = 0x0F,
    Background = 0xFF
};

class TasksDispatcherPrivate;
class Worker;
class ASYNQRO_EXPORT TasksDispatcher
{
    Q_DECLARE_PRIVATE(TasksDispatcher)
public:
    static TasksDispatcher *instance();
    TasksDispatcher(const TasksDispatcher &) = delete;
    TasksDispatcher(const TasksDispatcher &&) = delete;
    TasksDispatcher operator=(const TasksDispatcher &) = delete;
    TasksDispatcher operator=(const TasksDispatcher &&) = delete;

    qint32 capacity() const;
    qint32 subPoolCapacity(TaskType type, qint32 tag = 0) const;
    void setCapacity(qint32 capacity);
    void addCustomTag(qint32 tag, qint32 capacity);
    void setBoundCapacity(qint32 capacity);

    qint32 idleLoopsAmount() const;
    void setIdleLoopsAmount(qint32 amount);

    qint32 instantUsage() const;

    void preHeatPool(double amount = 1.0);
    void preHeatIntensivePool();

    template <typename Task>
    auto run(Task &&task, TaskType type, qint32 tag, TaskPriority priority) noexcept
    {
        using RawResult = typename std::invoke_result_t<Task>;
        using NonVoidResult = detail::ValueTypeIfFuture_T<RawResult>;
        Promise<std::conditional_t<std::is_same_v<RawResult, void>, bool, NonVoidResult>> promise;

        //TODO: MSVC2019+: move constexpr if back into lambda when MSFT will fix their issue with constexpr in lambdas
        if constexpr (detail::IsSpecialization_V<RawResult, Future>) {
            std::function<void()> f = [promise, task = std::forward<Task>(task)]() noexcept
            {
                if (promise.isFilled())
                    return;
                detail::invalidateLastFailure();
                try {
                    task()
                        .onSuccess([promise](const auto &result) noexcept { promise.success(result); })
                        .onFailure([promise](const QVariant &failure) noexcept { promise.failure(failure); });
                } catch (const std::exception &e) {
                    promise.failure(detail::exceptionFailure(e));
                } catch (...) {
                    promise.failure(detail::exceptionFailure());
                }
            };
            insertTaskInfo(std::move(f), type, tag, priority);
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
                    promise.failure(detail::exceptionFailure(e));
                } catch (...) {
                    promise.failure(detail::exceptionFailure());
                }
            };
            insertTaskInfo(std::move(f), type, tag, priority);
        } else {
            std::function<void()> f = [promise, task = std::forward<Task>(task)]() noexcept
            {
                if (promise.isFilled())
                    return;
                detail::invalidateLastFailure();
                try {
                    promise.success(task());
                } catch (const std::exception &e) {
                    promise.failure(detail::exceptionFailure(e));
                } catch (...) {
                    promise.failure(detail::exceptionFailure());
                }
            };
            insertTaskInfo(std::move(f), type, tag, priority);
        }
        return CancelableFuture<>::create(promise);
    }

    template <typename Task>
    void runAndForget(Task &&task, TaskType type, qint32 tag, TaskPriority priority) noexcept
    {
        insertTaskInfo(
            [task = std::forward<Task>(task)]() noexcept {
                try {
                    task();
                } catch (...) {
                }
            },
            type, tag, priority);
    }

private:
    friend class Worker;
    TasksDispatcher();
    ~TasksDispatcher();
    void insertTaskInfo(std::function<void()> &&wrappedTask, TaskType type, qint32 tag, TaskPriority priority) noexcept;

    QScopedPointer<TasksDispatcherPrivate> d_ptr;
};
} // namespace asynqro::tasks

#endif // ASYNQRO_TASKS_DISPATCHER_H
