#pragma once

#include <thread>
#include <mutex>
#include <iostream>

#ifdef _WIN32
//Sleep() function
#include <Windows.h>
#endif // WIND32


//synch between multi threads
class SynchronizableObject
{
public:
    enum FLAG
    {
        FIRST,
        LAST
    };

public:
    size_t m_threadCount = 0;
    size_t* m_synchFlag1 = 0;
    size_t* m_synchFlag2 = 0;
    std::mutex* m_sharedMutex1 = 0;
    std::mutex* m_sharedMutex2 = 0;

public:
    inline SynchronizableObject(size_t numThreads, size_t* synchFlag1, std::mutex* sharedMutex1, 
        size_t* synchFlag2, std::mutex* sharedMutex2)
    {
        m_synchFlag1 = synchFlag1;
        m_synchFlag2 = synchFlag2;
        m_sharedMutex1 = sharedMutex1;
        m_sharedMutex2 = sharedMutex2;
        m_threadCount = numThreads;
    };

public:
    //sleep time in millisec
    //arg1 pass to fastestThreadBeforeSynchCallback
    //arg2 pass to fastestThreadAfterSynchCallback
    inline void Synch(
        size_t sleepTime = 2, 
        void(*fastestThreadBeforeSynchCallback)(void*) = 0, void* arg1 = 0,
        void(*fastestThreadAfterSynchCallback)(void*) = 0, void* arg2 = 0
    ) 
    {
        size_t ret = 0;
        m_sharedMutex1->lock();

        ret = *m_synchFlag1;

        (*m_synchFlag1)++;

        if (ret == FIRST)
        {
            m_sharedMutex2->lock();
        }

        m_sharedMutex1->unlock();

        if (ret == FIRST)
        {
            (*m_synchFlag2) = 1;

            if (fastestThreadBeforeSynchCallback) fastestThreadBeforeSynchCallback(arg1);

            while (*m_synchFlag1 != m_threadCount)
            {
                if (sleepTime) Sleep(sleepTime);
            }

            if (fastestThreadAfterSynchCallback) fastestThreadAfterSynchCallback(arg2);

            m_sharedMutex1->lock();
            *m_synchFlag1 = 0;

            m_sharedMutex2->unlock();

            while (*m_synchFlag2 != m_threadCount)
            {
                if (sleepTime) Sleep(sleepTime);
            }
            m_sharedMutex1->unlock();
        }
        else
        {
            m_sharedMutex2->lock();
            (*m_synchFlag2)++;
            m_sharedMutex2->unlock();
        }
    };

    inline void Desynch()
    {
        while (*m_synchFlag1 != m_threadCount)
        {
            *m_synchFlag1 = m_threadCount;
            Sleep(200);
        }

        while (*m_synchFlag2 != m_threadCount)
        {
            *m_synchFlag2 = m_threadCount;
            Sleep(200);
        }
    }
};