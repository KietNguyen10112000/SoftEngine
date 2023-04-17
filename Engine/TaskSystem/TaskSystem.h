#pragma once

#include "Core/TypeDef.h"
#include "Core/Structures/Raw/ConcurrentQueue.h"
#include "Core/Thread/Spinlock.h"
#include "Core/Thread/ThreadLimit.h"
#include "Core/Memory/Memory.h"

#include "Task.h"

#include "SubSystems/SubSystemInfo.h"

#include "TaskParamUnpack.h"

NAMESPACE_BEGIN

class Fiber;

class API TaskSystem
{
protected:
	friend class Task;
	friend class TaskWorker;

	// all tasks in SynchContext will run under lock guard
	struct SynchContext
	{
		ConcurrentQueue<Task> tasks;
		std::atomic<size_t> tasksCount = { 0 };
		Spinlock lock;
		bool isRunning = false;

		SynchContext();
		~SynchContext();
	};

	static std::condition_variable s_cv;
	static std::mutex s_mutex;
	static std::atomic<size_t> s_workingWorkersCount;
	static size_t s_workersCount;
	static SynchContext s_sychCtxs[SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT];
	static ConcurrentQueue<Fiber*> s_resumeFibers;
	static ConcurrentQueue<Task> s_queues[Task::PRIORITY::COUNT];

	// task that must run in specified thread
	static ConcurrentQueue<Task> s_threadQueues[ThreadLimit::MAX_THREADS];

public:
	static void Initialize();

protected:
	inline static void WorkerWait()
	{
		--s_workingWorkersCount;
		std::unique_lock lk(s_mutex);
		s_cv.wait(lk);
	}

	inline static void WorkerResume()
	{
		++s_workingWorkersCount;
	}

	inline static void Resume(Fiber* fiber)
	{
		s_resumeFibers.enqueue(fiber);
	}

	inline static void TryInvokeOneMoreWorker()
	{
		if (s_workingWorkersCount.load(std::memory_order_relaxed) != s_workersCount)
		{
			s_cv.notify_one();
		}
	}

	inline static void TryInvokeAllWorkers()
	{
		if (s_workingWorkersCount.load(std::memory_order_relaxed) != s_workersCount)
		{
			s_cv.notify_all();
		}
	}

	inline static bool Take(Task& output, SynchContext*& sych)
	{
	begin:
		//sych = 0;

		// thread specified task
		auto& threadQueue = s_threadQueues[ThreadID::Get()];
		if (threadQueue.size_approx() != 0 && threadQueue.try_dequeue(output))
		{
			return true;
		}

		//// synch task
		//for (auto& ctx : s_sychCtxs)
		//{
		//	if (ctx.isRunning == false && 
		//		ctx.tasks.size_approx() != 0
		//		&& ctx.lock.try_lock()
		//		&& ctx.tasks.try_dequeue(output))
		//	{
		//		sych = &ctx;
		//		ctx.isRunning = true;
		//		return true;
		//	}
		//}

		// resume task
		Fiber* fiber = 0;
		while (s_resumeFibers.size_approx() != 0 && s_resumeFibers.try_dequeue(fiber))
		{
			Thread::SwitchToFiber(fiber, true);
			goto begin;
		}

		for (auto& queue : s_queues)
		{
			if (queue.size_approx() != 0 && queue.try_dequeue(output))
			{
				return true;
			}
		}
		return false;
	}

protected:
	template <bool WAIT = false>
	inline static void SubmitOneTempl(const Task& task, Task::PRIORITY priority = Task::PRIORITY::NORMAL)
	{
		if constexpr (WAIT)
		{
			auto fiber = Thread::GetCurrentFiber();
			task.m_handle = (TaskWaitingHandle*)&fiber->m_localStorage[0];
			task.m_handle->counter = 1;
			task.m_handle->waitingFiber = fiber;
		}

		s_queues[priority].enqueue(task);
		TryInvokeOneMoreWorker();

		if constexpr (WAIT)
		{
			auto fiber = FiberPool::Take();
			Thread::SwitchToFiber(fiber, false);
		}
	}

