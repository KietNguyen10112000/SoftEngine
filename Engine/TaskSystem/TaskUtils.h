#pragma once

#include "TaskSystem.h"

#include "Core/Structures/Raw/ConcurrentList.h"

#include "TaskParamUnpack.h"

NAMESPACE_BEGIN

namespace TaskUtils
{

template <class CList, typename Fn, size_t MAX_THREADS = 128, size_t EFFECTIVE_MT_SIZE = 1024>
inline void ForEachConcurrentList(CList& list, Fn callback,
	const size_t N_THREADS, Task::PRIORITY priority = Task::CRITICAL)
{
	using Iteration = CList::RingIteration;
	struct Param
	{
		Fn*						cb;
		Iteration				it;

		std::atomic<intmax_t>*	count;
		ID						dispatchId;
	};

	intmax_t size = list.size();

	if (size < N_THREADS * EFFECTIVE_MT_SIZE)
	{
		list.ForEach([&](auto elm) { callback(elm, 0); });
		return;
	}

	/*if (size == 0)
	{
		return;
	}*/


	Task	tasks	[MAX_THREADS];
	Param	params	[MAX_THREADS];

	std::atomic<intmax_t> count = { size };

	auto it = list.RingBegin();
	auto numPerThread = size / N_THREADS;

	for (size_t i = 0; i < N_THREADS; i++)
	{
		auto& param = params[i];
		auto& task = tasks[i];

		param.cb = &callback;
		param.it = it;
		param.count = &count;
		param.dispatchId = i;

		task.Params() = &param;
		task.Entry() = [](void* p) 
		{
			TASK_SYSTEM_UNPACK_PARAM_4(Param, p, cb, it, count, dispatchId);
			
			auto& remain = *count;

			auto begin = it;
			auto& call = *cb;
			do 
			{
				auto& elm = *it;

				if (call(elm, dispatchId))
				{
					--remain;
				}

				++it;
			} while (remain.load(std::memory_order_relaxed) > 0 && begin != it);
		};

		it += numPerThread;
	}

	TaskSystem::SubmitAndWait(tasks, N_THREADS, priority);
}

}

NAMESPACE_END