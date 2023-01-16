#include "TaskWorker.h"

NAMESPACE_BEGIN

TaskWorker TaskWorker::s_workers[FiberInfo::TOTAL_FIBERS] = {};
size_t TaskWorker::s_totalInitializedWorker = 0;

void TaskWorker::Initalize(size_t maxWorker)
{
	auto numThreads = std::thread::hardware_concurrency();
	numThreads = std::min((uint32_t)maxWorker, numThreads);
	numThreads = std::min((uint32_t)FiberInfo::TOTAL_FIBERS, numThreads);

	s_totalInitializedWorker = numThreads;
	TaskSystem::s_workersCount = s_totalInitializedWorker;

	// init all avaiable threads
	for (uint32_t i = 1; i < numThreads; i++)
	{
		auto& worker = s_workers[i];
		worker.m_bindedThread = std::thread([i]()
			{
				Thread::InitializeForThisThreadInThisModule();
				auto& w = s_workers[i];
				w.IsRunning() = true;
				w.m_bindedThreadId = ThreadID::Get();
				w.Main();
				Thread::FinalizeForThisThreadInThisModule();
			}
		);
	}
}

void TaskWorker::Finalize()
{
	for (uint32_t i = 1; i < s_totalInitializedWorker; i++)
	{
		auto& w = s_workers[i];
		while (w.m_bindedThreadId == 0)
		{
			std::this_thread::yield();
		}

		while (w.IsRunning())
		{
			w.IsRunning() = false;
			std::this_thread::yield();
		}

		TaskSystem::SubmitForThread({ [](void*) {} ,0 }, w.m_bindedThreadId);
		w.m_bindedThread.join();
	}
}

NAMESPACE_END