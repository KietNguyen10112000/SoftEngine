#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/StackAllocator.h"
#include "Core/Structures/Raw/ConcurrentList.h"
#include "TaskSystem/TaskUtils.h"

//#include "MainComponent.h"

NAMESPACE_BEGIN

//template <typename _C, size_t>
//class AsyncServer2;

class MainComponent;

namespace raw 
{

template <typename _C>
class AsyncTaskRunner
{
public:
	//friend class AsyncServer2<_C, NUM_CMD>;
	friend typename _C;

	using Param = void*;
	using PramDtor = void (*)(void*);
	using Entry = void (*)(_C*, Param);
	
	struct AsyncTask
	{
		//private:
		//	friend class AsyncServer<_C, NUM_CMD>;
		size_t processed;
		Entry entry;
		Param param;
		PramDtor paramDtor;
	};

	StackAllocator m_stackAllocator = { 8 * MB };

	raw::ConcurrentArrayList<AsyncTask> m_tasks;

public:
	AsyncTaskRunner()
	{
		m_tasks.ReserveNoSafe(8 * KB);
	}

public:
	inline void ProcessAllTasks(_C* self)
	{
		for (auto& task : m_tasks)
		{
			task.entry(self, task.param);
			if (task.paramDtor)
			{
				task.paramDtor(task.param);
			}
		}
		m_tasks.Clear();
		m_stackAllocator.Clear();
	}

	inline void ProcessAllTasksMT(_C* self)
	{
		TaskUtils::ForEachConcurrentListAsRingBuffer(
			m_tasks,
			[&](AsyncTask& task, ID)
			{
				auto& processed_ = reinterpret_cast<std::atomic<size_t>&>(task.processed);
				auto id = processed_.exchange(INVALID_ID);
				if (id == INVALID_ID)
				{
					return false;
				}

				task.entry(self, task.param);

				if (task.paramDtor)
				{
					task.paramDtor(task.param);
				}

				return true;
			},
			TaskSystem::GetWorkerCount()
		);

		m_tasks.Clear();
		m_stackAllocator.Clear();
	}

public:
	inline AsyncTask CreateTask(Entry func)
	{
		AsyncTask task;
		task.processed = 0;
		task.entry = func;
		return task;
	}

	template <typename T, typename ...Args>
	inline T* CreateParam(AsyncTask* ofTask, Args&&... args)
	{
		T* ret = (T*)m_stackAllocator.Allocate(sizeof(T));
		new (ret) T(std::forward<Args>(args)...);

		ofTask->param = ret;

		if constexpr (!(std::is_pod_v<T>))
		{
			ofTask->paramDtor = [](void* ptr)
			{
				auto p = (T*)ptr;
				p->~T();
			};
		}
		else 
		{
			ofTask->paramDtor = nullptr;
		}

		return ret;
	}

	inline void CreateParamVoidPtr(AsyncTask* ofTask, void* p)
	{
		ofTask->param = p;
		ofTask->paramDtor = nullptr;
	}

	inline void RunAsync(AsyncTask* task)
	{
		m_tasks.Add(*task);
	}

};

template <typename _C>
class AsyncTaskRunnerForMainComponent
{
public:
	friend typename _C;

	using Param = void*;
	using PramDtor = void (*)(void*);
	using Entry = void (*)(_C*, Param);

	struct AsyncTask
	{
		Entry entry;
		Param param;
		PramDtor paramDtor;
	};

	using ListType = raw::ConcurrentArrayList<AsyncTask>;

	struct AsyncTaskList
	{
		size_t processed;
		ListType* list;
		MainComponent* component;
	};

	StackAllocator m_stackAllocator = { 8 * MB };

	raw::ConcurrentArrayList<AsyncTaskList> m_objectTasks;

	raw::ConcurrentArrayList<ListType*> m_cacheList;

public:
	AsyncTaskRunnerForMainComponent()
	{
		m_objectTasks.ReserveNoSafe(8 * KB);
	}

	~AsyncTaskRunnerForMainComponent()
	{
		for (auto& list : m_objectTasks)
		{
			delete list.list;
		}

		for (auto& list : m_cacheList)
		{
			delete list;
		}
	}

public:
	template <typename MainComponent_ = MainComponent>
	inline void ProcessAllTasks(_C* self)
	{
		m_cacheList.BeginTryTake();

		TaskUtils::ForEachConcurrentListAsRingBuffer(
			m_objectTasks,
			[&](AsyncTaskList& taskList, ID)
			{
				auto& processed_ = reinterpret_cast<std::atomic<size_t>&>(taskList.processed);
				auto id = processed_.exchange(INVALID_ID);
				if (id == INVALID_ID)
				{
					return false;
				}

				for (auto& task : *taskList.list)
				{
					task.entry(self, task.param);

					if (task.paramDtor)
					{
						task.paramDtor(task.param);
					}
				}

				taskList.list->Clear();

				m_cacheList.Add(taskList.list);

				using AtomicType = std::atomic<ListType*>;
				AtomicType& atom = reinterpret_cast<AtomicType&>(taskList.component->m_forAsyncTaskRunner);
				atom.store(nullptr, std::memory_order_relaxed);

				return true;
			},
			TaskSystem::GetWorkerCount()
		);

		m_objectTasks.Clear();
		m_stackAllocator.Clear();
	}

public:
	inline AsyncTask CreateTask(Entry func)
	{
		AsyncTask task;
		task.entry = func;
		return task;
	}

