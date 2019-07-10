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
#ifndef ASYNQRO_REPEAT_H
#define ASYNQRO_REPEAT_H

#include "asynqro/future.h"
#include "asynqro/tasks.h"

#include <optional>

namespace asynqro {
namespace detail {
enum class RepeaterBehavior
{
    Simple,
    Trampolined
};
} // namespace detail

template <typename T, typename... Args>
using RepeaterResult = std::variant<T, std::pair<detail::RepeaterBehavior, std::tuple<Args...>>>;

template <typename T, typename FailureT, typename... Args>
using RepeaterFutureResult = Future<RepeaterResult<T, Args...>, FailureT>;

namespace repeater {
template <typename T>
struct Finish
{
    explicit Finish(T value) { m_result.emplace(std::move(value)); }

    template <typename... Args>
    operator RepeaterResult<T, Args...>() noexcept // NOLINT(google-explicit-constructor)
    {
        try {
            return RepeaterResult<T, Args...>(std::in_place_index_t<0>(), std::move(m_result.value()));
        } catch (...) {
            // Should never happen
        }
        return RepeaterResult<T, Args...>();
    }

private:
    std::optional<T> m_result;
};

template <typename... Args>
struct Continue
{
    explicit Continue(Args... values) { m_newArgs.emplace(std::move(values)...); }

    template <typename T>
    operator RepeaterResult<T, Args...>() noexcept // NOLINT(google-explicit-constructor)
    {
        try {
            return RepeaterResult<T, Args...>(std::in_place_index_t<1>(), asynqro::detail::RepeaterBehavior::Simple,
                                              std::move(m_newArgs.value()));
        } catch (...) {
            // Should never happen
        }
        return RepeaterResult<T, Args...>();
    }

private:
    std::optional<std::tuple<std::decay_t<Args>...>> m_newArgs;
};

template <typename... Args>
struct TrampolinedContinue
{
    explicit TrampolinedContinue(Args... values) { m_newArgs.emplace(std::move(values)...); }

    template <typename T>
    operator RepeaterResult<T, Args...>() noexcept // NOLINT(google-explicit-constructor)
    {
        try {
            return RepeaterResult<T, Args...>(std::in_place_index_t<1>(), asynqro::detail::RepeaterBehavior::Trampolined,
                                              std::move(m_newArgs.value()));
        } catch (...) {
            // Should never happen
        }
        return RepeaterResult<T, Args...>();
    }

private:
    std::optional<std::tuple<std::decay_t<Args>...>> m_newArgs;
};
} // namespace repeater

template <typename T, typename FailureT, typename Func, typename... Args>
Future<T, FailureT> repeat(Func &&f, Args &&... args) noexcept
{
    constexpr bool returnsFuture =
        std::is_invocable_r_v<RepeaterFutureResult<T, FailureT, std::decay_t<Args>...>, Func, Args...>;
    constexpr bool returnsData = std::is_invocable_r_v<RepeaterResult<T, std::decay_t<Args>...>, Func, Args...>;
    try {
        if constexpr (returnsFuture) {
            return f(std::forward<Args>(args)...)
                .flatMap([f](const RepeaterResult<T, std::decay_t<Args>...> &result) -> Future<T, FailureT> {
                    if (result.index() == 0)
                        return Future<T, FailureT>::successful(std::get<0>(result));
                    auto nextStep = std::apply(repeat<T, FailureT, Func, Args...>,
                                               std::tuple_cat(std::make_tuple(f), static_cast<std::tuple<Args...>>(
                                                                                      std::get<1>(result).second)));
                    if (std::get<1>(result).first == asynqro::detail::RepeaterBehavior::Trampolined)
                        return Trampoline(nextStep);
                    return nextStep;
                });
        } else if constexpr (returnsData) { // NOLINT(readability-else-after-return,readability-misleading-indentation)
            auto tupledArgs = std::make_tuple(std::forward<Args>(args)...);
            while (true) {
                detail::invalidateLastFailure();
                auto result = std::apply(f, std::move(tupledArgs));
                if (detail::hasLastFailure()) {
                    const auto lastFailure = detail::lastFailure<FailureT>();
                    detail::invalidateLastFailure();
                    return Future<T, FailureT>::failed(lastFailure);
                }
                if (result.index() == 0)
                    return Future<T, FailureT>::successful(std::move(std::get<0>(result)));
                std::swap(tupledArgs, std::get<1>(result).second);
            }
        } else { // NOLINT(readability-else-after-return,readability-misleading-indentation)
            static_assert(returnsFuture || returnsData,
                          "Function must be (Args...)->RepeaterFutureResult<T, FailureT, Args...> or "
                          "(Args...)->RepeaterResult<T, Args...>");
        }
    } catch (const std::exception &e) {
        return Future<T, FailureT>::failed(detail::exceptionFailure<FailureT>(e));
    } catch (...) {
        return Future<T, FailureT>::failed(detail::exceptionFailure<FailureT>());
    }
    return {};
}

namespace detail {
template <typename Data, template <typename...> typename Container, typename... Ds, typename Func, typename It,
          typename T, typename FailureT>
void iterateSequenceRepeater(Container<Data, Ds...> &&data, It current, T result,
                             const Promise<std::decay_t<T>, FailureT> &promise, Func &&f) noexcept
{
    while (current != data.cend()) {
        Future<std::decay_t<T>, FailureT> innerFuture;
        try {
            innerFuture = f(*current, std::forward<T>(result));
        } catch (const std::exception &e) {
            promise.failure(detail::exceptionFailure<FailureT>(e));
            return;
        } catch (...) {
            promise.failure(detail::exceptionFailure<FailureT>());
            return;
        }

        ++current;
        if (!innerFuture.isCompleted()) {
            innerFuture
                .onSuccess([data = std::move(data), current, promise,
                            f = std::forward<Func>(f)](const T &result) mutable noexcept {
                    iterateSequenceRepeater(std::move(data), current, result, promise, std::forward<Func>(f));
                })
                .onFailure([promise](const FailureT &reason) noexcept { promise.failure(reason); });
            return;
        }
        if (innerFuture.isFailed()) {
            promise.failure(innerFuture.failureReason());
            return;
        }
        result = innerFuture.result();
    }
    promise.success(std::forward<T>(result));
}
} // namespace detail

template <typename T, typename Data, template <typename...> typename Container, typename... Ds, typename Func,
          typename FailureT = typename std::invoke_result_t<Func, Data, T>::Failure>
Future<std::decay_t<T>, FailureT> repeatForSequence(Container<Data, Ds...> &&data, T initial, Func &&f) noexcept
{
    if (data.empty())
        return Future<T, FailureT>::successful(std::move(initial));
    Promise<std::decay_t<T>, FailureT> promise;
    detail::iterateSequenceRepeater(std::move(data), data.cbegin(), std::move(initial), promise, std::forward<Func>(f));
    return promise.future();
}

// This overload copies container to make sure that it will be reachable in future
template <typename T, typename Data, template <typename...> typename Container, typename... Ds, typename Func>
auto repeatForSequence(const Container<Data, Ds...> &data, T &&initial, Func &&f) noexcept
{
    Container<Data> copy(data);
    return repeatForSequence(std::move(copy), std::forward<T>(initial), std::forward<Func>(f));
}
} // namespace asynqro

#endif //ASYNQRO_REPEAT_H
