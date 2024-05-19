#pragma once
#include "Core/Memory/TypeDef.h"

#include "ThreadID.h"
//#include "Thread.h"

#include <atomic>
#include <thread>
#include <mutex>
#include <cassert>

NAMESPACE_BEGIN

class spinlock 
{
    std::atomic<bool> lock_ = { 0 };

DEBUG_CODE(
    size_t fiberId = -1;
);

public:
    inline void lock() noexcept {

        /*DEBUG_CODE(if (threadId != -1)
        {
            assert(threadId != ThreadID::Get());
        });*/

#ifdef _DEBUG
        if (fiberId != -1)
        {
            if (fiberId == ThreadID::GetCurrentFiberID())
            {
                int x = 3;
            }
            assert(fiberId != ThreadID::GetCurrentFiberID());
        }
#endif // _DEBUG 

        for (;;) {
            // Optimistically assume the lock is free on the first try
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                DEBUG_CODE(
                    assert(fiberId == -1);
                    fiberId = ThreadID::GetCurrentFiberID();
                );
                return;
            }
            // Wait for lock to be released without generating cache misses
            while (lock_.load(std::memory_order_relaxed)) {
                // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
                // hyper-threads
                //_mm_pause();
                std::this_thread::yield();
            }
        }
    }

    inline bool try_lock() noexcept {
        // First do a relaxed load to check if lock is free in order to prevent
        // unnecessary cache misses if someone does while(!try_lock())

#ifdef _DEBUG
        DEBUG_CODE(if (fiberId != -1) assert(fiberId != ThreadID::GetCurrentFiberID()));

        auto ret = !lock_.load(std::memory_order_relaxed) &&
            !lock_.exchange(true, std::memory_order_acquire);

        if (ret)
        {
            assert(fiberId == -1);
            fiberId = ThreadID::GetCurrentFiberID();
        }

        return ret;
#else
        return !lock_.load(std::memory_order_relaxed) &&
            !lock_.exchange(true, std::memory_order_acquire);
#endif // _DEBUG

        
    }

    inline void unlock() noexcept {
        DEBUG_CODE(
            assert(fiberId == ThreadID::GetCurrentFiberID());
            fiberId = -1;
        );
        lock_.store(false, std::memory_order_release);
    }

    inline bool try_lock_no_check_own_thread() noexcept {
        DEBUG_CODE(
            fiberId = -1;
        );
        return !lock_.load(std::memory_order_relaxed) &&
            !lock_.exchange(true, std::memory_order_acquire);
    }

    inline bool try_lock_no_check_own_thread_2() noexcept {
        return !lock_.load(std::memory_order_relaxed) &&
            !lock_.exchange(true, std::memory_order_acquire);
    }

    inline void lock_no_check_own_thread() noexcept {
        for (;;) {
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                DEBUG_CODE(
                    assert(fiberId == -1);
                    fiberId = ThreadID::GetCurrentFiberID();
                );
                return;
            }
            while (lock_.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
        }
    }

    inline void unlock_no_check_own_thread() noexcept {
        DEBUG_CODE(
            fiberId = -1;
        );
        lock_.store(false, std::memory_order_release);
    }
};

using Spinlock = spinlock;
//using spinlock = std::mutex;

class SpinlockThreadBound
{
    std::atomic<bool> lock_ = { 0 };

DEBUG_CODE(
    size_t threadId = -1;
);

public:
    inline void lock() noexcept {

        /*DEBUG_CODE(if (threadId != -1)
        {
            assert(threadId != ThreadID::Get());
        });*/

#ifdef _DEBUG
        if (threadId != -1)
        {
            if (threadId == ThreadID::Get())
            {
                int x = 3;
            }
            assert(threadId != ThreadID::Get());
        }
#endif // _DEBUG 

        for (;;) {
            // Optimistically assume the lock is free on the first try
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                DEBUG_CODE(
                    assert(threadId == -1);
                    threadId = ThreadID::Get();
                );
                return;
            }
            // Wait for lock to be released without generating cache misses
            while (lock_.load(std::memory_order_relaxed)) {
                // Issue X86 PAUSE or ARM YIELD instruction to reduce contention between
                // hyper-threads
                //_mm_pause();
                std::this_thread::yield();
            }
        }
    }

    inline bool try_lock() noexcept {
        // First do a relaxed load to check if lock is free in order to prevent
        // unnecessary cache misses if someone does while(!try_lock())

#ifdef _DEBUG
        DEBUG_CODE(if (threadId != -1) assert(threadId != ThreadID::Get()));

        auto ret = !lock_.load(std::memory_order_relaxed) &&
            !lock_.exchange(true, std::memory_order_acquire);

        if (ret)
        {
            assert(threadId == -1);
            threadId = ThreadID::Get();
        }

        return ret;
#else
        return !lock_.load(std::memory_order_relaxed) &&
            !lock_.exchange(true, std::memory_order_acquire);
#endif // _DEBUG

        
    }

    inline void unlock() noexcept {
        DEBUG_CODE(
            assert(threadId == ThreadID::Get());
            threadId = -1;
        );
        lock_.store(false, std::memory_order_release);
    }

    inline bool try_lock_no_check_own_thread() noexcept {
        DEBUG_CODE(
            threadId = -1;
        );
        return !lock_.load(std::memory_order_relaxed) &&
            !lock_.exchange(true, std::memory_order_acquire);
    }

    inline bool try_lock_no_check_own_thread_2() noexcept {
        return !lock_.load(std::memory_order_relaxed) &&
            !lock_.exchange(true, std::memory_order_acquire);
    }

    inline void lock_no_check_own_thread() noexcept {
        for (;;) {
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                DEBUG_CODE(
                    assert(threadId == -1);
                    threadId = ThreadID::Get();
                );
                return;
            }
            while (lock_.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
        }
    }

    inline void unlock_no_check_own_thread() noexcept {
        DEBUG_CODE(
            threadId = -1;
        );
        lock_.store(false, std::memory_order_release);
    }
};

NAMESPACE_END