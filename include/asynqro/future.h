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
#ifndef ASYNQRO_FUTURE_H
#define ASYNQRO_FUTURE_H

#include "asynqro/impl/asynqro_export.h"
#include "asynqro/impl/cancelablefuture.h"
#include "asynqro/impl/containers_traverse.h"
#include "asynqro/impl/failure_handling.h"
#include "asynqro/impl/promise.h"
#include "asynqro/impl/spinlock.h"
#include "asynqro/impl/zipfutures.h"

#include <QCoreApplication>
#include <QMutex>
#include <QThread>
#include <QTime>
#include <QVariant>
#include <QWaitCondition>

#include <iostream>
#include <list>
#include <memory>
#include <tuple>
#include <type_traits>
#include <variant>

namespace asynqro {
template <typename... T>
class Future;

template <>
class Future<>
{
public:
    Future() = delete;
    Future(const Future &) = delete;
    Future(Future &&) = delete;
    Future &operator=(const Future &) = delete;
    Future &operator=(Future &&) = delete;
    ~Future() = delete;

    template <typename T>
    static auto successful(T &&value) noexcept
    {
        return Future<std::decay_t<T>>::successful(std::forward<T>(value));
    }

    template <typename T>
    static auto successful(const T &value) noexcept
    {
        return Future<T>::successful(value);
    }

    // This overload copies container to make sure that it will be reachable in future
    template <typename T, template <typename...> typename F, template <typename...> typename Container>
    static Future<Container<T>> sequence(const Container<F<T>> &container) noexcept
    {
        Container<F<T>> copy = container;
        return Future<T>::sequence(std::move(copy));
    }

    template <typename T, template <typename...> typename F, template <typename...> typename Container>
    static Future<Container<T>> sequence(Container<F<T>> &&container) noexcept
    {
        return Future<T>::sequence(std::move(container));
    }
};

namespace detail {
enum FutureState
{
    NotCompletedFuture = 0,
    SucceededFuture = 1,
    FailedFuture = 2
};

void ASYNQRO_EXPORT incrementFuturesUsage();
void ASYNQRO_EXPORT decrementFuturesUsage();

template <typename T>
struct FutureData
{
    FutureData()
    {
#ifdef ASYNQRO_DEBUG_COUNT_OBJECTS
        incrementFuturesUsage();
#endif
    }
    FutureData(const FutureData<T> &) = delete;
    FutureData(FutureData<T> &&) = delete;
    FutureData &operator=(const FutureData<T> &) = delete;
    FutureData &operator=(FutureData<T> &&) = delete;
    ~FutureData()
    {
#ifdef ASYNQRO_DEBUG_COUNT_OBJECTS
        decrementFuturesUsage();
#endif
    }

    std::atomic_int state{NotCompletedFuture};
    std::variant<std::monostate, T, QVariant> value;

    std::list<std::function<void(const T &)>> successCallbacks;
    std::list<std::function<void(const QVariant &)>> failureCallbacks;

    SpinLock mainLock;
};

} // namespace detail

qint64 ASYNQRO_EXPORT instantFuturesUsage();

template <typename T>
class Future<T>
{
    static_assert(!std::is_same_v<T, void>, "Future<void> is not allowed. Use Future<bool> instead");
    template <typename... U>
    friend class Future;
    friend class Promise<T>;
    friend struct detail::FutureData<T>;
    friend struct WithFailure;
    template <typename... U>
    friend class CancelableFuture;

public:
    using Value = T;
    Future() noexcept = default; // Creates invalid future, calling methods of such object will lead to assertion/segfault
    Future(const Promise<T> &promise) { d = promise.future().d; }
    Future(const Future<T> &) noexcept = default;
    Future(Future<T> &&) noexcept = default;
    Future &operator=(const Future<T> &) noexcept = default;
    Future &operator=(Future<T> &&) noexcept = default;
    ~Future() = default;

    bool operator==(const Future<T> &other) const noexcept { return d == other.d; }
    bool operator!=(const Future<T> &other) const noexcept { return !operator==(other); }

    bool isCompleted() const noexcept
    {
        assert(d);
        int value = d->state.load(std::memory_order_acquire);
        return value == detail::FutureState::FailedFuture || value == detail::FutureState::SucceededFuture;
    }
    bool isFailed() const noexcept
    {
        assert(d);
        return d->state.load(std::memory_order_acquire) == detail::FutureState::FailedFuture;
    }
    bool isSucceeded() const noexcept
    {
        assert(d);
        return d->state.load(std::memory_order_acquire) == detail::FutureState::SucceededFuture;
    }

    bool isValid() const noexcept { return d; }

