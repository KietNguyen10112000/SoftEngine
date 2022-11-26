#include <gtest/gtest.h>

#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/Time/Clock.h"

#include "GTestLogger.h"

using namespace soft;

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