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
#ifndef ASYNQRO_CONTAINERS_HELPERS_H
#define ASYNQRO_CONTAINERS_HELPERS_H

#include "asynqro/impl/typetraits.h"

#ifdef ASYNQRO_QT_SUPPORT
#    include <QHash>
#    include <QMap>
#    include <QMultiHash>
#    include <QMultiMap>
#    include <QPair>
#    include <QSet>
#endif

#include <iterator>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace asynqro::traverse::detail {
using namespace asynqro::detail;
template <int current>
struct Level : Level<current - 1>
{};
template <>
struct Level<0>
{};

namespace containers {

// clang-format off
template <typename C>
inline constexpr bool SetLike_V = IsSpecialization_V<C, std::set>
                                  || IsSpecialization_V<C, std::multiset>
                                  || IsSpecialization_V<C, std::unordered_set>
                                  || IsSpecialization_V<C, std::unordered_multiset>
#ifdef ASYNQRO_QT_SUPPORT
                                  || IsSpecialization_V<C, QSet>
#endif
;

#ifdef ASYNQRO_QT_SUPPORT
template <typename C>
inline constexpr bool QMapLike_V = IsSpecialization_V<C, QMap>
                                   || IsSpecialization_V<C, QHash>
                                   || IsSpecialization_V<C, QMultiMap>
                                   || IsSpecialization_V<C, QMultiHash>;
#else
template <typename C>
inline constexpr bool QMapLike_V = false;
#endif

template <typename C>
inline constexpr bool StlMapLike_V = IsSpecialization_V<C, std::map>
                                     || IsSpecialization_V<C, std::unordered_map>
                                     || IsSpecialization_V<C, std::multimap>
                                     || IsSpecialization_V<C, std::unordered_multimap>;
// clang-format on

template <typename C, typename T>
auto add(C &container, T &&value) -> decltype(container.push_back(value), void())
{
    container.push_back(std::forward<T>(value));
}

template <typename C, typename T, typename = std::enable_if_t<SetLike_V<C>>>
auto add(C &container, T &&value) -> decltype(container.insert(std::forward<T>(value)), void())
{
    container.insert(std::forward<T>(value));
}

template <typename C, typename T1, typename T2, typename = std::enable_if_t<StlMapLike_V<C>>>
auto add(C &container, std::pair<T1, T2> &&value) -> decltype(container.insert(std::move(value)), void())
{
    container.insert(std::move(value));
}

template <typename C, typename T1, typename T2, typename = std::enable_if_t<StlMapLike_V<C>>>
auto add(C &container, const std::pair<T1, T2> &value) -> decltype(container.insert(value), void())
{
    container.insert(value);
}

template <typename C, template <typename T1, typename T2> typename Pair, typename T1, typename T2>
auto add(C &container, Pair<T1, T2> &&value)
    -> decltype(container.insert(std::make_pair(value.first, value.second)), void())
{
    container.insert(std::make_pair(value.first, value.second));
}

template <typename C, typename T, typename = std::enable_if_t<!StlMapLike_V<C>>>
auto add(C &container, const T &value) -> decltype(container.insert(value.key(), value.value()), void())
{
    container.insert(value.key(), value.value());
}

template <typename C, typename T, typename = std::enable_if_t<!StlMapLike_V<C>>>
auto add(C &container, const T &value) -> decltype(container.insert(value.first, value.second), void())
{
    container.insert(value.first, value.second);
}

template <typename C>
auto reserve(C &container, long long size, Level<1>) -> decltype(container.reserve(size), void())
{
    container.reserve(size);
}

template <typename C>
void reserve(C &, long long, Level<0>)
{}

// Caller
template <typename C>
void reserve(C &container, long long size)
{
    reserve(container, size, Level<1>{});
}

template <typename C>
auto begin(C &container, Level<3>) -> decltype(container.cbegin())
{
    return container.cbegin();
}

template <typename C>
auto begin(C &container, Level<2>) -> decltype(container.constBegin())
{
    return container.constBegin();
}

template <typename C>
auto begin(C &container, Level<1>) -> decltype(container.begin())
{
    return container.begin();
}

template <typename C>
auto begin(C &container, Level<0>) -> decltype(std::begin(container))
{
    return std::begin(container);
}

// Caller
template <typename C>
auto begin(C &container)
{
    return begin(container, Level<3>{});
}

template <typename C>
auto end(C &container, Level<3>) -> decltype(container.cend())
{
    return container.cend();
}

template <typename C>
auto end(C &container, Level<2>) -> decltype(container.constEnd())
{
    return container.constEnd();
}

template <typename C>
auto end(C &container, Level<1>) -> decltype(container.end())
{
    return container.end();
}

template <typename C>
auto end(C &container, Level<0>) -> decltype(std::end(container))
{
    return std::end(container);
}

// Caller
template <typename C>
auto end(C &container)
{
    return end(container, Level<3>{});
}
} // namespace containers

// TODO: remove this workaround with wrapper for const_cast after msvc fix its INTERNAL COMPILER ERROR
template <typename T>
T &constCastWrapper(const T &ref)
{
    return const_cast<T &>(ref);
}
} // namespace asynqro::traverse::detail

#endif // ASYNQRO_CONTAINERS_HELPERS_H
