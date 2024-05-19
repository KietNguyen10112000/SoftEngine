#pragma once
#include "Core/Memory/TypeDef.h"

//#include "ThreadID.h"
#include "Thread.h"

#include <atomic>
#include <thread>
#include <mutex>
#include <cassert>

NAMESPACE_BEGIN

class ReentrantLock
{
    std::atomic<bool> lock_ = { 0 };

    int32_t counter = 0;
    size_t fiberId = -1;

public:
    inline void lock() noexcept 
    {
        if (fiberId != -1)
        {
            if (fiberId == Thread::GetCurrentFiberID())
            {
                counter++;
                return;
            }
        }

        for (;;) {
            // Optimistically assume the lock is free on the first try
            if (!lock_.exchange(true, std::memory_order_acquire)) {
                assert(fiberId == -1 && counter == 0);
                fiberId = Thread::GetCurrentFiberID();
                counter++;
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

        if (fiberId != -1)
        {
            if (fiberId == Thread::GetCurrentFiberID())
            {
                counter++;
                return true;
            }
        }

        auto ret = !lock_.load(std::memory_order_relaxed) &&
            !lock_.exchange(true, std::memory_order_acquire);

        if (ret)
        {
            assert(fiberId == -1 && counter == 0);
            fiberId = Thread::GetCurrentFiberID();
            counter++;
        }

        return ret;
    }

    inline void unlock() noexcept {
        assert(fiberId == Thread::GetCurrentFiberID());

        counter--;
        if (counter == 0)
        {
            fiberId = -1;
            lock_.store(false, std::memory_order_release);
        }
    }

};

NAMESPACE_END