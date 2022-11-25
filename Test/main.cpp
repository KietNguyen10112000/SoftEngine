#include <gtest/gtest.h>

#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/Thread/Thread.h"
#include "Core/Fiber/FiberPool.h"

#include "TaskSystem/TaskWorker.h"

#include "Core/Time/Clock.h"

using namespace soft;

TEST(MemoryTest, RawHeapAllocation)
{
	rheap::free(rheap::malloc(64));
}

TEST(MemoryTest, RawHeapAllocationCompareSystemMalloc)
{
	auto start = Clock::ns::now();

	// let it cache first
	{
		std::vector<void*> arr = {};
		for (size_t i = 0; i < 1000; i++)
		{
			arr.push_back(rheap::malloc(i + 64));
		}

		for (auto& v : arr)
		{
			rheap::free(v);
		}
	}

	// try again
	start = Clock::ns::now();
	{
		std::vector<void*> arr = {};
		for (size_t i = 0; i < 1000; i++)
		{
			arr.push_back(rheap::malloc(i + 64));
		}

		for (auto& v : arr)
		{
			rheap::free(v);
		}
	}
	auto rheapAllocTime = Clock::ns::now() - start;


	start = Clock::ns::now();
	{
		std::vector<void*> arr = {};
		for (size_t i = 0; i < 1000; i++)
		{
			arr.push_back(::malloc(i + 64));
		}

		for (auto& v : arr)
		{
			::free(v);
		}
	}
	auto mallocAllocTime = Clock::ns::now() - start;

	EXPECT_LE(rheapAllocTime, mallocAllocTime);
	std::cout << "\033[0;32m" << "[          ] " << "RawHeapAlloc (ns) / SysMalloc (ns) = "
		<< rheapAllocTime / (float)mallocAllocTime << "\033[0;0m" "\n";
}

int main(int argc, char** argv)
{
	soft::MemoryInitialize();
	soft::FiberPool::Initialize();
	soft::TaskWorker::Initalize();
	soft::Thread::InitializeForThisThread();

    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();

	soft::Thread::FinalizeForThisThread();
	soft::TaskWorker::Finalize();
	soft::FiberPool::Finalize();
	soft::MemoryFinalize();

	return ret;
}