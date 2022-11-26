#include <gtest/gtest.h>

#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/Time/Clock.h"
#include "Core/Thread/Thread.h"
#include "Core/Random/Random.h"

#include "GTestLogger.h"
#include "MemoryLeakDetector.h"

using namespace soft;

class Node : Traceable<Node>
{
public:
	Handle<Node> m_parent;
	Array<Handle<Node>> m_childs;

	size_t m_data[3] = { 0x23, 0xff, 0xc9 };

	bool m_isVisited = false;

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

TEST(ManagedPointerTest, GarbageCollector)
{
#ifdef _DEBUG
	constexpr size_t num = 1'000;
	constexpr size_t numPerLog = 1'000;
#else
	//constexpr size_t num = 1'000'000'0;
	//constexpr size_t numPerLog = 1'000'00;
	constexpr size_t num = 1'000;
	constexpr size_t numPerLog = 1'000;
#endif // _DEBUG

	constexpr size_t totalLog = num / numPerLog;

	//MemoryLeakDetector memoryLeakDetector = { 64 };
	{
		Handle<Node> node = nullptr;
		for (size_t i = 0; i < num; i++)
		{
			node = mheap::New<Node>();

			if (i % numPerLog == 0)
			{
				GTestLogger::Stream(String::Format("Testing garbage collector {} / {} ...", i / numPerLog, totalLog));
				Thread::Sleep(100);
			}
		}
	}

	gc::Run(-1);
	//mheap::internal::Reset();
}

TEST(ManagedPointerTest, MemoryValidation)
{
	constexpr size_t total = 100;

#ifdef _DEBUG
	constexpr size_t num = 1'000'0;
	constexpr size_t numPerLog = num / total;
	//constexpr size_t numPerLog2 = 1'00;
#else
	constexpr size_t num = 1'000'000'0;
	constexpr size_t numPerLog = num / total;
	//constexpr size_t numPerLog2 = 5;
#endif // _DEBUG

	//MemoryLeakDetector memoryLeakDetector = { 64 };
	{
		size_t ids[] = { Random::RangeInt64(1, 20), Random::RangeInt64(21, 50), Random::RangeInt64(51, 100) };
		size_t changeRoot = Random::RangeInt64(1000, 10000);

		Handle<Node> root = mheap::New<Node>();
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
				Thread::Sleep(200);
			}
		}

		GTestLogger::Log(String::Format("Total {} nodes", totalNode));
		Thread::Sleep(1000);

		size_t numPerLog2 = totalNode / total;

		Array<Handle<Node>> stack = {};
		stack.Push(root);

		size_t count = 0;

		while (stack.Size() != 0)
		{
			Handle<Node> it = stack.Pop();

			if (it->IsVisited()) continue;

			it->SetVisited(true);

			EXPECT_TRUE(it->IsDataValid(ids));

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
				Thread::Sleep(200);
			}
		}

		EXPECT_EQ(count, totalNode);
	}

	gc::Run(-1);
	//mheap::internal::Reset();
}