#pragma once
#include "Core/TypeDef.h"

#include "Core/Structures/Managed/Array.h"
#include "Core/Structures/Managed/ConcurrentList.h"

#include "Objects/GameObject.h"

NAMESPACE_BEGIN

class AABBQueryStructure;
class BuiltinEventManager;
class EventManager;

class Scene : Traceable<Scene>
{
protected:
	constexpr static ID TEMP_OBJECT_ID_MASK		= 0x10000000'00000000ULL;
	constexpr static ID TEMP_OBJECT_ID_UNMASK	= 0x7FFFFFFF'FFFFFFFFULL;

	///
	/// for long live-time object
	/// eg: static object
	/// 
	/// this list can be very large (hundreds of thousands to millions objs)
	/// 
	Array<Handle<GameObject>> m_stableObjects;

	///
	/// for short live-time object
	/// eg: dynamic and kinematic object that spawned to scene in runtime (bullet, spell, sensor, ...)
	/// 
	Array<Handle<GameObject>> m_tempObjects;


	/// 
	/// with backend memory, no object pooling is needed (but still available)
	/// 
	
	Array<Handle<GameObject>>* m_objsAccessor = &m_stableObjects;


	/// 
	/// 1 query structures for static object
	/// 
	AABBQueryStructure* m_staticObjsQueryStructure = nullptr;

	/// 
	/// defer lists for scene reconstruction
	/// 
	
private:
	// object need to add to scene
	ConcurrentList<Handle<GameObject>> m_waitForAdd;

	// object need to remove from scene
	ConcurrentList<Handle<GameObject>> m_waitForRemoves[2];
	ConcurrentList<Handle<GameObject>>* m_waitForRemove = &m_waitForRemoves[0];

	ID m_idMask = 0;

	ID m_uidCounter = 0;

	friend class Engine;
	BuiltinEventManager* m_objectEventMgr = nullptr;

	Handle<EventManager> m_eventMgr = nullptr;

protected:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_tempObjects);
		tracer->Trace(m_waitForAdd);
		tracer->Trace(m_waitForRemoves);
		tracer->Trace(m_eventMgr);
	}

public:
	Scene();
	virtual ~Scene();

private:
	inline ID GetSceneId()
	{
		return m_objsAccessor->Size() | TEMP_OBJECT_ID_MASK;
	}

	inline ID UnmaskId(ID id)
	{
		return id & TEMP_OBJECT_ID_UNMASK;
	}

	void ProcessRemoveLists();
	void ProcessAddLists();

public:
	inline void BeginSetup()
	{
		m_objsAccessor = &m_stableObjects;
		m_idMask = 0;
	}

	inline void EndSetup()
	{
		m_objsAccessor = &m_tempObjects;
		m_idMask = TEMP_OBJECT_ID_MASK;
	}

public:
	// thread-safe method
	void AddObject(Handle<GameObject>& obj);
	void RemoveObject(Handle<GameObject>& obj);

protected:
	// call whenever a dynamic object need to add, or remove
	// must be thread-safe methods
	virtual void AddDynamicObject(GameObject* obj) = 0;
	virtual void RemoveDynamicObject(GameObject* obj) = 0;

	inline static ID GetObjectAABBQueryId(GameObject* obj)
	{
		return obj->m_aabbQueryId;
	}

	inline static void SetObjectAABBQueryId(GameObject* obj, ID id)
	{
		obj->m_aabbQueryId = id;
	}

public:
	void PrevIteration();
	void PostIteration();

	///
	/// re-construct the scene for next frame
	/// include:
	/// + Defer add, remove objects
	/// + Re-construct query structure
	/// 
	/// call at the begining of iteration
	/// 
	virtual void ReConstruct() = 0;

	/// 
	/// call at the end of iteration
	///
	virtual void Synchronize() = 0;

};

NAMESPACE_END