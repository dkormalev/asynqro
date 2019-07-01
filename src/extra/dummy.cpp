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
#include "asynqro/future.h"
#include "asynqro/simplefuture.h"
#include "asynqro/tasks.h"

#include <vector>

using namespace asynqro;

namespace {
auto f = Future<int, int>::successful(5).andThenValue(25.0);
auto p = Promise<int, int>();
auto f2 = p.future()
              .recover([](auto f) { return f; })
              .recoverWith([](auto f) { return Future<int, int>::failed(f); })
              .recoverValue(5)
              .map([](auto) { return 5; })
              .filter([](auto) { return true; })
              .flatMap([](auto) { return f; })
              .andThen([]() { return f; })
              .mapFailure([](auto fail) { return fail; });
auto r = f2.zip(tasks::run([]() { int x = 5 + 2; })).zipValue(5).result();

auto f3 = p.future() >> ([](auto) { return 5; }) >> ([](auto) { return f; }) >> []() { return f; };
auto r2 = (f3 + tasks::run([]() { int x = 5 + 2; })).result();

auto f4 = Future<double, int>::sequence(std::vector{f, f2, f3});
auto f5 = f4.innerMap([](auto) { return 5.0; })
              .innerFilter([](auto) { return true; })
              .innerReduce([](auto, auto) { return 5; }, 0);

auto f6 = Future<double, int>::sequenceWithFailures(std::vector{f, f2, f3});

auto sf1 = simple::successful(5);
auto sf2 = simple::successful(5);
auto sf3 = simple::sequence(std::vector{sf1, sf2});
auto sf4 = simple::sequenceWithFailures(std::vector{sf1, sf2});
} // namespace
