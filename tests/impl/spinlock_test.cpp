#include "asynqro/impl/spinlock.h"

#include "gtest/gtest.h"

using namespace asynqro::detail;

using namespace std::chrono_literals;

TEST(SpinLockTest, lock)
{
    SpinLock lock;
    std::atomic_bool done{false};
    std::thread([&lock, &done]() {
        lock.lock();
        done = true;
    }).detach();
    auto timeout = std::chrono::high_resolution_clock::now() + 10s;
    while (!done && std::chrono::high_resolution_clock::now() < timeout)
        ;
    EXPECT_FALSE(lock.tryLock());
    lock.unlock();
    EXPECT_TRUE(lock.tryLock());
}

TEST(SpinLockTest, tryLock)
{
    SpinLock lock;
    std::atomic_bool done{false};
    std::thread([&lock, &done]() {
        lock.tryLock();
        done = true;
    }).detach();
    auto timeout = std::chrono::high_resolution_clock::now() + 10s;
    while (!done && std::chrono::high_resolution_clock::now() < timeout)
        ;
    EXPECT_FALSE(lock.tryLock());
}

TEST(SpinLockTest, tryLockNegative)
{
    SpinLock lock;
    lock.lock();
    std::atomic_bool done{false};
    std::atomic_bool result{true};
    std::thread([&lock, &done, &result]() {
        result = lock.tryLock();
        done = true;
    }).detach();
    auto timeout = std::chrono::high_resolution_clock::now() + 10s;
    while (!done && std::chrono::high_resolution_clock::now() < timeout)
        ;
    EXPECT_FALSE(result);
}

TEST(SpinLockTest, holder)
{
    SpinLock lock;
    std::atomic_bool done{false};
    std::atomic_bool readyToDie{false};
    std::thread([&lock, &done, &readyToDie]() {
        SpinLockHolder holder(&lock);
        done = true;
        while (!readyToDie)
            ;
    }).detach();
    auto timeout = std::chrono::high_resolution_clock::now() + 10s;
    while (!done && std::chrono::high_resolution_clock::now() < timeout)
        ;
    EXPECT_FALSE(lock.tryLock());
    readyToDie = true;
    std::this_thread::sleep_for(100ms);
    EXPECT_TRUE(lock.tryLock());
}

TEST(SpinLockTest, holderWithAbandon)
{
    SpinLock lock;
    std::atomic_bool done{false};
    std::atomic_bool abandon{true};
    lock.lock();
    std::thread([&lock, &done, &abandon]() {
        SpinLockHolder holder(&lock, abandon);
        done = true;
    }).detach();
    auto timeout = std::chrono::high_resolution_clock::now() + 10s;
    while (!done && std::chrono::high_resolution_clock::now() < timeout)
        ;
    EXPECT_FALSE(lock.tryLock());
}

TEST(SpinLockTest, holderUnlock)
{
    SpinLock lock;
    SpinLockHolder holder(&lock);
    EXPECT_FALSE(lock.tryLock());
    holder.unlock();
    EXPECT_TRUE(lock.tryLock());
}

TEST(SpinLockTest, holderScoped)
{
    SpinLock lock;
    {
        SpinLockHolder holder(&lock);
        EXPECT_FALSE(lock.tryLock());
    }
    EXPECT_TRUE(lock.tryLock());
}