    bool wait(long long timeout = -1) const noexcept
    {
        assert(d);
        if (isCompleted())
            return true;
        bool waitForever = timeout < 1;
        bool maintainEvents = qApp && QThread::currentThread() == qApp->thread();
        if (maintainEvents || !waitForever) {
            QTime timer;
            timer.start();
            while (waitForever || (timer.elapsed() <= timeout)) {
                if (isCompleted())
                    return true;
                if (maintainEvents)
                    QCoreApplication::processEvents();
                else
                    QThread::msleep(1);
            }
        } else {
            if constexpr (std::is_copy_constructible_v<T>) {
                QMutex mutex;
                QWaitCondition waiter;
                mutex.lock();
                bool wasInSameThread = false;
                recoverValue(T()).onSuccess(
                    [&waiter, &mutex, &wasInSameThread, waitingThread = QThread::currentThread()](const T &) {
                        if (QThread::currentThread() == waitingThread) {
                            wasInSameThread = true;
                            return;
                        }
                        mutex.lock(); // We wait here for waiter start
                        mutex.unlock();
                        waiter.wakeAll();
                    });
                if (!wasInSameThread)
                    waiter.wait(&mutex);
                mutex.unlock();
            } else {
                while (!isCompleted())
                    QThread::msleep(1);
            }
        }
        return isCompleted();
    }

    template <typename Dummy = void, typename = std::enable_if_t<std::is_copy_constructible_v<T>, Dummy>>
    T result() const noexcept
    {
        assert(d);
        if (!isCompleted())
            wait();
        return isSucceeded() ? std::get<1>(d->value) : T();
    }

    const T &resultRef() const noexcept
    {
        assert(d);
        if (!isCompleted())
            wait();
        return std::get<1>(d->value);
    }

    QVariant failureReason() const noexcept
    {
        assert(d);
        if (!isCompleted())
            wait();
        return isFailed() ? std::get<2>(d->value) : QVariant();
    }

    template <typename Func, typename = std::enable_if_t<std::is_invocable_v<Func, T>>>
    Future<T> onSuccess(Func &&f) const noexcept
    {
        assert(d);
        bool callIt = false;
        detail::SpinLockHolder lock(&d->mainLock);
        if (isCompleted())
            callIt = isSucceeded();
        else
            d->successCallbacks.emplace_back(std::forward<Func>(f));
        lock.unlock();

        if (callIt) {
            try {
                f(std::get<1>(d->value));
            } catch (...) {
            }
        }
        return Future<T>(d);
    }

    template <typename Func, typename = std::enable_if_t<std::is_invocable_v<Func, QVariant>>>
    Future<T> onFailure(Func &&f) const noexcept
    {
        assert(d);
        bool callIt = false;
        detail::SpinLockHolder lock(&d->mainLock);
        if (isCompleted())
            callIt = isFailed();
        else
            d->failureCallbacks.emplace_back(std::forward<Func>(f));
        lock.unlock();

        if (callIt) {
            try {
                f(std::get<2>(d->value));
            } catch (...) {
            }
        }
        return Future<T>(d);
    }

    template <typename Func, typename = std::enable_if_t<std::is_invocable_r_v<bool, Func, T>>>
    Future<T> filter(Func &&f, const QVariant &rejected = QStringLiteral("Result wasn't good enough")) const noexcept
    {
        Future<T> result = Future<T>::create();
        onSuccess([result, f = std::forward<Func>(f), rejected](const T &v) noexcept {
            try {
                if (f(v))
                    result.fillSuccess(v);
                else
                    result.fillFailure(rejected);
            } catch (const std::exception &e) {
                result.fillFailure(detail::exceptionFailure(e));
            } catch (...) {
                result.fillFailure(detail::exceptionFailure());
            }
        });
        onFailure([result](const QVariant &failure) noexcept { result.fillFailure(failure); });
        return result;
    }

    template <typename Func, typename U = std::invoke_result_t<Func, T>>
    Future<U> map(Func &&f) const noexcept
    {
        Future<U> result = Future<U>::create();
        onSuccess([result, f = std::forward<Func>(f)](const T &v) noexcept {
            try {
                result.fillSuccess(f(v));
            } catch (const std::exception &e) {
                result.fillFailure(detail::exceptionFailure(e));
            } catch (...) {
                result.fillFailure(detail::exceptionFailure());
            }
        });
        onFailure([result](const QVariant &failure) noexcept { result.fillFailure(failure); });
        return result;
    }

    template <typename Func, typename U = decltype(std::declval<std::invoke_result_t<Func, T>>().result())>
    Future<U> flatMap(Func &&f) const noexcept
    {
        Future<U> result = Future<U>::create();
        onSuccess([result, f = std::forward<Func>(f)](const T &v) noexcept {
            try {
                f(v).onSuccess([result](const U &v) noexcept { result.fillSuccess(v); })
                    .onFailure([result](const QVariant &failure) noexcept { result.fillFailure(failure); });
            } catch (const std::exception &e) {
                result.fillFailure(detail::exceptionFailure(e));
            } catch (...) {
                result.fillFailure(detail::exceptionFailure());
            }
        });
        onFailure([result](const QVariant &failure) noexcept { result.fillFailure(failure); });
        return result;
    }

