#include "TaskWorker.h"

NAMESPACE_BEGIN

TaskWorker TaskWorker::s_workers[FiberInfo::TOTAL_FIBERS] = {};

void TaskWorker::Initalize()
{
	
}

void TaskWorker::Finalize()
{
	for (auto& w : s_workers)
	{
		w.IsRunning() = false;
	}
}

NAMESPACE_END