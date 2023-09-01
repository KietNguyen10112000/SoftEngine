#include <iostream>
#include <string>

#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/Thread/Thread.h"
#include "Core/Fiber/FiberPool.h"
#include "Core/Random/Random.h"

#include "TaskSystem/TaskWorker.h"

#include "Runtime/Runtime.h"

#include "StartupConfig.h"

namespace soft
{
	namespace gc
	{
		class System;
		extern System* g_system;
	}
}


//
// example:
// --NThread=4 --RcPath=<path> --PlgPath=<path>
// 
inline void ProcessArgs(int argc, const char** argv)
{
	using namespace soft;

	size_t id = -1;
	for (int i = 1; i < argc; i++)
	{
		std::string_view str = argv[i];

		if ((id = str.find("--NThread=")) != std::string_view::npos)
		{
			StartupConfig::Get().numThreads = std::stoi(str.substr(id + 10).data());
			continue;
		}

		if ((id = str.find("--RcPath=")) != std::string_view::npos)
		{
			StartupConfig::Get().resourcesPath = str.substr(id + 9).data();
			continue;
		}

		if ((id = str.find("--PlgPath=")) != std::string_view::npos)
		{
			StartupConfig::Get().pluginsPath = str.substr(id + 10).data();
			continue;
		}

		if ((id = str.find("--NoRendering")) != std::string_view::npos)
		{
			StartupConfig::Get().isEnableRendering = false;
			continue;
		}
	}
}

int main(int argc, const char** argv)
{
	using namespace soft;

	ManagedLocalScope::ClearStack();

	ProcessArgs(argc, argv);

	MemoryInitialize();
	Random::Initialize();
	FiberPool::Initialize();
	Thread::InitializeForThisThreadInThisModule();
	TaskWorker::Initalize(StartupConfig::Get().maxThreads, StartupConfig::Get().reservedThread);

	auto sys = soft::gc::g_system;

	{
		auto engine = Runtime::Initialize();

		auto currentThreadId = Thread::GetID();

		auto task = Task([](void* e) { ((Runtime*)e)->Run(); }, engine.Get());
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
	

	Runtime::Finalize();
	
	TaskWorker::Finalize();
	Thread::FinalizeForThisThreadInThisModule();
	FiberPool::Finalize();
	MemoryFinalize();

	return 0;
}