    template <typename Func, typename U = decltype(std::declval<std::invoke_result_t<Func>>().result())>
    Future<U> andThen(Func &&f) const noexcept
    {
        return flatMap([f = std::forward<Func>(f)](const T &) { return f(); });
    }

    template <typename T2, typename U = typename std::decay_t<T2>>
    Future<U> andThenValue(T2 &&value) const noexcept
    {
        return map([value = std::forward<T2>(value)](const auto &) noexcept { return value; });
    }

    template <typename Func, typename Result>
    Future<Result> innerReduce(Func &&f, Result acc) const noexcept
    {
        return map([f = std::forward<Func>(f), acc = std::move(acc)](const T &v) {
            auto result = traverse::reduce(v, f, std::move(acc));
            return result;
        });
    }

    template <typename Func, typename Result>
    Future<Result> innerMap(Func &&f, Result dest) const noexcept
    {
        return map([f = std::forward<Func>(f), dest = std::move(dest)](const T &v) {
            return traverse::map(v, f, std::move(dest));
        });
    }

    template <typename Func>
    auto innerMap(Func &&f) const noexcept -> decltype(Future<decltype(traverse::map(T(), f))>())
    {
        return map([f = std::forward<Func>(f)](const T &v) { return traverse::map(v, f); });
    }

    template <typename Func>
    Future<T> innerFilter(Func &&f) const noexcept
    {
        return map([f = std::forward<Func>(f)](const T &v) { return traverse::filter(v, f); });
    }

    template <typename Result>
    Future<Result> innerFlatten(Result acc) const noexcept
    {
        return map([acc = std::move(acc)](const T &v) { return traverse::flatten(v, std::move(acc)); });
    }

    template <typename Dummy = void, typename = std::enable_if_t<detail::NestingLevel<T>::value >= 2, Dummy>>
    auto innerFlatten() const noexcept
    {
        return map([](const T &v) { return traverse::flatten(v); });
    }

    template <typename Func, typename = std::enable_if_t<std::is_invocable_v<Func, QVariant>>>
    Future<T> recover(Func &&f) const noexcept
    {
        Future<T> result = Future<T>::create();
        onSuccess([result](const T &v) noexcept { result.fillSuccess(v); });
        onFailure([result, f = std::forward<Func>(f)](const QVariant &failure) noexcept {
            try {
                result.fillSuccess(f(failure));
            } catch (const std::exception &e) {
                result.fillFailure(detail::exceptionFailure(e));
            } catch (...) {
                result.fillFailure(detail::exceptionFailure());
            }
        });
        return result;
    }

    template <typename Func, typename = decltype(std::declval<std::invoke_result_t<Func, QVariant>>().result())>
    Future<T> recoverWith(Func &&f) const noexcept
    {
        Future<T> result = Future<T>::create();
        onSuccess([result](const T &v) noexcept { result.fillSuccess(v); });
        onFailure([result, f = std::forward<Func>(f)](const QVariant &failure) noexcept {
            try {
                f(failure)
                    .onSuccess([result](const T &v) noexcept { result.fillSuccess(v); })
                    .onFailure([result](const QVariant &failure) noexcept { result.fillFailure(failure); });
            } catch (const std::exception &e) {
                result.fillFailure(detail::exceptionFailure(e));
            } catch (...) {
                result.fillFailure(detail::exceptionFailure());
            }
        });
        return result;
    }

    template <typename Dummy = void, typename = std::enable_if_t<std::is_copy_constructible_v<T>, Dummy>>
    Future<T> recoverValue(T &&value) const noexcept
    {
        return recover([value = std::forward<T>(value)](const QVariant &) noexcept { return value; });
    }

    template <typename Head, typename... Tail, typename Result = detail::ZipFutures_T<Future<T>, Head, Tail...>>
    Future<Result> zip(Head head, Tail... tail) const noexcept
    {
        return flatMap([head, tail...](const T &v) noexcept->Future<Result> {
            return head.zip(tail...).map([v](const auto &argsResult) noexcept->Result {
                return std::tuple_cat(detail::AsTuple<T>::make(v), argsResult);
            });
        });
    }
    template <typename T2, typename Result = detail::ZipFutures_T<Future<T>, Future<std::decay_t<T2>>>>
    Future<Result> zipValue(T2 &&value) const noexcept
    {
        return zip(Future<>::successful(std::forward<T2>(value)));
    }

