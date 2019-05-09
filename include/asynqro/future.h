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

#ifdef ASYNQRO_QT_SUPPORT
#    include <QCoreApplication>
#    include <QThread>
#endif

#include <cassert>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <variant>

namespace asynqro {
namespace detail {
enum FutureState
{
    NotCompletedFuture = 0,
    SucceededFuture = 1,
    FailedFuture = 2
};

void ASYNQRO_EXPORT incrementFuturesUsage();
void ASYNQRO_EXPORT decrementFuturesUsage();

template <typename T, typename FailureT>
struct FutureData
{
    FutureData()
    {
#ifdef ASYNQRO_DEBUG_COUNT_OBJECTS
        incrementFuturesUsage();
#endif
    }
    FutureData(const FutureData<T, FailureT> &) = delete;
    FutureData(FutureData<T, FailureT> &&) = delete;
    FutureData<T, FailureT> &operator=(const FutureData<T, FailureT> &) = delete;
    FutureData<T, FailureT> &operator=(FutureData<T, FailureT> &&) = delete;
    ~FutureData()
    {
#ifdef ASYNQRO_DEBUG_COUNT_OBJECTS
        decrementFuturesUsage();
#endif
    }

    std::atomic_int state{NotCompletedFuture};
    std::variant<std::monostate, T, FailureT> value;

    std::list<std::function<void(const T &)>> successCallbacks;
    std::list<std::function<void(const FailureT &)>> failureCallbacks;

    SpinLock mainLock;
};

} // namespace detail

template <typename T, typename FailureT>
class Future
{
    static_assert(!std::is_same_v<T, void>, "Future<void, _> is not allowed. Use Future<bool, _> instead");
    static_assert(!std::is_same_v<FailureT, void>, "Future<_, void> is not allowed. Use Future<_, bool> instead");
    template <typename T2, typename FailureT2>
    friend class Future;
    friend class Promise<T, FailureT>;
    friend struct detail::FutureData<T, FailureT>;
    friend struct WithFailure<FailureT>;
    template <typename... U>
    friend class CancelableFuture;

public:
    using Value = T;
    using Failure = FailureT;
    Future() noexcept = default; // Creates invalid future, calling methods of such object will lead to assertion/segfault
    explicit Future(const Promise<T, FailureT> &promise) { d = promise.future().d; }
    Future(const Future<T, FailureT> &) noexcept = default;
    Future(Future<T, FailureT> &&) noexcept = default;
    Future<T, FailureT> &operator=(const Future<T, FailureT> &) noexcept = default;
    Future<T, FailureT> &operator=(Future<T, FailureT> &&) noexcept = default;
    ~Future() = default;

    bool operator==(const Future<T, FailureT> &other) const noexcept { return d == other.d; }
    bool operator!=(const Future<T, FailureT> &other) const noexcept { return !operator==(other); }

    template <typename... NewFailures, typename Dummy = detail::AsVariant_T<FailureT>,
              typename = std::enable_if_t<detail::CanConvertVariant_V<Dummy, std::variant<NewFailures...>>>>
    operator Future<T, std::variant<NewFailures...>>() // NOLINT(google-explicit-constructor)
    {
        return mapFailure([](const FailureT &failure) {
            return std::visit([](auto &&x) noexcept->std::variant<NewFailures...> { return x; },
                              detail::AsVariant<FailureT>::make(failure));
        });
    }

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

    bool isValid() const noexcept { return static_cast<bool>(d); }

