#ifndef FUTUREBASETEST_H
#define FUTUREBASETEST_H

#define ASYNQRO_DEBUG_COUNT_OBJECTS
#include "asynqro/asynqro"

#include "gtest/gtest.h"

#include <chrono>

using namespace std::chrono_literals;

namespace asynqro {
namespace failure {
template <>
inline int failureFromString<int>(std::string &&s)
{
    return 42;
}
} // namespace failure
} // namespace asynqro

using namespace asynqro;
using namespace asynqro::detail;

template <typename T>
using TestPromise = Promise<T, std::string>;
template <typename T>
using TestFuture = Future<T, std::string>;
template <typename T>
using CancelableTestFuture = CancelableFuture<T, std::string>;
using WithTestFailure = WithFailure<std::string>;

class CommonFutureBaseTest : public testing::Test
{
protected:
    void TearDown() override
    {
        auto timeout = std::chrono::high_resolution_clock::now() + 10s;
        while (std::chrono::high_resolution_clock::now() < timeout && asynqro::instantFuturesUsage() != 0)
            ;
        EXPECT_EQ(0, asynqro::instantFuturesUsage());
    }
};

class FutureBaseTest : public CommonFutureBaseTest
{
protected:
    template <typename T, typename Failure>
    Future<T, Failure> createFuture(const Promise<T, Failure> &promise)
    {
        return promise.future();
    }
};

class CancelableFutureBaseTest : public CommonFutureBaseTest
{
protected:
    template <typename T, typename Failure>
    CancelableFuture<T, Failure> createFuture(const Promise<T, Failure> &promise)
    {
        return CancelableFuture<T, Failure>(promise);
    }
};
#endif // FUTUREBASETEST_H