	template <bool WAIT = false>
	inline static void SubmitManyTempl(const Task* tasks, size_t count, Task::PRIORITY priority = Task::PRIORITY::NORMAL)
	{
		TaskWaitingHandle* pHandle = 0;
		if constexpr (WAIT)
		{
			auto fiber = Thread::GetCurrentFiber();
			pHandle = (TaskWaitingHandle*)&fiber->m_localStorage[0];
			pHandle->counter = count;
			pHandle->waitingFiber = fiber;
		}

		auto& queue = s_queues[priority];

		for (size_t i = 0; i < count; i++)
		{
//#ifdef _DEBUG
//			// must same
//			if (i != 0) assert(tasks[i].m_main == tasks[i - 1].m_main);
//#endif // _DEBUG

			if constexpr (WAIT)
			{
				tasks[i].m_handle = pHandle;
			}

			queue.enqueue(tasks[i]);
		}

		TryInvokeAllWorkers();

		if constexpr (WAIT)
		{
			auto fiber = FiberPool::Take();
			Thread::SwitchToFiber(fiber, false);
		}
	}


	//==================================================================================================
	template <bool WAIT = false>
	inline static void SubmitOneSynchTempl(const Task& task, size_t sychContextId)
	{
		assert(sychContextId < SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT);

		if constexpr (WAIT)
		{
			auto fiber = Thread::GetCurrentFiber();
			task.m_handle = (TaskWaitingHandle*)&fiber->m_localStorage[0];
			task.m_handle->counter = 1;
			task.m_handle->waitingFiber = fiber;
		}

		s_sychCtxs[sychContextId].tasks.enqueue(task);
		TryInvokeOneMoreWorker();

		if constexpr (WAIT)
		{
			auto fiber = FiberPool::Take();
			Thread::SwitchToFiber(fiber, false);
		}
	}

	template <bool WAIT = false>
	inline static void SubmitManySynchTempl(const Task* tasks, size_t count, size_t sychContextId)
	{
		assert(sychContextId < SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT);

		TaskWaitingHandle* pHandle = 0;
		if constexpr (WAIT)
		{
			auto fiber = Thread::GetCurrentFiber();
			pHandle = (TaskWaitingHandle*)&fiber->m_localStorage[0];
			pHandle->counter = count;
			pHandle->waitingFiber = fiber;
		}

		auto& queue = s_sychCtxs[sychContextId].tasks;
		for (size_t i = 0; i < count; i++)
		{
#ifdef _DEBUG
			// must same
			if (i != 0) assert(tasks[i].m_main == tasks[i - 1].m_main);
#endif // _DEBUG

			if constexpr (WAIT)
			{
				tasks[i].m_handle = pHandle;
			}

			queue.enqueue(tasks[i]);
		}

		TryInvokeOneMoreWorker();

		if constexpr (WAIT)
		{
			auto fiber = FiberPool::Take();
			Thread::SwitchToFiber(fiber, false);
		}
	}


	//==================================================================================================
	template <bool WAIT = false>
	inline static void SubmitOneForThreadTempl(const Task& task, size_t threadID)
	{
		if constexpr (WAIT)
		{
			auto fiber = Thread::GetCurrentFiber();
			task.m_handle = (TaskWaitingHandle*)&fiber->m_localStorage[0];
			task.m_handle->counter = 1;
			task.m_handle->waitingFiber = fiber;
		}

		s_threadQueues[threadID].enqueue(task);
		TryInvokeAllWorkers();

		if constexpr (WAIT)
		{
			auto fiber = FiberPool::Take();
			Thread::SwitchToFiber(fiber, false);
		}
	}

