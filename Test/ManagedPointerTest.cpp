#include <gtest/gtest.h>

#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/Time/Clock.h"
#include "Core/Thread/Thread.h"
#include "Core/Random/Random.h"

#include "GTestLogger.h"
#include "MemoryLeakDetector.h"

#define MANAGED_PTR_TESTS

using namespace soft;

class Node : Traceable<Node>
{
public:
	Handle<Node> m_parent;
	Array<Handle<Node>> m_childs;

	size_t m_data[3] = { 0x23, 0xff, 0xc9 };

	bool m_isVisited = false;

public:
	~Node()
	{
		m_data[0] = 0;
		m_data[1] = 0;
		m_data[2] = 0;
	}

public:
	void AddChild(const Handle<Node>& node)
	{
		m_childs.Push(node);
	}

	void SetData(size_t* data)
	{
		m_data[0] = data[0];
		m_data[1] = data[1];
		m_data[2] = data[2];
	}

	bool IsDataValid(size_t* data)
	{
		return m_data[0] == data[0] && m_data[1] == data[1] && m_data[2] == data[2];
	}

	void SetVisited(bool value)
	{
		m_isVisited = value;
	}

	bool IsVisited()
	{
		return m_isVisited;
	}

	auto& Childs()
	{
		return m_childs;
	}

public:
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_parent);
		tracer->Trace(m_childs);
	}

};

//TEST(ManagedPointerTest, GarbageCollector)
//{
//#ifdef _DEBUG
//	constexpr size_t num = 1'000;
//	constexpr size_t numPerLog = 1'000;
//#else
//	//constexpr size_t num = 1'000'000'0;
//	//constexpr size_t numPerLog = 1'000'00;
//	constexpr size_t num = 1'000;
//	constexpr size_t numPerLog = 1'000;
//#endif // _DEBUG
//
//	constexpr size_t totalLog = num / numPerLog;
//
//	//MemoryLeakDetector memoryLeakDetector = { 64 };
//	{
//		Handle<Node> node = nullptr;
//		for (size_t i = 0; i < num; i++)
//		{
//			node = mheap::New<Node>();
//
//			if (i % numPerLog == 0)
//			{
//				GTestLogger::Stream(String::Format("Testing garbage collector {} / {} ...", i / numPerLog, totalLog));
//				Thread::Sleep(100);
//			}
//		}
//	}
//
//	gc::Run(-1);
//	//mheap::internal::Reset();
//}

