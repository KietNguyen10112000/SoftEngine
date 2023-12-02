#include <iostream>
#include <string>

#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/Thread/Thread.h"
#include "Core/Fiber/FiberPool.h"
#include "Core/Random/Random.h"

//#include "Core/Memory/Page.h"

#include "TaskSystem/TaskWorker.h"

#include "Runtime/Runtime.h"

#include "StartupConfig.h"

#include "Platform/Platform.h"

//#include "windows.h"
//#define _CRTDBG_MAP_ALLOC //to get more details
//#include <stdlib.h>  
//#include <crtdbg.h>   //for malloc and free

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

	StartupConfig::Get().executablePath = platform::GetExecutablePathCStr();

	size_t id = -1;
	for (int i = 1; i < argc; i++)
	{
		std::string_view str = argv[i];

		if ((id = str.find("--NReservedThread=")) != std::string_view::npos)
		{
			StartupConfig::Get().reservedThread = std::stoi(str.substr(id + 18).data());
			continue;
		}

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

	//_CrtMemState sOld;
	//_CrtMemState sNew;
	//_CrtMemState sDiff;
	//_CrtMemCheckpoint(&sOld); //take a snapshot

	ManagedLocalScope::ClearStack();

	MemoryInitialize();
	Random::Initialize();
	FiberPool::Initialize();
	Thread::InitializeForThisThreadInThisModule();

	ProcessArgs(argc, argv);

	//struct AllocInfo
	//{
	//	void* ptr;
	//	size_t size;
	//};

	//const size_t size = 3 * 1024 * MB;

	//Page page(size);

	//size_t allocSize = 0;

	//std::Vector<AllocInfo> bufs;

	//std::cout << "begin!!!\n";
	//for (size_t i = 0; i < 10'000'000; i++)
	//{
	//	auto bSize = Random::RangeInt64(512, 64 * KB * KB);
	//	auto ptr = page.Allocate(bSize);//std::malloc(bSize);//page.Allocate(bSize);

	//	while (allocSize >= size / 2)
	//	{
	//		auto _v = Random::RangeInt64(0, bufs.size());
	//		for (size_t j = 0; j < _v; j++)
	//		{
	//			allocSize -= bufs[j].size;
	//			//std::free(bufs[j].ptr);
	//			page.Free(bufs[j].ptr);

	//			if (bufs.size() != 0)
	//			{
	//				bufs[j] = bufs.back();
	//				bufs.pop_back();
	//				j--;
	//				_v--;

	//				if (bufs.size() == 0) break;
	//			}
	//			else
	//			{
	//				bufs.clear();
	//				break;
	//			}
	//		}
	//	}

	//	allocSize += bSize;
	//	bufs.push_back({ ptr, (size_t)bSize });
	//}

	//std::cout << "Done!!!\n";

	//return 0;

	TaskWorker::Initalize(StartupConfig::Get().maxThreads, StartupConfig::Get().reservedThread);

	auto sys = soft::gc::g_system;

	{
		auto engine = Runtime::Initialize();

		auto currentThreadId = Thread::GetID();

		auto task = Task([](void* e) { ((Runtime*)e)->Setup(); ((Runtime*)e)->Run(); }, engine.Get());
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

	//_CrtMemCheckpoint(&sNew); //take a snapshot 
	//if (_CrtMemDifference(&sDiff, &sOld, &sNew)) // if there is a difference
	//{
	//	OutputDebugString(L"-----------_CrtMemDumpStatistics ---------");
	//	_CrtMemDumpStatistics(&sDiff);
	//	OutputDebugString(L"-----------_CrtMemDumpAllObjectsSince ---------");
	//	_CrtMemDumpAllObjectsSince(&sOld);
	//	OutputDebugString(L"-----------_CrtDumpMemoryLeaks ---------");
	//	_CrtDumpMemoryLeaks();
	//}

	return 0;
}