	template <bool WAIT = false>
	inline static void SubmitManyForThreadTempl(const Task* tasks, size_t count, size_t threadID)
	{
		TaskWaitingHandle* pHandle = 0;
		if constexpr (WAIT)
		{
			auto fiber = Thread::GetCurrentFiber();
			pHandle = (TaskWaitingHandle*)&fiber->m_localStorage[0];
			pHandle->counter = count;
			pHandle->waitingFiber = fiber;
		}

		auto& queue = s_queues[threadID];

		for (size_t i = 0; i < count; i++)
		{
#ifdef _DEBUG
			// must same
			if (i != 0) assert(tasks[i].m_main == tasks[i - 1].m_main);
#endif // _DEBUG

			if constexpr (WAIT)
			{
				tasks[i].m_handle = pHandle;
			}

			queue.enqueue(tasks[i]);
		}

		TryInvokeAllWorkers();

		if constexpr (WAIT)
		{
			auto fiber = FiberPool::Take();
			Thread::SwitchToFiber(fiber, false);
		}
	}

public:
	inline static void Submit(const Task& task, Task::PRIORITY priority = Task::PRIORITY::NORMAL)
	{
		SubmitOneTempl<false>(task, priority);
	}

	inline static void SubmitAndWait(const Task& task, Task::PRIORITY priority = Task::PRIORITY::NORMAL)
	{
		SubmitOneTempl<true>(task, priority);
	}

	inline static void Submit(const Task* tasks, size_t count, Task::PRIORITY priority = Task::PRIORITY::NORMAL)
	{
		SubmitManyTempl<false>(tasks, count, priority);
	}

	inline static void SubmitAndWait(const Task* tasks, size_t count, Task::PRIORITY priority = Task::PRIORITY::NORMAL)
	{
		SubmitManyTempl<true>(tasks, count, priority);
	}


	//==================================================================================================
	inline static void SubmitSynch(const Task& task, Task::PRIORITY priority = Task::PRIORITY::NORMAL)
	{
		SubmitOneSynchTempl<false>(task, priority);
	}

	inline static void SubmitAndWaitSynch(const Task& task, Task::PRIORITY priority = Task::PRIORITY::NORMAL)
	{
		SubmitOneSynchTempl<true>(task, priority);
	}

	inline static void SubmitSynch(const Task* tasks, size_t count, Task::PRIORITY priority = Task::PRIORITY::NORMAL)
	{
		SubmitManySynchTempl<false>(tasks, count, priority);
	}

	inline static void SubmitAndWaitSynch(const Task* tasks, size_t count, Task::PRIORITY priority = Task::PRIORITY::NORMAL)
	{
		SubmitManySynchTempl<true>(tasks, count, priority);
	}


	//==================================================================================================
	inline static void SubmitForThread(const Task& task, size_t threadId)
	{
		SubmitOneForThreadTempl<false>(task, threadId);
	}

	inline static void SubmitForThreadAndWait(const Task& task, size_t threadId)
	{
		SubmitOneForThreadTempl<true>(task, threadId);
	}

	inline static void SubmitForThread(const Task* tasks, size_t count, size_t threadId)
	{
		SubmitManyForThreadTempl<false>(tasks, count, threadId);
	}

	/*inline static void SubmitAndWait(const Task* tasks, size_t count, size_t threadId)
	{
		SubmitManyForThreadTempl<true>(tasks, count, threadId);
	}*/



	// use these 3 function to do dynamic submit-wait tasking
	inline static void PrepareHandle(TaskWaitingHandle* handle)
	{
		handle->counter = 1;
		handle->waitingFiber = Thread::GetCurrentFiber();
	}

	inline static void WaitForHandle(TaskWaitingHandle* handle)
	{
		if ((--handle->counter) != 0)
		{
			auto fiber = FiberPool::Take();
			Thread::SwitchToFiber(fiber, false);
		}
	}

	inline static void Submit(TaskWaitingHandle* handle, const Task& task, Task::PRIORITY priority = Task::PRIORITY::NORMAL)
	{
		task.m_handle = handle;
		task.m_handle->counter++;

		assert(task.m_handle->waitingFiber == Thread::GetCurrentFiber());

		s_queues[priority].enqueue(task);
		TryInvokeOneMoreWorker();
	}

	inline static void SubmitForThread(TaskWaitingHandle* handle, ID threadId, const Task& task)
	{
		task.m_handle = handle;
		task.m_handle->counter++;

		assert(task.m_handle->waitingFiber == Thread::GetCurrentFiber());

		s_threadQueues[threadId].enqueue(task);
		TryInvokeAllWorkers();
	}

public:
	inline static auto GetWorkerCount()
	{
		return s_workersCount;
	}

};

NAMESPACE_END