    bool wait(int64_t timeout = -1) const noexcept
    {
        using namespace std::chrono_literals;
        assert(d);
        if (isCompleted())
            return true;
        bool waitForever = timeout < 1;
        bool maintainEvents =
#ifdef ASYNQRO_QT_SUPPORT
            qApp && QThread::currentThread() == qApp->thread();
#else
            false;
#endif
        if (maintainEvents || !waitForever) {
            auto finalPoint = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(timeout);
            while (waitForever || (std::chrono::high_resolution_clock::now() <= finalPoint)) {
                if (isCompleted())
                    return true;
#ifdef ASYNQRO_QT_SUPPORT
                if (maintainEvents)
                    QCoreApplication::processEvents();
                else
#endif
                    std::this_thread::sleep_for(1ms); // NOLINT(readability-misleading-indentation)
            }
        } else {
            if constexpr (std::is_copy_constructible_v<T>) {
                std::mutex mutex;
                std::condition_variable waiter;
                std::unique_lock lock(mutex);
                bool wasInSameThread = false;
                recoverValue(T()).onSuccess(
                    [&waiter, &mutex, &wasInSameThread, waitingThread = std::this_thread::get_id()](const T &) {
                        if (std::this_thread::get_id() == waitingThread) {
                            wasInSameThread = true;
                            return;
                        }
                        mutex.lock(); // We wait here for waiter start
                        mutex.unlock();
                        waiter.notify_all();
                    });
                if (!wasInSameThread)
                    waiter.wait(lock);
            } else {
                while (!isCompleted())
                    std::this_thread::sleep_for(1ms);
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
        if (isSucceeded()) {
            try {
                return std::get<1>(d->value);
            } catch (...) {
            }
        }
        return T();
    }

    const T &resultRef() const
    {
        assert(d);
        if (!isCompleted())
            wait();
        return std::get<1>(d->value);
    }

    FailureT failureReason() const noexcept
    {
        assert(d);
        if (!isCompleted())
            wait();
        if (isFailed()) {
            try {
                return std::get<2>(d->value);
            } catch (...) {
            }
        }
        return FailureT();
    }

    template <typename Func, typename = std::enable_if_t<std::is_invocable_v<Func, T>>>
    Future<T, FailureT> onSuccess(Func &&f) const noexcept
    {
        assert(d);
        bool callIt = false;
        detail::SpinLockHolder lock(&d->mainLock);
        if (isCompleted()) {
            callIt = isSucceeded();
        } else {
            try {
                d->successCallbacks.emplace_back(std::forward<Func>(f));
            } catch (const std::exception &e) {
                return Future<T, FailureT>::failed(detail::exceptionFailure<FailureT>(e));
            } catch (...) {
                return Future<T, FailureT>::failed(detail::exceptionFailure<FailureT>());
            }
        }
        lock.unlock();

        if (callIt) {
            try {
                f(std::get<1>(d->value));
            } catch (...) {
            }
        }
        return Future<T, FailureT>(d);
    }

    template <typename Func, typename = std::enable_if_t<std::is_invocable_v<Func, FailureT>>>
    Future<T, FailureT> onFailure(Func &&f) const noexcept
    {
        assert(d);
        bool callIt = false;
        detail::SpinLockHolder lock(&d->mainLock);
        if (isCompleted()) {
            callIt = isFailed();
        } else {
            try {
                d->failureCallbacks.emplace_back(std::forward<Func>(f));
            } catch (const std::exception &e) {
                return Future<T, FailureT>::failed(detail::exceptionFailure<FailureT>(e));
            } catch (...) {
                return Future<T, FailureT>::failed(detail::exceptionFailure<FailureT>());
            }
        }
        lock.unlock();

        if (callIt) {
            try {
                f(std::get<2>(d->value));
            } catch (...) {
            }
        }
        return Future<T, FailureT>(d);
    }

    template <typename Func, typename = std::enable_if_t<std::is_invocable_r_v<bool, Func, T>>>
    Future<T, FailureT>
    filter(Func &&f, const FailureT &rejected = failure::failureFromString<FailureT>("Result wasn't good enough")) const
        noexcept
    {
        Future<T, FailureT> result = Future<T, FailureT>::create();
        onSuccess([result, f = std::forward<Func>(f), rejected](const T &v) noexcept {
            try {
                if (f(v))
                    result.fillSuccess(v);
                else
                    result.fillFailure(rejected);
            } catch (const std::exception &e) {
                result.fillFailure(detail::exceptionFailure<FailureT>(e));
            } catch (...) {
                result.fillFailure(detail::exceptionFailure<FailureT>());
            }
        });
        onFailure([result](const FailureT &failure) noexcept { result.fillFailure(failure); });
        return result;
    }

    template <typename Func, typename U = std::invoke_result_t<Func, T>>
    Future<U, FailureT> map(Func &&f) const noexcept
    {
        Future<U, FailureT> result = Future<U, FailureT>::create();
        onSuccess([result, f = std::forward<Func>(f)](const T &v) noexcept {
            try {
                result.fillSuccess(f(v));
            } catch (const std::exception &e) {
                result.fillFailure(detail::exceptionFailure<FailureT>(e));
            } catch (...) {
                result.fillFailure(detail::exceptionFailure<FailureT>());
            }
        });
        onFailure([result](const FailureT &failure) noexcept { result.fillFailure(failure); });
        return result;
    }

    template <typename Func, typename OtherFailure = std::invoke_result_t<Func, FailureT>>
    Future<T, OtherFailure> mapFailure(Func &&f) const noexcept
    {
        Future<T, OtherFailure> result = Future<T, OtherFailure>::create();
        onSuccess([result](const T &v) noexcept { result.fillSuccess(v); });
        onFailure([result, f = std::forward<Func>(f)](const FailureT &failure) noexcept {
            try {
                result.fillFailure(f(failure));
            } catch (const std::exception &e) {
                result.fillFailure(detail::exceptionFailure<OtherFailure>(e));
            } catch (...) {
                result.fillFailure(detail::exceptionFailure<OtherFailure>());
            }
        });
        return result;
    }

    template <typename Func, typename U = decltype(std::declval<std::invoke_result_t<Func, T>>().result())>
    Future<U, FailureT> flatMap(Func &&f) const noexcept
    {
        Future<U, FailureT> result = Future<U, FailureT>::create();
        onSuccess([result, f = std::forward<Func>(f)](const T &v) noexcept {
            try {
                f(v).onSuccess([result](const U &v) noexcept { result.fillSuccess(v); })
                    .onFailure([result](const FailureT &failure) noexcept { result.fillFailure(failure); });
            } catch (const std::exception &e) {
                result.fillFailure(detail::exceptionFailure<FailureT>(e));
            } catch (...) {
                result.fillFailure(detail::exceptionFailure<FailureT>());
            }
        });
        onFailure([result](const FailureT &failure) noexcept { result.fillFailure(failure); });
        return result;
    }

    template <typename Func>
    auto andThen(Func &&f) const noexcept
    {
        return flatMap([f = std::forward<Func>(f)](const T &) { return f(); });
    }

    template <typename T2, typename U = typename std::decay_t<T2>>
    Future<U, FailureT> andThenValue(T2 &&value) const noexcept
    {
        return map([value = std::forward<T2>(value)](const auto &) noexcept { return value; });
    }

    template <typename Func, typename Result>
    Future<Result, FailureT> innerReduce(Func &&f, Result acc) const noexcept
    {
        return map([f = std::forward<Func>(f), acc = std::move(acc)](const T &v) {
            auto result = traverse::reduce(v, f, std::move(acc));
            return result;
        });
    }

    template <typename Func, typename Result>
    Future<Result, FailureT> innerMap(Func &&f, Result dest) const noexcept
    {
        return map([f = std::forward<Func>(f), dest = std::move(dest)](const T &v) {
            return traverse::map(v, f, std::move(dest));
        });
    }

    template <typename Func>
    auto innerMap(Func &&f) const noexcept -> decltype(Future<decltype(traverse::map(T(), f)), FailureT>())
    {
        return map([f = std::forward<Func>(f)](const T &v) { return traverse::map(v, f); });
    }

    template <typename Func>
    Future<T, FailureT> innerFilter(Func &&f) const noexcept
    {
        return map([f = std::forward<Func>(f)](const T &v) { return traverse::filter(v, f); });
    }

    template <typename Result>
    Future<Result, FailureT> innerFlatten(Result acc) const noexcept
    {
        return map([acc = std::move(acc)](const T &v) { return traverse::flatten(v, std::move(acc)); });
    }

    template <typename Dummy = void, typename = std::enable_if_t<detail::NestingLevel<T>::value >= 2, Dummy>>
    auto innerFlatten() const noexcept
    {
        return map([](const T &v) { return traverse::flatten(v); });
    }

    template <typename Func, typename = std::enable_if_t<std::is_invocable_v<Func, FailureT>>>
    Future<T, FailureT> recover(Func &&f) const noexcept
    {
        Future<T, FailureT> result = Future<T, FailureT>::create();
        onSuccess([result](const T &v) noexcept { result.fillSuccess(v); });
        onFailure([result, f = std::forward<Func>(f)](const FailureT &failure) noexcept {
            try {
                result.fillSuccess(f(failure));
            } catch (const std::exception &e) {
                result.fillFailure(detail::exceptionFailure<FailureT>(e));
            } catch (...) {
                result.fillFailure(detail::exceptionFailure<FailureT>());
            }
        });
        return result;
    }

    template <typename Func,
              typename OtherFailure = decltype(std::declval<std::invoke_result_t<Func, FailureT>>().failureReason()),
              typename = decltype(std::declval<std::invoke_result_t<Func, FailureT>>().result())>
    Future<T, OtherFailure> recoverWith(Func &&f) const noexcept
    {
        Future<T, OtherFailure> result = Future<T, OtherFailure>::create();
        onSuccess([result](const T &v) noexcept { result.fillSuccess(v); });
        onFailure([result, f = std::forward<Func>(f)](const FailureT &failure) noexcept {
            try {
                f(failure)
                    .onSuccess([result](const T &v) noexcept { result.fillSuccess(v); })
                    .onFailure([result](const OtherFailure &failure) noexcept { result.fillFailure(failure); });
            } catch (const std::exception &e) {
                result.fillFailure(detail::exceptionFailure<OtherFailure>(e));
            } catch (...) {
                result.fillFailure(detail::exceptionFailure<OtherFailure>());
            }
        });
        return result;
    }

    template <typename Dummy = void, typename = std::enable_if_t<std::is_copy_constructible_v<T>, Dummy>>
    Future<T, FailureT> recoverValue(T &&value) const noexcept
    {
        return recover([value = std::forward<T>(value)](const FailureT &) noexcept { return value; });
    }

    template <typename Head, typename... Tail,
              typename Result = detail::TypesProduct_T<T, typename Head::Value, typename Tail::Value...>,
              typename InnerZipFailure = detail::TypesSum_T<typename Head::Failure, typename Tail::Failure...>,
              typename NewFailure = detail::TypesSum_T<FailureT, InnerZipFailure>>
    Future<Result, NewFailure> zip(Head head, Tail... tail) const noexcept
    {
        if constexpr (std::is_same_v<FailureT, NewFailure>) {
            return flatMap([head, tail...](const T &v) noexcept->Future<Result, FailureT> {
                return head.zip(tail...).map([v](const auto &argsResult) noexcept->Result {
                    return std::tuple_cat(detail::AsTuple<T>::make(v), argsResult);
                });
            });
        } else { // NOLINT(readability-else-after-return,readability-misleading-indentation)
            return mapFailure([](const FailureT &failure) {
                       return std::visit([](auto &&x) noexcept->NewFailure { return x; },
                                         detail::AsVariant<FailureT>::make(failure));
                   })
                .flatMap([head, tail...](const T &v) noexcept->Future<Result, NewFailure> {
                    return head.zip(tail...)
                        .mapFailure([](const InnerZipFailure &failure) {
                            return std::visit([](auto &&x) noexcept->NewFailure { return x; },
                                              detail::AsVariant<InnerZipFailure>::make(failure));
                        })
                        .map([v](const auto &argsResult) noexcept->Result {
                            return std::tuple_cat(detail::AsTuple<T>::make(v), argsResult);
                        });
                });
        }
    }

    template <typename T2, typename Result = detail::TypesProduct_T<T, std::decay_t<T2>>>
    Future<Result, FailureT> zipValue(T2 &&value) const noexcept
    {
        return zip(Future<std::decay_t<T2>, FailureT>::successful(std::forward<T2>(value)));
    }

    static auto successful(T &&value) noexcept
    {
        Future<std::decay_t<T>, FailureT> result = Future<std::decay_t<T>, FailureT>::create();
        result.fillSuccess(std::forward<T>(value));
        return result;
    }

    static Future<T, FailureT> successful(const T &value) noexcept
    {
        Future<T, FailureT> result = Future<T, FailureT>::create();
        result.fillSuccess(value);
        return result;
    }

    static Future<T, FailureT> successful() noexcept { return Future<T, FailureT>::successful(T()); }

    static Future<T, FailureT> failed(const FailureT &failure) noexcept
    {
        Future<T, FailureT> result = Future<T, FailureT>::create();
        result.fillFailure(failure);
        return result;
    }

    // This overload copies container to make sure that it will be reachable in future
    template <template <typename...> typename F, template <typename...> typename Container, typename... Fs,
              typename = std::enable_if_t<
                  std::is_same_v<F<T, FailureT>,
                                 Future<T, FailureT>> || std::is_same_v<F<T, FailureT>, CancelableFuture<T, FailureT>>>>
    static Future<Container<T>, FailureT> sequence(const Container<F<T, FailureT>, Fs...> &container) noexcept
    {
        Container<F<T, FailureT>> copy(container);
        return sequence(std::move(copy));
    }

    template <template <typename...> typename F, template <typename...> typename Container, typename... Fs,
              typename Dummy = void, typename = std::enable_if_t<std::is_copy_constructible_v<T>, Dummy>,
              typename = std::enable_if_t<
                  std::is_same_v<F<T, FailureT>,
                                 Future<T, FailureT>> || std::is_same_v<F<T, FailureT>, CancelableFuture<T, FailureT>>>>
    static Future<Container<T>, FailureT> sequence(Container<F<T, FailureT>, Fs...> &&container) noexcept
    {
        if (container.empty())
            return Future<Container<T>, FailureT>::successful();
        Future<Container<T>, FailureT> future = Future<Container<T>, FailureT>::create();
        Container<T> result;
        traverse::detail::containers::reserve(result, container.size());
        iterateSequence(std::move(container), container.cbegin(), std::move(result), future);
        return future;
    }

#ifdef ASYNQRO_QT_SUPPORT
    template <typename Signal, typename Sender>
    static Future<T, FailureT> fromQtSignal(Sender *sender, const Signal &signal)
    {
        Future<T, FailureT> result = Future<T, FailureT>::create();
        auto conn = std::make_shared<QMetaObject::Connection>();
        auto deathConn = std::make_shared<QMetaObject::Connection>();
        if constexpr (std::is_invocable_v<Signal, decltype(sender), T>) {
            *conn = QObject::connect(sender, signal, sender, [result, conn, deathConn](T x) {
                result.fillSuccess(std::forward<T>(x));
                QObject::disconnect(*conn);
                QObject::disconnect(*deathConn);
            });
        } else if constexpr (std::is_same_v<T, bool>) { // NOLINT(readability-misleading-indentation)
            *conn = QObject::connect(sender, signal, sender, [result, conn, deathConn]() {
                result.fillSuccess(true);
                QObject::disconnect(*conn);
                QObject::disconnect(*deathConn);
            });
        } else if constexpr (detail::IsSpecialization_V<T, std::tuple>) { // NOLINT(readability-misleading-indentation)
            *conn = QObject::connect(sender, signal, sender, [result, conn, deathConn](auto... x) {
                result.fillSuccess(std::make_tuple(x...));
                QObject::disconnect(*conn);
                QObject::disconnect(*deathConn);
            });
        } else { // NOLINT(readability-misleading-indentation)
            static_assert(std::is_invocable_v<Signal, decltype(sender), T>,
                          "Signal is not compatible with Future type");
        }
        *deathConn = QObject::connect(sender, &QObject::destroyed, sender, [result, conn, deathConn]() {
            result.fillFailure(failure::failureFromString<FailureT>("Destroyed"));
            QObject::disconnect(*conn);
            QObject::disconnect(*deathConn);
        });
        return result;
    }
#endif

private:
    explicit Future(const std::shared_ptr<detail::FutureData<T, FailureT>> &otherD) { d = otherD; }
    inline static Future<T, FailureT> create()
    {
        Future<T, FailureT> result;
        result.d = std::make_shared<detail::FutureData<T, FailureT>>();
        return result;
    }

    void fillSuccess(const T &result) const noexcept
    {
        T copy = result;
        fillSuccess(std::move(copy));
    }

    void fillSuccess(T &&result) const noexcept
    {
        assert(d);
        if (detail::hasLastFailure()) {
            FailureT failure = detail::lastFailure<FailureT>();
            detail::invalidateLastFailure();
            fillFailure(std::move(failure));
            return;
        }

        detail::SpinLockHolder lock(&d->mainLock);
        if (isCompleted())
            return;
        try {
            d->value.template emplace<1>(std::forward<T>(result));
        } catch (...) {
            // Should never happen
        }
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

    void fillFailure(const FailureT &reason) const noexcept
    {
        FailureT copy = reason;
        fillFailure(std::move(copy));
    }

    void fillFailure(FailureT &&reason) const noexcept
    {
        assert(d);

        detail::SpinLockHolder lock(&d->mainLock);
        if (isCompleted())
            return;
        try {
            d->value.template emplace<2>(std::forward<FailureT>(reason));
        } catch (...) {
            // Should never happen
        }
        d->state.store(detail::FutureState::FailedFuture, std::memory_order_release);
        const auto callbacks = std::move(d->failureCallbacks);
        d->failureCallbacks = std::list<std::function<void(const FailureT &)>>();
        d->successCallbacks.clear();
        lock.unlock();

        for (const auto &f : callbacks) {
            try {
                f(std::get<2>(d->value));
            } catch (...) {
            }
        }
    }

    auto zip() const noexcept
    {
        return map([](const T &v) noexcept { return detail::AsTuple<T>::make(v); });
    }

    template <typename It, template <typename...> typename F, typename... Ts, typename... Fs,
              template <typename...> typename Container>
    static void iterateSequence(Container<F<T, FailureT>, Fs...> &&initial, It current, Container<T, Ts...> &&result,
                                const Future<Container<T, Ts...>, FailureT> &future) noexcept
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
                Future<T, FailureT>::iterateSequence(std::move(initial), current, std::move(result), future);
            })
            .onFailure([future](const FailureT &reason) noexcept { future.fillFailure(reason); });
    }

    std::shared_ptr<detail::FutureData<T, FailureT>> d;
};

int_fast64_t ASYNQRO_EXPORT instantFuturesUsage();

} // namespace asynqro

#endif // ASYNQRO_FUTURE_H
