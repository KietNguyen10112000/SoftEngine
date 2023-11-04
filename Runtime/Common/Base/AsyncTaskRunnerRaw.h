#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/StackAllocator.h"
#include "Core/Structures/Raw/ConcurrentList.h"
#include "TaskSystem/TaskUtils.h"

NAMESPACE_BEGIN

//template <typename _C, size_t>
//class AsyncServer2;

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