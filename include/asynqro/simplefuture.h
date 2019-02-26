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
#ifndef ASYNQRO_SIMPLEFUTURE_H
#define ASYNQRO_SIMPLEFUTURE_H

#include "asynqro/asynqro"

namespace asynqro {
namespace failure {
template <>
inline std::any failureFromString<std::any>(const std::string &s)
{
    return std::any(s);
}
} // namespace failure

namespace simple {
template <typename T>
using Future = ::asynqro::Future<T, std::any>;
template <typename T>
using CancelableFuture = ::asynqro::CancelableFuture<T, std::any>;
template <typename T>
using Promise = ::asynqro::Promise<T, std::any>;
using Runner = ::asynqro::tasks::TaskRunner<std::any>;

template <typename T>
auto successful(T &&value) noexcept
{
    return Future<std::decay_t<T>>::successful(std::forward<T>(value));
}

template <typename T>
auto successful(const T &value) noexcept
{
    return Future<T>::successful(value);
}

// This overload copies container to make sure that it will be reachable in future
template <typename T, template <typename...> typename F, template <typename...> typename Container, typename... Fs>
Future<Container<T>> sequence(const Container<F<T, std::any>, Fs...> &container) noexcept
{
    Container<F<T, std::any>> copy = container;
    return Future<T>::sequence(std::move(copy));
}

template <typename T, template <typename...> typename F, template <typename...> typename Container, typename... Fs>
Future<Container<T>> sequence(Container<F<T, std::any>, Fs...> &&container) noexcept
{
    return Future<T>::sequence(std::move(container));
}
} // namespace simple
} // namespace asynqro

#endif // ASYNQRO_SIMPLEFUTURE_H
