#pragma once
#include "Core/TypeDef.h"

#include "Core/Structures/Managed/Array.h"
#include "Core/Structures/Managed/ConcurrentList.h"
#include "Core/Memory/SmartPointers.h"
#include "Core/Time/Clock.h"

#include "Objects/GameObject.h"
#include "Objects/QueryStructures/AABBQuerySession.h"
#include "Objects/QueryStructures/AABBQueryStructure.h"

#include "Components/Physics/Physics.h"

#include "SceneQuerySession.h"


NAMESPACE_BEGIN

class AABBQueryStructure;
class BuiltinEventManager;
class EventManager;
class Engine;
class Input;

struct SceneStaticQuerySession final : public SceneQuerySession
{
	AABBQueryStructure* queryStructure;
	AABBQuerySession* session;

	virtual void Clear() override
	{
		session->ClearPrevQueryResult();
		begin = nullptr;
		end = nullptr;
	}

	~SceneStaticQuerySession()
	{
		if (queryStructure)
		{
			queryStructure->DeleteSession(session);
			queryStructure = nullptr;
			session = nullptr;
		}
	}
};

class Scene : Traceable<Scene>
{
protected:
	constexpr static ID TEMP_OBJECT_ID_MASK		= 0x10000000'00000000ULL;
	constexpr static ID TEMP_OBJECT_ID_UNMASK	= 0x7FFFFFFF'FFFFFFFFULL;

	friend class Engine;
	friend class DynamicLayer;
	friend class SubSystem;
	friend class SubSystemMergingUnit;
	friend class RenderingSystem;
	friend class PhysicsSystem;
	friend class ScriptSystem;

	raw::ConcurrentList<GameObject*> m_branchedObjects;

	Engine*				m_engine			= nullptr;
	Input*				m_input				= nullptr;
	RenderingSystem*	m_renderingSystem	= nullptr;
	PhysicsSystem*		m_physicsSystem		= nullptr;
	ScriptSystem*		m_scriptSystem		= nullptr;

	float m_dt = 0; // in sec
	float padd;
	size_t m_prevTimeSinceEpoch = 0;
	size_t m_curTimeSinceEpoch = 0;
	size_t m_iterationCount = 0;

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
	
	//Array<Handle<GameObject>>* m_objsAccessor = &m_stableObjects;


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
	Scene(Engine* engine);
	virtual ~Scene();

private:
	inline ID GetSceneId(bool isDynamic)
	{
		//return m_objsAccessor->Size() | TEMP_OBJECT_ID_MASK;
		return isDynamic ? (m_tempObjects.size() | TEMP_OBJECT_ID_MASK) : m_stableObjects.size();
	}

	inline ID UnmaskId(ID id)
	{
		return id & TEMP_OBJECT_ID_UNMASK;
	}

	void ProcessRemoveLists();
	void ProcessAddLists();
	void ProcessRecordedBranchedLists();

	inline void RecordBranchedObject(GameObject* obj)
	{
		assert(obj->m_scene == this);
		m_branchedObjects.Add(obj);
	}

	inline bool IsDynamicObject(GameObject* obj)
	{
		auto physicsComp = obj->GetComponentRaw<Physics>();
		if (physicsComp)
		{
			auto type = physicsComp->Type();

			switch (type)
			{
			case soft::Physics::STATIC:
				return false;
			case soft::Physics::DYNAMIC:
			case soft::Physics::KINEMATIC:
				return true;
				break;
			}
		}

		if (m_idMask == 0)
		{
			return false;
		}

		return true;
	}

public:
	inline void BeginSetup()
	{
		//m_objsAccessor = &m_stableObjects;
		m_idMask = 0;
	}

	inline void EndSetup()
	{
		//m_objsAccessor = &m_tempObjects;
		m_idMask = TEMP_OBJECT_ID_MASK;
		m_prevTimeSinceEpoch = m_curTimeSinceEpoch;
		m_curTimeSinceEpoch = Clock::ms::now();
	}

public:
	// thread-safe method
	// for user use
	void AddObject(Handle<GameObject>& obj);
	void RemoveObject(Handle<GameObject>& obj);

protected:
	// call whenever a dynamic object need to add, or remove
	// must be thread-safe methods
	virtual void AddDynamicObject(GameObject* obj) = 0;
	virtual void RemoveDynamicObject(GameObject* obj) = 0;

	virtual void RefreshDynamicObject(GameObject* obj) = 0;

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
	void Iteration();
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

public:
	friend class SceneQuerySession;

	UniquePtr<SceneStaticQuerySession> NewStaticQuerySession()
	{
		auto ret = MakeUnique<SceneStaticQuerySession>();
		ret->queryStructure = m_staticObjsQueryStructure;
		ret->session = m_staticObjsQueryStructure->NewSession();
		return ret;
	}

	// query static objects
	void AABBStaticQueryAABox(const AABox& aaBox, SceneStaticQuerySession* session);
	void AABBStaticQueryFrustum(const Frustum& frustum, SceneStaticQuerySession* session);



	virtual UniquePtr<SceneQuerySession> NewDynamicQuerySession() = 0;
	// query dynamic objects
	virtual void AABBDynamicQueryAABox(const AABox& aaBox, SceneQuerySession* session) = 0;
	virtual void AABBDynamicQueryFrustum(const Frustum& frustum, SceneQuerySession* session) = 0;

public:
	// refresh object on scene graph
	void RefreshObject(GameObject* obj);

public:
	inline auto GetRenderingSystem()
	{
		return m_renderingSystem;
	}

	inline auto GetPhysicsSystem()
	{
		return m_physicsSystem;
	}

	inline auto GetScriptSystem()
	{
		return m_scriptSystem;
	}

	inline auto GetInput()
	{
		return m_input;
	}

	inline auto GetIterationCount()
	{
		return m_iterationCount;
	}

	// delta time in sec
	inline auto Dt()
	{
		return m_dt;
	}

};

NAMESPACE_END