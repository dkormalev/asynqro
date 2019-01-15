#ifndef FUTUREBASETEST_H
#define FUTUREBASETEST_H

#define ASYNQRO_DEBUG_COUNT_OBJECTS
#include "asynqro/asynqro"

#include "gtest/gtest.h"

using namespace asynqro;
using namespace asynqro::detail;

class CommonFutureBaseTest : public testing::Test
{
protected:
    void TearDown() override
    {
        QTime timer;
        timer.start();
        while (timer.elapsed() < 10000 && asynqro::instantFuturesUsage() != 0)
            ;
        EXPECT_EQ(0, asynqro::instantFuturesUsage());
    }
};

class FutureBaseTest : public CommonFutureBaseTest
{
protected:
    template <typename T>
    Future<T> createFuture(const Promise<T> &promise)
    {
        return promise.future();
    }
};

class CancelableFutureBaseTest : public CommonFutureBaseTest
{
protected:
    template <typename T>
    CancelableFuture<T> createFuture(const Promise<T> &promise)
    {
        return CancelableFuture<T>(promise);
    }
};
#endif // FUTUREBASETEST_H
