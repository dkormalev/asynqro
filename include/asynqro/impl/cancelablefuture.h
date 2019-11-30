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

// Normally this file shouldn't be included directly. asynqro/future.h already has it included
// Moved to separate header only to keep files smaller
#ifndef ASYNQRO_CANCELABLEFUTURE_H
#define ASYNQRO_CANCELABLEFUTURE_H

#include "asynqro/impl/failure_handling.h"

#include <chrono>

namespace asynqro {
template <typename T, typename Failure>
class Future;
template <typename T, typename Failure>
class Promise;

template <typename... T>
class CancelableFuture;

template <>
class CancelableFuture<>
{
public:
    CancelableFuture() = delete;
    CancelableFuture(const CancelableFuture &) = delete;
    CancelableFuture(CancelableFuture &&) = delete;
    CancelableFuture &operator=(const CancelableFuture &) = delete;
    CancelableFuture &operator=(CancelableFuture &&) = delete;
    ~CancelableFuture() = delete;

    template <typename T, typename Failure>
    static CancelableFuture<T, Failure> create(const Promise<T, Failure> &promise)
    {
        return CancelableFuture<T, Failure>(promise);
    }
};

//Should have the same object-level public API as Future<T, Failure>
template <typename T, typename FailureType>
class CancelableFuture<T, FailureType>
{
    static_assert(!std::is_same_v<T, void>,
                  "CancelableFuture<void, _> is not allowed. Use CancelableFuture<bool, _> instead");
    static_assert(!std::is_same_v<FailureType, void>,
                  "CancelableFuture<_, void> is not allowed. Use CancelableFuture<_, bool> instead");
    template <typename T2, typename FailureType2>
    friend class Future;

public:
    using Value = T;
    using Failure = FailureType;
    CancelableFuture() = default;
    explicit CancelableFuture(const Promise<T, FailureType> &promise) { m_promise = promise; }
    CancelableFuture(CancelableFuture<T, FailureType> &&) noexcept = default;
    CancelableFuture(const CancelableFuture<T, FailureType> &) noexcept = default;
    CancelableFuture<T, FailureType> &operator=(CancelableFuture<T, FailureType> &&) noexcept = default;
    CancelableFuture<T, FailureType> &operator=(const CancelableFuture<T, FailureType> &) noexcept = default;
    ~CancelableFuture() = default;
    void cancel(const FailureType &failure = failure::failureFromString<FailureType>("Canceled")) const noexcept
    {
        if (!m_promise.isFilled())
            m_promise.failure(failure);
    }
    operator Future<T, FailureType>() const noexcept // NOLINT(google-explicit-constructor)
    {
        return m_promise.future();
    }
    Future<T, FailureType> future() const noexcept { return m_promise.future(); }

    bool isCompleted() const noexcept { return future().isCompleted(); }
    bool isFailed() const noexcept { return future().isFailed(); }
    bool isSucceeded() const noexcept { return future().isSucceeded(); }
    bool isValid() const noexcept { return true; }
    bool wait(long long timeout) const noexcept { return future().wait(std::chrono::milliseconds(timeout)); }
    bool wait(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) const noexcept
    {
        return future().wait(timeout);
    }
    T result() const noexcept { return future().result(); }
    FailureType failureReason() const noexcept { return future().failureReason(); }

    template <typename Func>
    auto onSuccess(Func &&f) const noexcept
    {
        return future().onSuccess(std::forward<Func>(f));
    }

    template <typename Func>
    auto onFailure(Func &&f) const noexcept
    {
        return future().onFailure(std::forward<Func>(f));
    }

    template <typename Func>
    auto onComplete(Func &&f) const noexcept
    {
        return future().onComplete(std::forward<Func>(f));
    }

    template <typename Func>
    auto
    filter(Func &&f,
           const FailureType &rejected = failure::failureFromString<FailureType>("Result wasn't good enough")) noexcept
    {
        return future().filter(std::forward<Func>(f), rejected);
    }

    template <typename Func>
    auto map(Func &&f) const noexcept
    {
        return future().map(std::forward<Func>(f));
    }

    template <typename Func>
    auto mapFailure(Func &&f) const noexcept
    {
        return future().mapFailure(std::forward<Func>(f));
    }

    template <typename Func>
    auto flatMap(Func &&f) const noexcept
    {
        return future().flatMap(std::forward<Func>(f));
    }

    template <typename Func>
    auto andThen(Func &&f) const noexcept
    {
        return future().andThen(std::forward<Func>(f));
    }

    template <typename T2>
    auto andThenValue(T2 &&value) noexcept
    {
        return future().andThenValue(std::forward<T2>(value));
    }

    template <typename Func, typename Result>
    auto innerReduce(Func &&f, Result &&acc) const noexcept
    {
        return future().innerReduce(std::forward<Func>(f), std::forward<Result>(acc));
    }

    template <typename Func, typename Result>
    auto innerMap(Func &&f, Result &&dest) const noexcept
    {
        return future().innerMap(std::forward<Func>(f), std::forward<Result>(dest));
    }

    template <typename Func>
    auto innerMap(Func &&f) const noexcept
    {
        return future().innerMap(std::forward<Func>(f));
    }

    template <typename Func>
    auto innerFilter(Func &&f) const noexcept
    {
        return future().innerFilter(std::forward<Func>(f));
    }

    template <typename Result>
    auto innerFlatten(Result &&acc) const noexcept
    {
        return future().innerFlatten(std::forward<Result>(acc));
    }

    auto innerFlatten() const noexcept { return future().innerFlatten(); }

    template <typename Func>
    auto recover(Func &&f) const noexcept
    {
        return future().recover(std::forward<Func>(f));
    }

    template <typename Func>
    auto recoverWith(Func &&f) const noexcept
    {
        return future().recoverWith(std::forward<Func>(f));
    }

    auto recoverValue(T &&value) const noexcept { return future().recoverValue(std::forward<T>(value)); }

    template <typename Head, typename... Tail>
    auto zip(Head head, Tail... tail) const noexcept
    {
        return future().zip(head, tail...);
    }
    template <typename T2>
    auto zipValue(T2 &&value) const noexcept
    {
        return future().zipValue(std::forward<T2>(value));
    }

private:
    auto zip() const noexcept { return future().zip(); }

    Promise<T, FailureType> m_promise;
};

template <typename T, typename Failure>
bool operator==(const CancelableFuture<T, Failure> &l, const Future<T, Failure> &r) noexcept
{
    return l.future() == r;
}

template <typename T, typename Failure>
bool operator!=(const CancelableFuture<T, Failure> &l, const Future<T, Failure> &r) noexcept
{
    return !(l == r);
}

template <typename T, typename Failure>
bool operator==(const Future<T, Failure> &l, const CancelableFuture<T, Failure> &r) noexcept
{
    return l == r.future();
}

template <typename T, typename Failure>
bool operator!=(const Future<T, Failure> &l, const CancelableFuture<T, Failure> &r) noexcept
{
    return !(l == r);
}

} // namespace asynqro
#endif // ASYNQRO_CANCELABLEFUTURE_H
