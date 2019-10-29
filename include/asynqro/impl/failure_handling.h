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
#ifndef ASYNQRO_FAILUREHANDLING_H
#define ASYNQRO_FAILUREHANDLING_H

#include "asynqro/impl/asynqro_export.h"

#include <any>
#include <string>

namespace asynqro {
namespace failure {
template <typename Failure>
inline Failure failureFromString(std::string &&)
{
    return Failure();
}

template <typename Failure>
inline Failure failureFromString(const std::string &s)
{
    std::string copy = s;
    return failureFromString<Failure>(std::move(copy));
}

template <>
inline std::string failureFromString<std::string>(std::string &&s)
{
    return s;
}
} // namespace failure

namespace detail {
ASYNQRO_EXPORT bool hasLastFailure() noexcept;
ASYNQRO_EXPORT void invalidateLastFailure() noexcept;
ASYNQRO_EXPORT const std::any &lastFailureAny() noexcept;
ASYNQRO_EXPORT void setLastFailureAny(const std::any &failure) noexcept;

template <typename Failure>
inline Failure lastFailure() noexcept
{
    if (!hasLastFailure())
        return Failure();
    try {
        return std::any_cast<Failure>(lastFailureAny());
    } catch (...) {
        return Failure();
    }
}

template <typename Failure>
inline void setLastFailure(const Failure &failure) noexcept
{
    setLastFailureAny(std::any(failure));
}

template <typename Failure>
Failure exceptionFailure(const std::exception &e)
{
    return failure::failureFromString<Failure>(std::string("Exception: ") + e.what());
}

template <typename Failure>
Failure exceptionFailure()
{
    return failure::failureFromString<Failure>("Exception");
}
} // namespace detail

template <typename T, typename Failure>
class Future;

template <typename Failure>
struct WithFailure
{
    explicit WithFailure(const Failure &f = Failure()) noexcept { m_failure = f; }
    explicit WithFailure(Failure &&f) noexcept { m_failure = std::move(f); }
    template <typename... Args>
    explicit WithFailure(Args &&... args) noexcept
    {
        m_failure = Failure(std::forward<Args>(args)...);
    }

    template <typename T>
    operator T() noexcept // NOLINT(google-explicit-constructor)
    {
        detail::setLastFailure(std::move(m_failure));
        return T();
    }

    template <typename T>
    operator Future<T, Failure>() noexcept // NOLINT(google-explicit-constructor)
    {
        Future<T, Failure> result = Future<T, Failure>::create();
        result.fillFailure(std::move(m_failure));
        return result;
    }

private:
    Failure m_failure;
};

} // namespace asynqro
#endif // ASYNQRO_FAILUREHANDLING_H