	template <typename T, typename ...Args>
	inline T* CreateParam(AsyncTask* ofTask, Args&&... args)
	{
		T* ret = (T*)m_stackAllocator.Allocate(sizeof(T));
		new (ret) T(std::forward<Args>(args)...);

		ofTask->param = ret;

		if constexpr (!(std::is_pod_v<T>))
		{
			ofTask->paramDtor = [](void* ptr)
				{
					auto p = (T*)ptr;
					p->~T();
				};
		}
		else
		{
			ofTask->paramDtor = nullptr;
		}

		return ret;
	}

	inline void CreateParamVoidPtr(AsyncTask* ofTask, void* p)
	{
		ofTask->param = p;
		ofTask->paramDtor = nullptr;
	}

	// type of <component> must be derived from MainComponent
	template <typename MainComponent_>
	inline void RunAsync(MainComponent_* component, AsyncTask* task)
	{
		using AtomicType = std::atomic<ListType*>;
		AtomicType& atom = reinterpret_cast<AtomicType&>(component->m_forAsyncTaskRunner);

		ListType* list = atom.load(std::memory_order_relaxed);
		if (list != 0 && list != (ListType*)INVALID_ID)
		{
			list->Add(*task);
			return;
		}

		if (list == (ListType*)INVALID_ID)
		{
			while ((list = atom.load(std::memory_order_relaxed)) == (ListType*)INVALID_ID)
			{
				std::this_thread::yield();
			}

			list->Add(*task);
			return;
		}

		if (list == 0)
		{
			if (atom.exchange((ListType*)INVALID_ID) == 0)
			{
				auto pList = m_cacheList.TryTake();
				if (pList)
				{
					list = *pList;
				}
				else
				{
					list = new ListType();
				}

				AsyncTaskList taskList;
				taskList.processed = 0;
				taskList.component = component;
				taskList.list = list;
				m_objectTasks.Add(taskList);

				atom.store(list, std::memory_order_relaxed);

				list->Add(*task);
				return;
			}
			else
			{
				while ((list = atom.load(std::memory_order_relaxed)) == (ListType*)INVALID_ID)
				{
					std::this_thread::yield();
				}

				list->Add(*task);
				return;
			}
		}
	}
};

//template <typename _C, size_t NUM_CMD>
//class AsyncServer2
//{
//public:
//	constexpr static size_t NUM_SERVERS = 2;
//
//	friend typename _C;
//
//	using Server = AsyncServer<_C, NUM_CMD>;
//	using Command = typename Server::Command;
//	using Entry = typename Server::Entry;
//	using CmdProcessor = typename Server::CmdProcessor;
//
//	Server m_servers[NUM_SERVERS] = {};
//	size_t m_curServerId = 0;
//
//protected:
//	inline Server& GetCurrentServer()
//	{
//		return m_servers[m_curServerId];
//	}
//
//	inline Server& GetPrevServer()
//	{
//		return m_servers[(m_curServerId + NUM_SERVERS - 1) % NUM_SERVERS];
//	}
//
//	inline void UpdateCurrentServer()
//	{
//		m_curServerId = (m_curServerId + 1) % NUM_SERVERS;
//	}
//
//	inline void SetCmdProcessor(ID cmdID, CmdProcessor cmdProcessor)
//	{
//		for (auto& server : m_servers)
//		{
//			server.SetCmdProcessor(cmdID, cmdProcessor);
//		}
//	}
//
//	/*inline void ProcessAllCmds(Server& server, _C* self)
//	{
//		server.ProcessAllCmds(self);
//	}
//
//	inline void ProcessAllCmdsMT(Server& server, _C* self)
//	{
//		server.ProcessAllCmdsMT(self);
//	}*/
//
//public:
//	// thread-safe methods
//	inline Command CreateCommand(ID cmdID, Entry func)
//	{
//		return GetCurrentServer().CreateCommand(cmdID, func);
//	}
//
//	// thread-safe methods
//	template <typename T, typename ...Args>
//	inline T* CreateParam(Command* cmd, Args&&... args)
//	{
//		return GetCurrentServer().CreateParam(cmd, args...);
//	}
//
//	// thread-safe methods
//	inline void Run(Command* cmd)
//	{
//		GetCurrentServer().Run(cmd);
//	}
//
//};


}

NAMESPACE_END