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
#ifndef ASYNQRO_PROMISE_H
#define ASYNQRO_PROMISE_H

#include <QVariant>

namespace asynqro {
template <typename... T>
class Future;

template <typename T>
class Promise
{
    static_assert(!std::is_same_v<T, void>, "Promise<void> is not allowed. Use Promise<bool> instead");

public:
    using Value = T;
    Promise() = default;
    Promise(const Promise<T> &) noexcept = default;
    Promise(Promise<T> &&) noexcept = default;
    Promise &operator=(const Promise<T> &) noexcept = default;
    Promise &operator=(Promise<T> &&) noexcept = default;
    ~Promise() = default;

    Future<T> future() const { return m_future; }

    bool isFilled() const noexcept { return m_future.isCompleted(); }
    void failure(const QVariant &reason) const noexcept { m_future.fillFailure(reason); }
    void success(T &&result) const noexcept { m_future.fillSuccess(std::forward<T>(result)); }
    void success(const T &result) const noexcept { m_future.fillSuccess(result); }

private:
    Future<T> m_future = Future<T>::create();
};

} // namespace asynqro

#endif // ASYNQRO_PROMISE_H
