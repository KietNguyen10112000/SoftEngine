#pragma once

#include "Core/TypeDef.h"
#include "Core/Fiber/FiberInfo.h"
#include "Core/Memory/Memory.h"

#include <condition_variable>

#include "TaskSystem.h"

NAMESPACE_BEGIN

class API TaskWorker
{
protected:
	constexpr static size_t SPIN_TIMES = 5;

	friend class TaskSystem;

	static TaskWorker s_workers[FiberInfo::TOTAL_FIBERS];
	static size_t s_totalInitializedWorker;

protected:
	size_t m_bindedThreadId = 0;
	std::thread m_bindedThread;
	bool m_isRunning = true;
	Task m_currentTask = {};
	TaskSystem::SynchContext* m_currentSynchCtx = 0;

protected:
	inline void ExecuteCurrentTask()
	{
		m_currentTask.m_main(m_currentTask.m_params);

		if (m_currentTask.m_handle && m_currentTask.m_handle->waitingFiber && (--(m_currentTask.m_handle->counter)) == 0)
		{
			//rheap::Delete(m_currentTask.m_handle);

			if (m_currentSynchCtx)
			{
				TaskSystem::Resume(m_currentTask.m_handle->waitingFiber);
			}
			else
			{
				Thread::SwitchToFiber(m_currentTask.m_handle->waitingFiber, true);
			}
		}
	}

public:
	inline void Main()
	{
		while (m_isRunning)
		{
			if (TaskSystem::Take(m_currentTask, m_currentSynchCtx) == false)
			{
				bool pause = true;

				// spins before wait
				for (size_t i = 0; i < SPIN_TIMES; i++)
				//while (true)
				{
					if (TaskSystem::Take(m_currentTask, m_currentSynchCtx) == true)
					{
						pause = false;
						break;
					}

					std::this_thread::yield();
				}

				if (pause)
				{
					TaskSystem::WorkerWait();
					TaskSystem::WorkerResume();
					continue;
				}
			}

			ExecuteCurrentTask();

			if (m_currentSynchCtx)
			{
				while (m_currentSynchCtx->tasks.try_dequeue(m_currentTask))
				{
					ExecuteCurrentTask();
				}

				m_currentSynchCtx->isRunning = false;
				m_currentSynchCtx->lock.unlock();
				m_currentSynchCtx = 0;
			}
		}
	}

	inline auto& IsRunning() 
	{ 
		return m_isRunning; 
	}

public:
	static void Initalize(size_t maxWorker = 8);
	static void Finalize();

	// get the current task worker of current fiber
	inline static TaskWorker* Get()
	{
		return &s_workers[Thread::GetCurrentFiberID()];
	}

};

NAMESPACE_END