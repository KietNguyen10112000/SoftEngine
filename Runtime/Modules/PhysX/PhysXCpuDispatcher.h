#pragma once

#include "TaskSystem/TaskSystem.h"

#include "PxPhysicsAPI.h"

NAMESPACE_BEGIN

class PhysXCpuDispatcher : public physx::PxCpuDispatcher
{
public:
	inline virtual void submitTask(physx::PxBaseTask& task) override
	{
		Task myTask;
		myTask.Params() = &task;
		myTask.Entry() = [](void* p)
		{
			auto pxTask = ((physx::PxBaseTask*)p);
			pxTask->run();
			pxTask->release();
		};

		TaskSystem::Submit(myTask, Task::CRITICAL);
	};

	inline virtual uint32_t getWorkerCount() const override
	{
		return TaskSystem::GetWorkerCount() / 2;
	};

};

NAMESPACE_END