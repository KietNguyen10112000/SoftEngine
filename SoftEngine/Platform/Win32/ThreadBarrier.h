#pragma once

#include <Windows.h>

//#include <mutex>

class ThreadBarrier
{
public:
	SYNCHRONIZATION_BARRIER m_barrier1;
	//SYNCHRONIZATION_BARRIER m_barrier2;

	int m_threadCount = 0;
	int m_currentThreadCount = 0;

	//std::mutex m_mutex;

	inline ThreadBarrier(int threadCount)
	{
		InitializeSynchronizationBarrier(
			&m_barrier1,
			threadCount,
			-1);

		/*InitializeSynchronizationBarrier(
			&m_barrier2,
			threadCount,
			100);*/

		m_threadCount = threadCount;
	};

public:
	inline void Desynch()
	{
		EnterSynchronizationBarrier(
			&m_barrier1,
			SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);
		DeleteSynchronizationBarrier(&m_barrier1);

		/*EnterSynchronizationBarrier(
			&m_barrier2,
			SYNCHRONIZATION_BARRIER_FLAGS_SPIN_ONLY);
		DeleteSynchronizationBarrier(&m_barrier2);*/
	};

	inline void Synch(void (*func)(void*), void* args)
	{
		EnterSynchronizationBarrier(
			&m_barrier1,
			SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY);

		////int threadPos = 0;
		//m_mutex.lock();
		////threadPos = m_currentThreadCount;
		//m_currentThreadCount++;
		//m_mutex.unlock();

		InterlockedIncrement((uint32_t*)&m_currentThreadCount);

		if (func)
		{
			while (m_currentThreadCount != m_threadCount) {}

			func(args);

			//m_mutex.lock();
			m_currentThreadCount = 0;
			//m_mutex.unlock();
		}
		else
		{
			while (m_currentThreadCount != 0) {}
		}

		/*EnterSynchronizationBarrier(
			&m_barrier2,
			SYNCHRONIZATION_BARRIER_FLAGS_SPIN_ONLY);*/
	};

	inline void Synch(void (*func)(void*), void* args, void (*lastlyCall)(void*), void* args1)
	{
		if (EnterSynchronizationBarrier(
			&m_barrier1,
			SYNCHRONIZATION_BARRIER_FLAGS_BLOCK_ONLY))
		{
			if (lastlyCall) lastlyCall(args1);
		}

		InterlockedIncrement((uint32_t*)&m_currentThreadCount);

		if (func)
		{
			while (m_currentThreadCount != m_threadCount) {}
			func(args);
			m_currentThreadCount = 0;
		}
		else
		{
			while (m_currentThreadCount != 0) { Sleep(4); }
		}

	};

};