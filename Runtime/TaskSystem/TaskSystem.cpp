#include "TaskSystem.h"
#include "TaskSystemInfo.h"

NAMESPACE_BEGIN

std::condition_variable TaskSystem::s_cv;
std::mutex TaskSystem::s_mutex;
std::atomic<size_t> TaskSystem::s_workingWorkersCount = { 1 };
size_t TaskSystem::s_workersCount;
TaskSystem::SynchContext TaskSystem::s_sychCtxs[SubSystemInfo::INDEXED_SUBSYSTEMS_COUNT] = {};
ConcurrentQueue<Fiber*> TaskSystem::s_resumeFibers;
ConcurrentQueue<Task> TaskSystem::s_queues[Task::PRIORITY::COUNT];
ConcurrentQueue<Task> TaskSystem::s_threadQueues[ThreadLimit::MAX_THREADS];

TaskSystem::SynchContext::SynchContext() : tasks(TaskSystemInfo::QUEUE_CAPACITY)
{
}

TaskSystem::SynchContext::~SynchContext()
{
}

void TaskSystem::Initialize()
{
	s_workersCount = TaskSystemInfo::WORKERS_COUNT;

	for (auto& q : s_queues)
	{
		new (&q) ConcurrentQueue<Task>(TaskSystemInfo::QUEUE_CAPACITY);
	}

	for (auto& q : s_threadQueues)
	{
		new (&q) ConcurrentQueue<Task>();
	}
}

NAMESPACE_END