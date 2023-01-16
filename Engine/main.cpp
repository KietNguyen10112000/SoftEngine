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
	soft::MemoryInitialize();
	soft::Random::Initialize();
	soft::FiberPool::Initialize();
	soft::Thread::InitializeForThisThreadInThisModule();
	soft::TaskWorker::Initalize();
	soft::SubSystems::Initialize();

	soft::Engine::Get()->Loop();
	
	soft::TaskWorker::Finalize();
	soft::Thread::FinalizeForThisThreadInThisModule();
	soft::FiberPool::Finalize();
	soft::MemoryFinalize();

	return 0;
}