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

#include <QVariant>

namespace asynqro {
namespace detail {
bool ASYNQRO_EXPORT hasLastFailure() noexcept;
QVariant ASYNQRO_EXPORT lastFailure() noexcept;
void ASYNQRO_EXPORT invalidateLastFailure() noexcept;
void ASYNQRO_EXPORT setLastFailure(const QVariant &failure) noexcept;

inline QVariant exceptionFailure(const std::exception &e)
{
    return QStringLiteral("Exception caught: %1").arg(e.what());
}
inline QVariant exceptionFailure()
{
    return QStringLiteral("Exception caught");
}
} // namespace detail

template <typename... T>
class Future;

struct WithFailure
{
    explicit WithFailure(const QVariant &f = QVariant()) noexcept { m_failure = f.isValid() ? f : QVariant(false); }
    template <typename T>
    operator T() noexcept
    {
        detail::setLastFailure(m_failure);
        return T();
    }
    template <typename T>
    operator Future<T>() noexcept
    {
        Future<T> result = Future<T>::create();
        result.fillFailure(m_failure);
        return result;
    }

private:
    QVariant m_failure;
};

} // namespace asynqro
#endif // ASYNQRO_FAILUREHANDLING_H
