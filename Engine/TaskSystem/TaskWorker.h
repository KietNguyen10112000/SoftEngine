#pragma once

#include "Core/TypeDef.h"
#include "Core/Fiber/FiberInfo.h"

#include <condition_variable>

#include "TaskSystem.h"

NAMESPACE_BEGIN

class TaskWorker
{
protected:
	API static TaskWorker s_workers[FiberInfo::TOTAL_FIBERS];

protected:
	bool m_isRunning = true;
	Task m_currentTask = {};
	TaskSystem::SynchContext* m_currentSynchCtx = 0;

protected:
	inline void ExecuteCurrentTask()
	{
		m_currentTask.m_main(m_currentTask.m_params);

		if (m_currentTask.m_handle && m_currentTask.m_handle->waitingFiber && (--(m_currentTask.m_handle->counter)) == 0)
		{
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
				TaskSystem::WorkerWait();
				TaskSystem::WorkerResume();
				continue;
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
	API static void Initalize();
	API static void Finalize();

	// get the current task worker of current fiber
	inline static TaskWorker* Get()
	{
		return &s_workers[Thread::GetCurrentFiberID()];
	}

};

NAMESPACE_END