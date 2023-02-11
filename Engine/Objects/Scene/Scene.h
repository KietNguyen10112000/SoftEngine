#pragma once
#include "Core/TypeDef.h"

#include "Core/Structures/Managed/Array.h"

#include "Objects/GameObject.h"

NAMESPACE_BEGIN

class AABBQueryStructure;
class BuiltinEventManager;
class EventManager;

class Scene
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
	/// 2 query structures, 1 for static object, 1 for dynamic object
	/// 
	AABBQueryStructure* m_staticObjsQueryStructure = nullptr;
	AABBQueryStructure* m_dynamicObjsQueryStructure = nullptr;

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

	void AddObject(Handle<GameObject>& obj);
	void RemoveObject(Handle<GameObject>& obj);

public:
	///
	/// re-construct the scene for next frame
	/// include:
	/// + Defer add, remove objects
	/// + Re-construct query structure
	/// 
	virtual void ReConstruct() = 0;

	///
	/// mix data from interal processing task with game logic task
	/// include:
	/// + mix physics and scripting
	///
	virtual void Synchronize() = 0;

};

NAMESPACE_END