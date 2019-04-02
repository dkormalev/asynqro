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
#ifndef ASYNQRO_ZIPFUTURES_H
#define ASYNQRO_ZIPFUTURES_H

#include <tuple>
#include <variant>

namespace asynqro::detail {
template <typename T>
struct AsTuple
{
    using type = std::tuple<T>;
    static constexpr type make(const T &v) { return std::make_tuple(v); }
};

template <typename... T>
struct AsTuple<std::tuple<T...>>
{
    using type = std::tuple<T...>;
    static constexpr type make(const std::tuple<T...> &v) { return v; }
};

template <typename... T>
using AsTuple_T = typename AsTuple<T...>::type;

template <typename T>
struct AsVariant
{
    using type = std::variant<T>;
    static constexpr type make(const T &v) { return v; }
};

template <typename... T>
struct AsVariant<std::variant<T...>>
{
    using type = std::variant<T...>;
    static constexpr type make(const std::variant<T...> &v) { return v; }
};

template <typename T>
using AsVariant_T = typename AsVariant<T>::type;

template <typename T>
struct FromVariantIfSingle
{
    using type = T;
};

template <typename First, typename... Tail>
struct FromVariantIfSingle<std::variant<First, Tail...>>
{
    using type = std::conditional_t<sizeof...(Tail) == 0, First, std::variant<First, Tail...>>;
};

template <typename T>
using FromVariantIfSingle_T = typename FromVariantIfSingle<T>::type;

template <typename... T>
struct AddToVariant;

template <typename NewTypeFirst, typename NewTypeSecond, typename... NewTypes, typename... Types>
struct AddToVariant<std::variant<Types...>, std::variant<NewTypeFirst, NewTypeSecond, NewTypes...>>
{
    using type = typename AddToVariant<typename AddToVariant<std::variant<Types...>, NewTypeFirst>::type,
                                       std::variant<NewTypeSecond, NewTypes...>>::type;
};

template <typename NewType, typename... Types>
struct AddToVariant<std::variant<Types...>, std::variant<NewType>>
{
    using type = typename AddToVariant<std::variant<Types...>, NewType>::type;
};

template <typename NewType, typename... Types>
struct AddToVariant<std::variant<Types...>, NewType>
{
    using type = std::conditional_t<(std::is_same_v<Types, std::decay_t<NewType>> || ...), std::variant<Types...>,
                                    std::variant<Types..., std::decay_t<NewType>>>;
};

template <typename Base, typename Add>
using AddToVariant_T = typename AddToVariant<Base, Add>::type;

template <typename... T>
struct CanConvertVariant : std::false_type
{};

template <typename T>
struct CanConvertVariant<std::variant<T>, T> : std::true_type
{};
template <typename T>
struct CanConvertVariant<T, std::variant<T>> : std::true_type
{};
template <typename T>
struct CanConvertVariant<T, T> : std::true_type
{};
template <typename... T>
struct CanConvertVariant<std::variant<T...>, std::variant<T...>> : std::true_type
{};

template <typename... From, typename... To>
struct CanConvertVariant<std::variant<From...>, std::variant<To...>>
{
    static constexpr bool value =
        sizeof...(From) <= sizeof...(To)
        && std::is_same_v<std::variant<To...>, AddToVariant_T<std::variant<To...>, std::variant<From...>>>;
};

template <typename... T>
inline constexpr bool CanConvertVariant_V = CanConvertVariant<T...>::value;

template <typename... T>
using TypesProduct_T = decltype(std::tuple_cat(AsTuple_T<T>()...));

template <typename... T>
struct TypesSum;

template <typename T>
struct TypesSum<T>
{
    using wrappedType = AsVariant_T<T>;
    using type = FromVariantIfSingle_T<T>;
};

template <typename First, typename Second, typename... Tail>
struct TypesSum<First, Second, Tail...>
{
    using wrappedType = typename TypesSum<AddToVariant_T<AsVariant_T<First>, AsVariant_T<Second>>, Tail...>::wrappedType;
    using type = FromVariantIfSingle_T<wrappedType>;
};

template <typename... T>
using TypesSum_T = typename TypesSum<T...>::type;

} // namespace asynqro::detail

#endif // ASYNQRO_ZIPFUTURES_H
