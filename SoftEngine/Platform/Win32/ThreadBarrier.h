#pragma once

#include <Windows.h>

#include <mutex>

//disable compiler reordering
#ifndef _DEBUG
#pragma optimize("", off)
#endif

//#define USING_STL_THREAD_BARRIER_IMPL
//#define USING_OS_THREAD_BARRIER_IMPL

// wait cpu cycles
#define USING_SINGLE_CRITICAL_SECTION_THREAD_BARRIER_IMPL
#define USING_INTERLOCKED_INCRE

class ThreadBarrier
{
public:
	SYNCHRONIZATION_BARRIER m_barrier1;

#ifdef USING_OS_THREAD_BARRIER_IMPL
	SYNCHRONIZATION_BARRIER m_barrier;
#endif

#ifdef USING_STL_THREAD_BARRIER_IMPL
	int m_threadCount = 0;
	int m_currentThreadCount = 0;

	std::mutex m_mutex;
	std::condition_variable m_cv;
#endif

#ifdef USING_SINGLE_CRITICAL_SECTION_THREAD_BARRIER_IMPL
#ifndef USING_INTERLOCKED_INCRE
	CRITICAL_SECTION m_criticalSection;
#endif // !USING_INTERLOCKED_INCRE
	int m_threadCount = 0;
	int m_incomingThreadCount = 0;
	int m_outcomingThreadCount = 0;
#endif


	inline ThreadBarrier(int threadCount)
	{
		InitializeSynchronizationBarrier(
			&m_barrier1,
			threadCount,
			-1);

#ifdef USING_OS_THREAD_BARRIER_IMPL

		InitializeSynchronizationBarrier(
			&m_barrier,
			threadCount,
			-1);
#endif

#ifdef USING_STL_THREAD_BARRIER_IMPL
		m_threadCount = threadCount;
#endif

#ifdef USING_SINGLE_CRITICAL_SECTION_THREAD_BARRIER_IMPL
#ifndef USING_INTERLOCKED_INCRE
		(void) InitializeCriticalSectionAndSpinCount(&m_criticalSection, 0x80000400);
#endif
		m_threadCount = threadCount;
#endif

	};

	inline ~ThreadBarrier()
	{
#ifdef USING_SINGLE_CRITICAL_SECTION_THREAD_BARRIER_IMPL
#ifndef USING_INTERLOCKED_INCRE
		DeleteCriticalSection(&m_criticalSection);
#endif
#endif
	};

public:
	inline void Desynch()
	{
		EnterSynchronizationBarrier(
			&m_barrier1,
			SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);
		DeleteSynchronizationBarrier(&m_barrier1);
	};


	inline void FastSynch()
	{
#ifdef USING_STL_THREAD_BARRIER_IMPL

		bool wait = false;

		m_mutex.lock();

		m_currentThreadCount = (m_currentThreadCount + 1) % m_threadCount;

		wait = (m_currentThreadCount != 0);

		m_mutex.unlock();

		if (wait)
		{
			std::unique_lock<std::mutex> lk(m_mutex);
			m_cv.wait(lk);
		}
		else
		{
			m_cv.notify_all();
		}
#endif // USING_STL_THREAD_BARRIER_IMPL

#ifdef USING_OS_THREAD_BARRIER_IMPL
		EnterSynchronizationBarrier(
			&m_barrier,
			SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);
#endif // USING_STL_THREAD_BARRIER_IMPL



#ifdef USING_SINGLE_CRITICAL_SECTION_THREAD_BARRIER_IMPL

		bool wait = false;

#ifndef USING_INTERLOCKED_INCRE
		EnterCriticalSection(&m_criticalSection);

		m_incomingThreadCount = (m_incomingThreadCount + 1) % m_threadCount;

		wait = (m_incomingThreadCount != 0);

		LeaveCriticalSection(&m_criticalSection);
#else
		auto count = InterlockedIncrement(reinterpret_cast<uint32_t*>(& m_incomingThreadCount));
		wait = (count != m_threadCount);
#endif

		if (wait)
		{
			while (m_incomingThreadCount != 0) { /*std::this_thread::yield();*/ };

#ifndef USING_INTERLOCKED_INCRE
			EnterCriticalSection(&m_criticalSection);	
			m_outcomingThreadCount++;
			LeaveCriticalSection(&m_criticalSection);
#else
			InterlockedIncrement(reinterpret_cast<uint32_t*>(&m_outcomingThreadCount));
#endif
		}
		else
		{
#ifdef USING_INTERLOCKED_INCRE
			m_incomingThreadCount = 0;
#endif
			while (m_outcomingThreadCount != m_threadCount - 1) { };
			m_outcomingThreadCount = 0;
		}

#endif

	};

	inline void SlowSynch()
	{
		EnterSynchronizationBarrier(
			&m_barrier1,
			SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);
	};

};

#ifndef _DEBUG
#pragma optimize("", on)
#endif