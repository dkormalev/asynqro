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
#ifndef ASYNQRO_TYPETRAITS_H
#define ASYNQRO_TYPETRAITS_H

#include <type_traits>

namespace asynqro::detail {
template <typename Checkable, template <typename...> typename... Wrapper>
struct IsSpecialization : std::false_type
{};

template <template <typename...> typename Wrapper, typename... Args>
struct IsSpecialization<Wrapper<Args...>, Wrapper> : std::true_type
{};

template <template <typename...> typename Wrapper, template <typename...> typename Wrapper2,
          template <typename...> typename... Others, typename... Args>
struct IsSpecialization<Wrapper<Wrapper2<Args...>>, Wrapper, Wrapper2, Others...>
    : IsSpecialization<Wrapper2<Args...>, Wrapper2, Others...>
{};

template <typename Checkable, template <typename...> typename... Ts>
inline constexpr bool IsSpecialization_V = IsSpecialization<Checkable, Ts...>::value;

template <typename Checkable>
struct TypeParamsCount
{
    static constexpr int value = 0;
};

template <template <typename...> class Wrapper, typename... Args>
struct TypeParamsCount<Wrapper<Args...>>
{
    static constexpr int value = sizeof...(Args);
};

template <typename T>
inline constexpr int TypeParamsCount_V = TypeParamsCount<T>::value;

template <typename T>
inline constexpr bool HasTypeParams_V = (TypeParamsCount_V<T>) > 0;

template <typename...>
struct InnerType;
template <template <typename...> typename C, typename T, typename... Ts>
struct InnerType<C<T, Ts...>>
{
    using type = T;
};
template <typename T>
using InnerType_T = typename InnerType<T>::type;

template <typename...>
struct WithInnerType;
template <template <typename...> typename C, typename... Ts, typename U>
struct WithInnerType<C<Ts...>, U>
{
    using type = C<U>;
};
template <typename T, typename U>
using WithInnerType_T = typename WithInnerType<T, U>::type;

template <typename T>
struct NestingLevel
{
    static constexpr int value = 0;
};

template <template <typename...> class Wrapper, typename FirstArg, typename... Args>
struct NestingLevel<Wrapper<FirstArg, Args...>>
{
    static constexpr int value = 1 + NestingLevel<FirstArg>::value;
};

template <typename T>
struct DependentFalse : std::false_type
{};

template <bool returnValue, typename T>
struct ValueTypeIfNeeded;

template <typename T>
struct ValueTypeIfNeeded<true, T>
{
    using type = typename T::Value;
};

template <typename T>
struct ValueTypeIfNeeded<false, T>
{
    using type = T;
};

template <bool returnValue, typename T>
using ValueTypeIfNeeded_T = typename ValueTypeIfNeeded<returnValue, T>::type;

} // namespace asynqro::detail

#endif // ASYNQRO_TYPETRAITS_H
