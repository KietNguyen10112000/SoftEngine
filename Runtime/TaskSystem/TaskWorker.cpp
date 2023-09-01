#include "TaskWorker.h"
#include "Runtime/StartupConfig.h"

NAMESPACE_BEGIN

TaskWorker TaskWorker::s_workers[FiberInfo::TOTAL_FIBERS] = {};
size_t TaskWorker::s_totalInitializedWorker = 0;

void TaskWorker::Initalize(size_t maxWorker, size_t reservedThread)
{
	auto numThreads = std::thread::hardware_concurrency();
	numThreads = std::min((uint32_t)maxWorker, numThreads);
	numThreads = std::min((uint32_t)FiberInfo::TOTAL_FIBERS, numThreads);

	if (StartupConfig::Get().numThreads != -1)
	{
		numThreads = StartupConfig::Get().numThreads;
	}

	assert(numThreads > reservedThread);
	numThreads -= reservedThread;

	s_totalInitializedWorker = numThreads;
	TaskSystem::s_workersCount = s_totalInitializedWorker;
	TaskSystem::s_workingWorkersCount = numThreads;

	static TaskWorker* initedWorker = 0;
	// init all avaiable threads
	for (uint32_t i = 1; i < numThreads; i++)
	{
		initedWorker = 0;
		auto thread = std::thread([]()
			{
				Thread::InitializeForThisThreadInThisModule();
				auto w = TaskWorker::Get();
				w->IsRunning() = true;
				w->m_bindedThreadId = ThreadID::Get();
				initedWorker = w;
				w->Main();
				Thread::FinalizeForThisThreadInThisModule();
			}
		);

		while (initedWorker == 0)
		{
			Thread::Sleep(16);
		}

		((TaskWorker*)initedWorker)->m_bindedThread.swap(thread);
	}
}

void TaskWorker::Finalize()
{
	static std::atomic<size_t> finalized = 0;

	for (uint32_t i = 1; i < s_totalInitializedWorker; i++)
	{
		TaskSystem::SubmitForThread(
			{
				[](void* arg)
				{
					Thread::SwitchToFiber(FiberPool::Get(Thread::GetID()), true);
				},
				(void*)0
			},
			i
		);
	}

	while (finalized.load() != s_totalInitializedWorker - 1)
	{
		for (uint32_t i = 1; i < s_totalInitializedWorker; i++)
		{
			TaskSystem::SubmitForThread(
				{
					[](void* arg)
					{
						TaskWorker::Get()->IsRunning() = false;
						finalized++;
					},
					(void*)0
				},
				i
			);
		}
	}
	

	for (auto& w : s_workers)
	{
		if (w.m_bindedThread.joinable()) w.m_bindedThread.join();
	}
}

NAMESPACE_END