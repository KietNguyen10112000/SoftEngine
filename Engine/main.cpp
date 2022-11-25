#include <iostream>
#include <string>

#include "Core/Memory/Memory.h"
#include "Core/Structures/Structures.h"
#include "Core/Thread/Thread.h"
#include "Core/Fiber/FiberPool.h"

#include "TaskSystem/TaskWorker.h"

using namespace soft;

class Node : Traceable<Node>
{
public:
	Handle<Node> m_parent;
	Array<Handle<Node>> m_childs;

	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_parent);
		tracer->Trace(m_childs);
	}

};

namespace soft
{
	extern ManagedHeap* g_rawHeap;
}

void ExampleOfUsingManagedMemory()
{
	Handle<Node> root;

	{
		root = mheap::New<Node>();

		Handle<Node> node = mheap::New<Node>();
		node->m_parent = root;
		root->m_childs.Push(node);

		node = mheap::New<Node>();
		node->m_parent = root;
		root->m_childs.Push(node);

		auto field = node.Field<&Node::m_childs>();

		String str = String::Format("Tuple[{}, {}, {}, {}]", 1, 3.14, true, "Hello, World!");
		std::cout << str << "\n";

		str = String::Format("Tuple[{}, {}, {}]", 1, 3.14, true);
		std::cout << str << "\n";

		std::string str2 = "+++++++++++++++++++++++++++";
	}

	root = nullptr;
	gc::Run(-1);
}

size_t counter[2] = {};

void SpawnTasks()
{
	size_t currentThreadId = Thread::GetID();
	for (size_t i = 0; i < 50; i++)
	{
		TaskSystem::Submit(
			{
				[](void* param)
				{
					auto submitedThreadId = (size_t)param;
					std::cout << String::Format("Task of Thread[{}] is running on Thread[{}]\n",
						submitedThreadId, Thread::GetID());
					_sleep(32);
				},
				(void*)currentThreadId
			}
		);
	}
}

void SubmitAndWaitATask()
{
	size_t currentThreadId = Thread::GetID();
	TaskSystem::SubmitAndWait(
		{
			[](void* param)
			{
				//ExampleOfUsingManagedMemory();
				std::cout << String::Format("Running waiting task: Thread[{}] --- Fiber[{}]\n", Thread::GetID(), Thread::GetCurrentFiberID());
				_sleep(10);
			},
			(void*)currentThreadId
		}
	);
}

int main()
{
	soft::MemoryInitialize();
	soft::FiberPool::Initialize();
	soft::TaskWorker::Initalize();
	soft::Thread::InitializeForThisThread();

	std::cout << String::Format("InitializeForThisThread => Thread::GetID() = {} --- NativeThreadHandle = {}\n", Thread::GetID(), std::this_thread::get_id());

	{
		SpawnTasks();

		// assume that thread2 is client thread
		std::thread thread2 = std::thread([]()
			{
				// context of thread 2
				soft::Thread::InitializeForThisThread();

				std::cout << String::Format("InitializeForThisThread => Thread::GetID() = {} --- NativeThreadHandle = {}\n", Thread::GetID(), std::this_thread::get_id());


				size_t currentThreadId = Thread::GetID();

				SpawnTasks();

				// after call SubmitAndWait, threadID can be changed
				// because, this thread is currently running a fiber,
				// when call SubmitAndWait, the fiber will be pushed on waiting-list 
				// so when fiber is in waiting-list, the fiber can be run by other threads 
				SubmitAndWaitATask();
				//SubmitAndWaitATask();
				//SubmitAndWaitATask();
				

				//std::cout << String::Format("Thread::GetID() = {} --- NativeThreadHandle = {}\n", Thread::GetID(), std::this_thread::get_id());
				//std::cout << String::Format("Thread::GetID() = {} --- currentThreadId = {}\n", Thread::GetID(), currentThreadId);

				// call Thread::GetID() here can return a value differ from currentThreadId (above)
				if (Thread::GetID() != currentThreadId)
				{
					// self submit
					// all tasks submit for a thread will run in order
					TaskSystem::SubmitForThread(
						{
							[](void* param)
							{
								// submit a task that attach primary fiber to its thread
								TaskSystem::SubmitForThread(
									{
										[](void*)
										{
											std::cout << String::Format("Thread[{}] --- Fiber[{}]\n", Thread::GetID(), Thread::GetCurrentFiberID());

											// context of currentThreadId (at line 127), but fiberID can be any value
											// so switch to primary fiber of this thread
											Thread::SwitchToFiber(FiberPool::Get(Thread::GetID()), true);
										},
										0
									},
									(size_t)param
								);
							},
							(void*)currentThreadId
						},
						Thread::GetID()
					);

					// after switch fiber the above task will be execute
					Thread::SwitchToFiber(FiberPool::Take(), true);
				}

				_sleep(16);

				// stop thread 0
				TaskSystem::SubmitForThread(
					{
						[](void*)
						{
							Thread::SwitchToFiber(FiberPool::Get(Thread::GetID()), true);
						},
						0
					},
					0
				);

				TaskSystem::SubmitForThread(
					{
						[](void*)
						{
							TaskWorker::Get()->IsRunning() = false;
						},
						0
					},
					0
				);

				std::cout << String::Format("End Thread[{}] --- Fiber[{}]\n", Thread::GetID(), Thread::GetCurrentFiberID());

				soft::Thread::FinalizeForThisThread();
			}
		);

		//std::this_thread::yield();

		ExampleOfUsingManagedMemory();

		TaskWorker::Get()->Main();

		thread2.join();

		//std::cout << "==========================END====================\n";

		//_sleep(10000);
	}
	
	soft::Thread::FinalizeForThisThread();
	soft::TaskWorker::Finalize();
	soft::FiberPool::Finalize();
	soft::MemoryFinalize();

	return 0;
}