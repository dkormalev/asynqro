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
#ifndef ASYNQRO_CONTAINERS_TRAVERSE_H
#define ASYNQRO_CONTAINERS_TRAVERSE_H

#include "asynqro/impl/containers_helpers.h"
#include "asynqro/impl/typetraits.h"

#include <QPair>

#include <type_traits>

namespace asynqro::traverse {
namespace detail {
using namespace asynqro::detail;
}

template <typename C, typename Func, typename Result = typename C::value_type>
auto findIf(const C &src, const Func &f, const Result &defaultValue = Result())
    -> decltype(f(*detail::containers::begin(src)), Result())
{
    auto end = detail::containers::end(src);
    for (auto it = detail::containers::begin(src); it != end; ++it) {
        if (f(*it))
            return *it;
    }
    return defaultValue;
}

template <typename C, typename Func, typename Result = QPair<typename C::key_type, typename C::mapped_type>>
auto findIf(const C &src, const Func &f, const Result &defaultValue = Result())
    -> decltype(f(detail::containers::begin(src).key(), detail::containers::begin(src).value()), Result())
{
    auto end = detail::containers::end(src);
    for (auto it = detail::containers::begin(src); it != end; ++it) {
        if (f(it.key(), it.value()))
            return qMakePair(it.key(), it.value());
    }
    return defaultValue;
}

template <typename C, typename Func, typename Result = QPair<typename C::key_type, typename C::mapped_type>>
auto findIf(const C &src, const Func &f, const Result &defaultValue = Result())
    -> decltype(f(detail::containers::begin(src)->first, detail::containers::begin(src)->second), Result())
{
    auto end = detail::containers::end(src);
    for (auto it = detail::containers::begin(src); it != end; ++it) {
        if (f(it->first, it->second))
            return qMakePair(it->first, it->second);
    }
    return defaultValue;
}

template <typename C, typename Func>
auto filter(const C &src, const Func &f)
    -> decltype(detail::containers::add(detail::constCastWrapper(src), *(detail::containers::begin(src))),
                f(*detail::containers::begin(src)), C())
{
    C result;
    auto end = detail::containers::end(src);
    for (auto it = detail::containers::begin(src); it != end; ++it) {
        if (f(*it))
            detail::containers::add(result, *it);
    }
    return result;
}

template <typename C, typename Func>
auto filter(const C &src, const Func &f)
    -> decltype(detail::containers::add(detail::constCastWrapper(src), *detail::containers::begin(src)),
                f(detail::containers::begin(src)->first, detail::containers::begin(src)->second), C())
{
    C result;
    auto end = detail::containers::end(src);
    for (auto it = detail::containers::begin(src); it != end; ++it) {
        if (f(it->first, it->second))
            detail::containers::add(result, *it);
    }
    return result;
}

template <typename C, typename Func>
auto filter(const C &src, const Func &f)
    -> decltype(detail::containers::add(detail::constCastWrapper(src), detail::containers::begin(src)),
                f(detail::containers::begin(src).key(), detail::containers::begin(src).value()), C())
{
    C result;
    auto end = detail::containers::end(src);
    for (auto it = detail::containers::begin(src); it != end; ++it) {
        if (f(it.key(), it.value()))
            detail::containers::add(result, it);
    }
    return result;
}

// No indices, zero sockets
template <typename C, typename Func, typename Result, typename = typename std::enable_if_t<!detail::HasTypeParams_V<C>>>
auto map(const C &src, const Func &f, Result dest)
    -> decltype(detail::containers::add(dest, f(*detail::containers::begin(src))), Result())
{
    auto end = detail::containers::end(src);
    detail::containers::reserve(dest, src.size());
    for (auto it = detail::containers::begin(src); it != end; ++it)
        detail::containers::add(dest, f(*it));
    return dest;
}

// With indices, zero sockets
template <typename C, typename Func, typename Result, typename = typename std::enable_if_t<!detail::HasTypeParams_V<C>>>
auto map(const C &src, const Func &f, Result dest)
    -> decltype(detail::containers::add(dest, f(0ll, *detail::containers::begin(src))), Result())
{
    auto end = detail::containers::end(src);
    detail::containers::reserve(dest, src.size());
    long long counter = -1;
    for (auto it = detail::containers::begin(src); it != end; ++it)
        detail::containers::add(dest, f(++counter, *it));
    return dest;
}

// No indices, one socket
template <template <typename...> class C, typename Input, typename Func, typename Result>
auto map(const C<Input> &src, const Func &f, Result dest)
    -> decltype(detail::containers::add(dest, f(*detail::containers::begin(src))), Result())
{
    auto end = detail::containers::end(src);
    detail::containers::reserve(dest, src.size());
    for (auto it = detail::containers::begin(src); it != end; ++it)
        detail::containers::add(dest, f(*it));
    return dest;
}

// With indices, one socket
template <template <typename...> class C, typename Input, typename Func, typename Result>
auto map(const C<Input> &src, const Func &f, Result dest)
    -> decltype(detail::containers::add(dest, f(0ll, *detail::containers::begin(src))), Result())
{
    auto end = detail::containers::end(src);
    detail::containers::reserve(dest, src.size());
    long long counter = -1;
    for (auto it = detail::containers::begin(src); it != end; ++it)
        detail::containers::add(dest, f(++counter, *it));
    return dest;
}

// Qt container, two sockets
template <template <typename...> class C, typename Key, typename Value, typename Func, typename Result>
auto map(const C<Key, Value> &src, const Func &f, Result dest)
    -> decltype(detail::containers::add(dest,
                                        f(detail::containers::begin(src).key(), detail::containers::begin(src).value())),
                Result())
{
    auto end = detail::containers::end(src);
    detail::containers::reserve(dest, src.size());
    for (auto it = detail::containers::begin(src); it != end; ++it)
        detail::containers::add(dest, f(it.key(), it.value()));
    return dest;
}

// STL container, two sockets
template <template <typename...> class C, typename Key, typename Value, typename Func, typename Result, typename... Extra>
auto map(const C<Key, Value, Extra...> &src, const Func &f, Result dest)
    -> decltype(detail::containers::add(dest, f(detail::containers::begin(src)->first,
                                                detail::containers::begin(src)->second)),
                Result())
{
    auto end = detail::containers::end(src);
    detail::containers::reserve(dest, src.size());
    for (auto it = detail::containers::begin(src); it != end; ++it)
        detail::containers::add(dest, f(it->first, it->second));
    return dest;
}

// One socket, no explicit dest
template <template <typename...> class C, typename T, typename Func>
auto map(const C<T> &src, const Func &f)
{
    if constexpr (std::is_invocable_v<Func, T>) {
        return map(src, f, C<std::invoke_result_t<Func, T>>());
    } else if constexpr (std::is_invocable_v<Func, long long, T>) {
        return map(src, f, C<std::invoke_result_t<Func, long long, T>>());
    } else {
        static_assert(detail::DependentFalse<Func>::value,
                      "Function passed to map is not supported. It should be either T->U or (int64, T)->U");
    }
}

// Two sockets, no explicit dest
template <template <typename...> class C, typename InputKey, typename InputValue, typename Func, typename... Args,
          typename Result = C<typename std::invoke_result_t<Func, InputKey, InputValue>::first_type,
                              typename std::invoke_result_t<Func, InputKey, InputValue>::second_type>>
Result map(const C<InputKey, InputValue, Args...> &src, const Func &f)
{
    return map(src, f, Result());
}

template <typename C, typename Func, typename Result>
auto reduce(const C &src, const Func &f, Result acc)
    -> decltype(acc = f(std::move(acc), *detail::containers::begin(src)), std::decay_t<Result>())
{
    auto end = detail::containers::end(src);
    for (auto it = detail::containers::begin(src); it != end; ++it)
        acc = std::move(f(std::move(acc), *it));
    return acc;
}

template <typename C, typename Func, typename Result>
auto reduce(const C &src, const Func &f, Result acc)
    -> decltype(acc = f(std::move(acc), detail::containers::begin(src).key(), detail::containers::begin(src).value()),
                std::decay_t<Result>())
{
    auto end = detail::containers::end(src);
    for (auto it = detail::containers::begin(src); it != end; ++it)
        acc = f(std::move(acc), it.key(), it.value());
    return acc;
}

template <typename C, typename Func, typename Result>
auto reduce(const C &src, const Func &f, Result acc)
    -> decltype(acc = f(std::move(acc), detail::containers::begin(src)->first, detail::containers::begin(src)->second),
                std::decay_t<Result>())
{
    auto end = detail::containers::end(src);
    for (auto it = detail::containers::begin(src); it != end; ++it)
        acc = f(std::move(acc), it->first, it->second);
    return acc;
}

template <template <typename...> class COuter, template <typename...> class CInner, typename T, typename Result>
auto flatten(const COuter<CInner<T>> &src, Result dest)
    -> decltype(detail::containers::add(dest, *detail::containers::begin(*detail::containers::begin(src))), Result())
{
    long long estimatedSize = 0;
    int estimationsLeft = 100;
    auto end = detail::containers::end(src);
    for (auto it = detail::containers::begin(src); it != end && estimationsLeft; ++it, --estimationsLeft)
        estimatedSize += it->size();
    if (!estimationsLeft)
        estimatedSize = static_cast<double>(estimatedSize) / 100.0 * src.size();
    detail::containers::reserve(dest, estimatedSize + dest.size());
    for (auto it = detail::containers::begin(src); it != end; ++it) {
        auto innerEnd = detail::containers::end(*it);
        for (auto innerIt = detail::containers::begin(*it); innerIt != innerEnd; ++innerIt)
            detail::containers::add(dest, *innerIt);
    }
    return dest;
}

template <template <typename...> class COuter, template <typename...> class CInner, typename T>
COuter<T> flatten(const COuter<CInner<T>> &src)
{
    return flatten(src, COuter<T>());
}

} // namespace asynqro::traverse

#endif // ASYNQRO_CONTAINERS_TRAVERSE_H
