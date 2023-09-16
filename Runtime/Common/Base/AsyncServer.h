#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/StackAllocator.h"
#include "Core/Structures/Raw/ConcurrentList.h"

NAMESPACE_BEGIN

//template <typename _C, size_t>
//class AsyncServer2;

template <typename _C, size_t NUM_CMD>
class AsyncServer
{
public:
	//friend class AsyncServer2<_C, NUM_CMD>;

	using Param = void*;
	using PramDtor = void (*)(void*);
	using Entry = void (*)(_C*, Param*);
	
	struct Command
	{
	//private:
	//	friend class AsyncServer<_C, NUM_CMD>;

		ID cmdID;
		Entry entry;
		Param param;
		PramDtor paramDtor;
	};

	using CmdProcessor = void (*)(_C*, Command*);

	StackAllocator m_stackAllocator = { 8 * MB };

	raw::ConcurrentArrayList<Command, STDAllocatorMalloc<Command>> m_cmds;
	CmdProcessor m_processors[NUM_CMD] = {};

public:
	AsyncServer()
	{
		m_cmds.ReserveNoSafe(8 * KB);
	}

public:
	inline void SetCmdProcessor(ID cmdID, CmdProcessor cmdProcessor)
	{
		m_processors[cmdID] = cmdProcessor;
	}

	inline void ProcessAllCmds(_C* self)
	{
		for (auto& cmd : m_cmds)
		{
			m_processors[cmd.cmdID](self, &cmd);
			cmd.paramDtor(cmd.param);
		}
		m_cmds.Clear();
		m_stackAllocator.Clear();
	}

public:
	inline Command CreateCommand(ID cmdID, Entry func)
	{
		Command cmd;
		cmd.cmdID = cmdID;
		cmd.entry = func;
		return cmd;
	}

	template <typename T, typename ...Args>
	inline T* CreateParam(Command* cmd, Args&&... args)
	{
		T* ret = (T*)m_stackAllocator.Allocate(sizeof(T));
		new (ret) T(std::forward<Args>(args)...);

		cmd->param = ret;
		cmd->paramDtor = [](void* ptr)
		{
			auto p = (T*)ptr;
			p->~T();
		};

		return;
	}

	inline void Run(Command* cmd)
	{
		m_cmds.Add(*cmd);
	}

};

template <typename _C, size_t NUM_CMD>
class AsyncServer2
{
public:
	constexpr static size_t NUM_SERVERS = 2;

	using Server = AsyncServer<_C, NUM_CMD>;
	using Command = typename Server::Command;
	using Entry = typename Server::Entry;
	using CmdProcessor = typename Server::CmdProcessor;

	Server m_servers[NUM_SERVERS] = {};
	size_t m_curServerId = 0;

protected:
	inline Server& GetCurrentServer()
	{
		return m_servers[m_curServerId];
	}

	inline Server& GetPrevServer()
	{
		return m_servers[(m_curServerId + NUM_SERVERS - 1) % NUM_SERVERS];
	}

	inline void UpdateCurrentServer()
	{
		m_curServerId = (m_curServerId + 1) % NUM_SERVERS;
	}

	inline void SetCmdProcessor(ID cmdID, CmdProcessor cmdProcessor)
	{
		for (auto& server : m_servers)
		{
			server.SetCmdProcessor(cmdID, cmdProcessor);
		}
	}

	inline void ProcessAllCmds(Server& server, _C* self)
	{
		GetCurrentServer().ProcessAllCmds(self);
	}

public:
	// thread-safe methods
	inline Command CreateCommand(ID cmdID, Entry func)
	{
		return GetCurrentServer().CreateCommand(cmdID, func);
	}

	// thread-safe methods
	template <typename T, typename ...Args>
	inline T* CreateParam(Command* cmd, Args&&... args)
	{
		return GetCurrentServer().CreateParam(cmd, args...);
	}

	// thread-safe methods
	inline void Run(Command* cmd)
	{
		GetCurrentServer().Run(cmd);
	}

};

NAMESPACE_END