#pragma once

#include <mutex>

#include "Core/MultiThreading/CriticalSection.h"

class Scene;

///
/// This class provide rules for read and write data from scene
/// Each worker thread has a SceneQueryContext
///
/// 
/// usage example:
///
/// void Update()
/// {
///     // assume we have SceneQueryContext* ctx;
///     ctx->Begin();
///
///
///     // do all query and save use node, ...
///     // when query return, scene set all queried nodes to be acquired by this ctx
///     // when node is acquired by other contexts, shouldn't write data to node
///     scene->Query(ctx, ...);
///     ...
///
///
///     // no more query
///     ctx->End();
///
///
///     // read data from node, release node for other contexts write data to these nodes
///     // or prevent write data to this node by release later,
///     // process node, rendering, do physics, logic update, ...
///     ...
///
///
///     // remember check no contexts is quering
///     scene->CheckNoQueries();
///
///
///     // check node available and write data back to node
///     ...
///
/// }
///
class SceneQueryContext
{
public:
	static constexpr int64_t INVALID_ID = -1;

public:
	std::mutex m_frameMutex;
	//CriticalSection m_frameCriticalSection = {};
	std::mutex m_queryMutex;
	//CriticalSection m_queryCriticalSection = {};
	Scene* m_owner = 0;
	int64_t m_id = INVALID_ID;

public:
	inline void BeginFrame()
	{
		m_frameMutex.lock();
		//m_frameCriticalSection.Enter();
	};

	inline void EndFrame()
	{
		m_frameMutex.unlock();
		//m_frameCriticalSection.Leave();
	};

	inline void BeginQuery()
	{
		m_queryMutex.lock();
		//m_queryCriticalSection.Enter();
	};

	inline void EndQuery()
	{
		m_queryMutex.unlock();
		//m_queryCriticalSection.Leave();
	};

	inline auto GetID() const { return m_id; };

	inline void CheckIsNoQueringBlocking()
	{
		m_queryMutex.lock();
		m_queryMutex.unlock();
		//m_queryCriticalSection.Enter();
		//m_queryCriticalSection.Leave();
	};

	inline void CheckNoFrameBlocking()
	{
		m_frameMutex.lock();
		m_frameMutex.unlock();
	};

};