    static auto successful(T &&value) noexcept
    {
        Future<std::decay_t<T>> result = Future<std::decay_t<T>>::create();
        result.fillSuccess(std::forward<T>(value));
        return result;
    }

    static Future<T> successful(const T &value) noexcept
    {
        Future<T> result = Future<T>::create();
        result.fillSuccess(value);
        return result;
    }

    static Future<T> successful() noexcept { return Future<T>::successful(T()); }

    static Future<T> failed(const QVariant &failure) noexcept
    {
        Future<T> result = Future<T>::create();
        result.fillFailure(failure);
        return result;
    }

    // This overload copies container to make sure that it will be reachable in future
    template <template <typename...> typename F, template <typename...> typename Container,
              typename = std::enable_if_t<std::is_same_v<F<T>, Future<T>> || std::is_same_v<F<T>, CancelableFuture<T>>>>
    static Future<Container<T>> sequence(const Container<F<T>> &container) noexcept
    {
        Container<F<T>> copy(container);
        return sequence(std::move(copy));
    }

    template <template <typename...> typename F, template <typename...> typename Container, typename Dummy = void,
              typename = std::enable_if_t<std::is_copy_constructible_v<T>, Dummy>,
              typename = std::enable_if_t<std::is_same_v<F<T>, Future<T>> || std::is_same_v<F<T>, CancelableFuture<T>>>>
    static Future<Container<T>> sequence(Container<F<T>> &&container) noexcept
    {
        if (container.empty())
            return Future<Container<T>>::successful();
        Future<Container<T>> future = Future<Container<T>>::create();
        Container<T> result;
        traverse::detail::containers::reserve(result, container.size());
        iterateSequence(std::move(container), container.cbegin(), std::move(result), future);
        return future;
    }

private:
    Future(const std::shared_ptr<detail::FutureData<T>> &otherD) { d = otherD; }
    inline static Future<T> create()
    {
        Future<T> result;
        result.d = std::make_shared<detail::FutureData<T>>();
        return result;
    }

    void fillSuccess(const T &result) const noexcept
    {
        T copy(result);
        fillSuccess(std::move(copy));
    }

    void fillSuccess(T &&result) const noexcept
    {
        assert(d);
        if (detail::hasLastFailure()) {
            QVariant failure = detail::lastFailure();
            detail::invalidateLastFailure();
            fillFailure(failure);
            return;
        }

        detail::SpinLockHolder lock(&d->mainLock);
        if (isCompleted())
            return;
        d->value.template emplace<1>(std::forward<T>(result));
        d->state.store(detail::FutureState::SucceededFuture, std::memory_order_release);
        const auto callbacks = std::move(d->successCallbacks);
        d->successCallbacks = std::list<std::function<void(const T &)>>();
        d->failureCallbacks.clear();
        lock.unlock();

        for (const auto &f : callbacks) {
            try {
                f(std::get<1>(d->value));
            } catch (...) {
            }
        }
    }

    void fillFailure(const QVariant &reason) const noexcept
    {
        assert(d);

        detail::SpinLockHolder lock(&d->mainLock);
        if (isCompleted())
            return;
        d->value.template emplace<2>(reason);
        d->state.store(detail::FutureState::FailedFuture, std::memory_order_release);
        const auto callbacks = std::move(d->failureCallbacks);
        d->failureCallbacks = std::list<std::function<void(const QVariant &)>>();
        d->successCallbacks.clear();
        lock.unlock();

        for (const auto &f : callbacks) {
            try {
                f(reason);
            } catch (...) {
            }
        }
    }

    auto zip() const noexcept
    {
        return map([](const T &v) noexcept { return detail::AsTuple<T>::make(v); });
    }

    template <typename It, template <typename...> typename F, template <typename...> typename Container>
    static void iterateSequence(Container<F<T>> &&initial, It current, Container<T> &&result,
                                const Future<Container<T>> &future) noexcept
    {
        while (current != initial.cend()) {
            auto f = (*current);
            if (!f.isCompleted())
                break;
            if (f.isFailed()) {
                future.fillFailure(f.failureReason());
                return;
            }
            traverse::detail::containers::add(result, f.result());
            ++current;
        }
        if (current == initial.cend()) {
            future.fillSuccess(std::move(result));
            return;
        }
        auto currentFuture = *current;
        currentFuture
            .onSuccess([initial = std::move(initial), current, result = std::move(result),
                        future](const T &) mutable noexcept {
                Future<T>::iterateSequence(std::move(initial), current, std::move(result), future);
            })
            .onFailure([future](const QVariant &reason) noexcept { future.fillFailure(reason); });
    }

    std::shared_ptr<detail::FutureData<T>> d;
};

} // namespace asynqro

#endif // ASYNQRO_FUTURE_H