void MemoryValidationImpl(size_t num = 1'000'000'0, size_t sleep1 = 200, size_t sleep2 = 200, size_t sleep3 = 1000)
{
	constexpr size_t total = 100;

#ifdef _DEBUG
	//size_t num = 1'000'0;
	size_t numPerLog = num / total;
	//constexpr size_t numPerLog2 = 1'00;
#else
	//size_t num = 1'000'0;
	size_t numPerLog = num / total;
	//constexpr size_t numPerLog2 = 5;
#endif // _DEBUG

	//MemoryLeakDetector memoryLeakDetector = { 64 };
	Handle<Node> root = nullptr;
	{
		size_t ids[] = { Random::RangeInt64(10, 20), Random::RangeInt64(21, 50), Random::RangeInt64(51, 100) };
		size_t changeRoot = Random::RangeInt64(1000, 10000);

		root = mheap::New<Node>();
		root->SetData(ids);

		// root is a node
		size_t totalNode = 1;

		for (size_t i = 0; i < num; i++)
		{
			Handle<Node> node = mheap::New<Node>();
			node->SetData(ids);

			if (i % changeRoot == 0)
			{
				node->AddChild(root);
				root = node;
				totalNode++;
			}
			else
			{
				for (size_t id : ids)
				{
					if (i % id == 0)
					{
						root->AddChild(node);
						totalNode++;
						break;
					}
				}
			}

			if (i % numPerLog == 0)
			{
				GTestLogger::Stream(String::Format("Preparing memory for validation {} % ...", (i / numPerLog)));
				Thread::Sleep(sleep1);
			}
		}

		GTestLogger::Log(String::Format("Total {} nodes", totalNode));
		Thread::Sleep(sleep3);

		size_t numPerLog2 = totalNode / total;

		Array<Handle<Node>> stack = {};
		stack.Push(root);

		size_t count = 0;

		while (stack.Size() != 0)
		{
			Handle<Node> it = stack.Pop();

			if (it->IsVisited()) continue;

			it->SetVisited(true);

			EXPECT_TRUE(it->IsDataValid(ids)) << changeRoot << " --- " << count;

			auto& childs = it->Childs();
			for (size_t i = 0; i < childs.Size(); i++)
			{
				auto& child = childs[i];
				if (!child->IsVisited())
				{
					stack.Push(child);
				}
			}

			count++;

			if (count % numPerLog2 == 0)
			{
				GTestLogger::Stream(String::Format("Validated memory of {} / {} nodes ...", count, totalNode));
				Thread::Sleep(sleep2);
			}
		}

		EXPECT_EQ(count, totalNode);
	}

	//gc::Run(-1);
	//mheap::internal::Reset();
}

//TEST(ManagedPointerTest, MemoryValidationOnGarbageCollection_SingleThread)
//{
//	MemoryValidationImpl();
//}

#ifdef _DEBUG
#define NUM_TEST_NODES 1'000'000
#else
#define NUM_TEST_NODES 1'000'000'0
#endif // _DEBUG

#ifdef MANAGED_PTR_TESTS
//TEST(ManagedPointerTest, MemoryValidationOnGarbageCollection_MultipleThreads)
//{
//	volatile bool isRunning = true;
//	std::thread thread1 = std::thread([&] {
//		soft::Thread::InitializeForThisThreadInThisModule();
//
//		auto heap = mheap::internal::Get();
//		while (isRunning)
//		{
//			if (heap->IsNeedGC())
//			{
//				gc::Run(-1);
//				heap->EndGC();
//			}
//			Thread::Sleep(32);
//		}
//
//		soft::Thread::FinalizeForThisThreadInThisModule();
//	});
//
//	std::thread thread2 = std::thread([&] {
//		soft::Thread::InitializeForThisThreadInThisModule();
//
//		auto heap = mheap::internal::Get();
//		while (isRunning)
//		{
//			if (heap->IsNeedGC())
//			{
//				gc::Run(-1);
//				heap->EndGC();
//			}
//			Thread::Sleep(32);
//		}
//
//		soft::Thread::FinalizeForThisThreadInThisModule();
//	});
//
//	auto start = Clock::ns::now();
//	MemoryValidationImpl(NUM_TEST_NODES);
//	GTestLogger::Log(String::Format("Taken time {} ms ...", (Clock::ns::now() - start) / 1000000));
//
//	isRunning = false;
//	thread1.join();
//	thread2.join();
//
//	gc::Run(-1);
//}
//
//TEST(ManagedPointerTest, MemoryValidationOnGarbageCollection_MultipleThreads_Cached)
//{
//	volatile bool isRunning = true;
//	std::thread thread1 = std::thread([&] {
//		soft::Thread::InitializeForThisThreadInThisModule();
//
//		auto heap = mheap::internal::Get();
//		while (isRunning)
//		{
//			if (heap->IsNeedGC())
//			{
//				gc::Run(-1);
//				heap->EndGC();
//			}
//			Thread::Sleep(32);
//		}
//
//		soft::Thread::FinalizeForThisThreadInThisModule();
//	});
//
//	std::thread thread2 = std::thread([&] {
//		soft::Thread::InitializeForThisThreadInThisModule();
//
//		auto heap = mheap::internal::Get();
//		while (isRunning)
//		{
//			if (heap->IsNeedGC())
//			{
//				gc::Run(-1);
//				heap->EndGC();
//			}
//			Thread::Sleep(32);
//		}
//
//		soft::Thread::FinalizeForThisThreadInThisModule();
//	});
//
//	auto start = Clock::ns::now();
//	MemoryValidationImpl(NUM_TEST_NODES);
//	GTestLogger::Log(String::Format("Taken time {} ms ...", (Clock::ns::now() - start) / 1000000));
//
//	isRunning = false;
//	thread1.join();
//	thread2.join();
//
//	gc::Run(-1);
//}

TEST(ManagedPointerTest, ManagedPointerCrossBoundary)
{
	volatile bool isRunning = true;
	std::thread thread1 = std::thread([&] {
		soft::Thread::InitializeForThisThreadInThisModule();

		auto heap = mheap::internal::Get();
		while (isRunning)
		{
			if (heap->IsNeedGC())
			{
				gc::Run(-1);
				heap->EndGC();
			}
			Thread::Sleep(32);
		}

		soft::Thread::FinalizeForThisThreadInThisModule();
	});

	auto start = Clock::ns::now();

	{
		size_t ids[] = { Random::RangeInt64(10, 20), Random::RangeInt64(21, 50), Random::RangeInt64(51, 100) };

		mheap::internal::SetStableValue(1);
		Array<Handle<Node>> objects = {};
		for (size_t i = 0; i < 100000; i++)
		{
			auto node = mheap::New<Node>();
			node->SetData(ids);
			objects.Push(node);
		}

		mheap::internal::SetStableValue(0);
		for (size_t i = 0; i < 1'000'000; i++)
		{
			if (Random::RangeInt32(0, 3) == 0)
			{
				auto id = Random::RangeInt64(0, objects.Size() - 1);
				Handle<Node> parent = objects[id];

				auto node = mheap::New<Node>();
				node->SetData(ids);

				parent->AddChild(node);
			}
		}

		for (size_t i = 0; i < objects.Size(); i++)
		{
			auto& node = objects[i];
			EXPECT_TRUE(node->IsDataValid(ids));

			auto& childs = node->Childs();
			for (size_t i = 0; i < childs.Size(); i++)
			{
				auto& child = childs[i];
				EXPECT_TRUE(child->IsDataValid(ids));
			}
		}
	}

	GTestLogger::Log(String::Format("Taken time {} ms ...", (Clock::ns::now() - start) / 1000000));

	isRunning = false;
	thread1.join();

	gc::Run(-1);
	mheap::internal::FreeStableObjects(1, 0, 0);
	gc::Run(-1);
}

#endif // MANAGED_PTR_TESTS