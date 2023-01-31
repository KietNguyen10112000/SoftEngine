#include <iostream>
#include <string>

#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/Thread/Thread.h"
#include "Core/Fiber/FiberPool.h"
#include "Core/Random/Random.h"

#include "TaskSystem/TaskWorker.h"
#include "SubSystems/SubSystems.h"

#include "Engine/Engine.h"

int main()
{
	using namespace soft;

	MemoryInitialize();
	Random::Initialize();
	FiberPool::Initialize();
	Thread::InitializeForThisThreadInThisModule();
	TaskWorker::Initalize();

	SubSystems::Initialize();

	{
		auto engine = Engine::Initialize();

		auto currentThreadId = Thread::GetID();

		auto task = Task([](void* e) { ((Engine*)e)->Run(); }, engine.Get());
		TaskSystem::SubmitForThread(task, currentThreadId);

		//task = Task([](void*) { TaskWorker::Get()->IsRunning() = false; }, nullptr);
		//TaskSystem::SubmitForThread(task, currentThreadId);

		TaskWorker::Get()->Main();

		if (Thread::GetID() != currentThreadId)
		{
			TaskSystem::SubmitForThread(
				{
					[](void* arg)
					{
						Thread::SwitchToFiber(FiberPool::Get(Thread::GetID()), true);
					},
					(void*)0
				},
				currentThreadId
			);
			Thread::SwitchToFiber(FiberPool::Take(), true);
		}
	}
	

	Engine::Finalize();
	SubSystems::Finalize();
	
	TaskWorker::Finalize();
	Thread::FinalizeForThisThreadInThisModule();
	FiberPool::Finalize();
	MemoryFinalize();

	return 0;
}