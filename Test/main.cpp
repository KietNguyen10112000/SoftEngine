#include <gtest/gtest.h>

#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/Thread/Thread.h"
#include "Core/Fiber/FiberPool.h"
#include "Core/Time/Clock.h"
#include "Core/Random/Random.h"
//#include "Core/Thread/ThreadID.h"

#include "TaskSystem/TaskWorker.h"

#include "MemoryLeakDetector.h"

#include "Core/Random/Random.h"

#include "Objects/QueryStructures/DBVTQueryTree.h"

using namespace soft;

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

	int ret = 0;
	{
		soft::MemoryInitialize();
		soft::Random::Initialize();
		soft::FiberPool::Initialize();
		soft::TaskWorker::Initalize();
		soft::Thread::InitializeForThisThreadInThisModule();

		//soft::ThreadID::InitializeForThisThread();
		//auto v = soft::ThreadID::Get();

		ret = RUN_ALL_TESTS();

		soft::Thread::FinalizeForThisThreadInThisModule();
		soft::TaskWorker::Finalize();
		soft::FiberPool::Finalize();
		soft::MemoryFinalize();
	}
	
	return ret;
}