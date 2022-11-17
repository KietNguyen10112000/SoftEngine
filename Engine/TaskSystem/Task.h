#pragma once

#include "Core/TypeDef.h"

#include <atomic>

#include "Core/Fiber/Fiber.h"
#include "Core/Thread/Thread.h"

NAMESPACE_BEGIN

using TaskParams = void*;
using TaskEntryPoint = void (*)(TaskParams);

struct TaskWaitingHandle
{
	std::atomic<size_t> counter = { 0 };
	Fiber* waitingFiber = 0;
};

class Task
{
protected:
	friend class TaskSystem;
	friend class TaskWorker;

	TaskEntryPoint m_main = 0;
	TaskParams m_params = 0;
	mutable TaskWaitingHandle* m_handle = 0;
	size_t padding = 0;

public:
	enum PRIORITY
	{
		CRITICAL,				// io tasks (read and write file)
		HIGH,					// task belong to subsystems (eg: process input, physics, animation, rendering, ...)
		NORMAL,					// task just submits other tasks (doesn't wait)
		LOW,					// task submits and waits for other tasks
		COUNT
	};

public:
	Task() = default;
	Task(TaskEntryPoint main, TaskParams params) 
		: m_main(main), m_params(params) {}

};

NAMESPACE_END