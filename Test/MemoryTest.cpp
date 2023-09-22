#include <gtest/gtest.h>

#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/Time/Clock.h"

#include "Core/Memory/MemoryKeeper.h"

#include "GTestLogger.h"

using namespace soft;

#ifdef MEMORY_TEST

TEST(MemoryTest, MemoryKeeper)
{
	void* buffer = std::malloc(64 * MB);

	MemoryKeeper memKeeper;
	memKeeper.Reset(buffer, 64 * MB);

	size_t arrSizes[6] = {
		Random::RangeInt64(64, 100 * KB),
		Random::RangeInt64(64, 100 * KB),
		Random::RangeInt64(64, 100 * KB),
		Random::RangeInt64(64, 100 * KB),
		Random::RangeInt64(64, 100 * KB),
		Random::RangeInt64(64, 100 * KB)
	};

	std::vector<MemoryKeeper::Block*, STDAllocatorMalloc<MemoryKeeper::Block*>> blocks;
	for (size_t i = 0; i < Random::RangeInt64(500, 1000); i++)
	{
		if (Random::RangeInt64(0, 5) == 0)
		{
			if (!blocks.empty())
			{
				auto idx = Random::RangeInt64(0, blocks.size() - 1);
				if (blocks[idx])
					memKeeper.Deallocate(blocks[idx]);
				blocks[idx] = 0;
			}
		}

		blocks.push_back(memKeeper.Allocate(Random::RangeInt64(64, 100 * KB)));

		if (Random::RangeInt64(0, 3) == 0)
		{
			blocks.push_back(memKeeper.Allocate(arrSizes[Random::RangeInt64(0, 5)]));
		}

		if (Random::RangeInt64(0, 5) == 0)
		{
			assert(memKeeper.Allocate(100 * MB) == 0);
		}
	}

	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(blocks.begin(), blocks.end(), g);

	for (size_t i = 0; i < blocks.size(); i++)
	{
		if (blocks[i])
			memKeeper.Deallocate(blocks[i]);

		if (Random::RangeInt64(0, 5) == 0)
		{
			assert(memKeeper.Allocate(100 * MB) == 0);
		}
	}

	std::free(buffer);
}

TEST(MemoryTest, RawHeapAllocation)
{
	rheap::free(rheap::malloc(64));
}


TEST(MemoryTest, RawHeapAllocationCompareSystemMallocNoCached)
{
	auto start = Clock::ns::now();
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

	EXPECT_GE(rheapAllocTime, mallocAllocTime);
}


TEST(MemoryTest, RawHeapAllocationCompareSystemMallocCached)
{
	auto start = Clock::ns::now();
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
}


TEST(MemoryTest, RawHeapLargeObjectAllocation)
{
	int sizes[] = { 1 * MB, 5 * MB, 10 * MB, 20 * MB, 50 * MB/*, 120 * MB, 500 * MB*/ };
	const auto arrSize = sizeof(sizes) / sizeof(int);

	std::vector<void*> mems = {};
	for (size_t i = 0; i < arrSize; i++)
	{
		auto mem = rheap::malloc(sizes[i]);
		//GTestLogger::Log(String::Format("[{}] -> {} bytes", mem, sizes[i]));
		mems.push_back(mem);
	}

	for (size_t i = 0; i < arrSize; i++)
	{
		rheap::free(mems[i]);
	}
}

#endif // MEMORY_TEST