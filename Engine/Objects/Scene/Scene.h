#pragma once
#include "Core/TypeDef.h"

#include "Core/Structures/Managed/Array.h"

#include "Objects/GameObject.h"

NAMESPACE_BEGIN

class AABBQueryStructure;

class Scene
{
protected:
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

	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_tempObjects);
	}

public:
	inline virtual ~Scene() {};

protected:
	inline void SetId(GameObject* obj, ID id)
	{
		obj->m_id = id;
	}

public:
	inline void BeginSetup()
	{
		m_objsAccessor = &m_stableObjects;
	}

	inline void EndSetup()
	{
		m_objsAccessor = &m_tempObjects;
	}

	void Add(const Handle<GameObject>& obj);

public:
	///
	/// re-construct the scene for next frame
	/// include:
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