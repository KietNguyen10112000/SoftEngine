#pragma once
#include "Core/TypeDef.h"

#include "Core/Structures/STD/STDContainers.h"
#include "Core/Structures/Managed/Array.h"
#include "Core/Structures/Managed/ConcurrentList.h"
#include "Core/Memory/SmartPointers.h"
#include "Core/Time/Clock.h"

#include "Math/Math.h"

#include "SubSystems2D/SubSystemInfo2D.h"


NAMESPACE_BEGIN

class GameObject2D;

class Scene2DQuerySession
{
public:
	GameObject2D** m_begin;
	GameObject2D** m_end;

	virtual void Clear() = 0;

	inline auto begin() const
	{
		return m_begin;
	}

	inline auto end() const
	{
		return m_end;
	}

};

class API Scene2D : Traceable<Scene2D>
{
protected:
	constexpr static byte SCENE_MEM_STABLE_VALUE1 = 1;
	constexpr static byte SCENE_MEM_STABLE_VALUE2 = 2;

	constexpr static ID TEMP_OBJECT_ID_MASK = 0x10000000'00000000ULL;
	constexpr static ID TEMP_OBJECT_ID_UNMASK = 0x7FFFFFFF'FFFFFFFFULL;

	constexpr static ID GHOST_OBJECT_ID_MASK = 0x01000000'00000000ULL;
	constexpr static ID GHOST_OBJECT_ID_UNMASK = 0xF7FFFFFF'FFFFFFFFULL;

	friend class Engine;
	friend class Input;
	friend class SubSystem2D;
	friend class RenderingSystem2D;
	friend class PhysicsSystem2D;
	friend class ScriptSystem2D;
	friend class GameObject2D;

	Engine* m_engine = nullptr;
	Input* m_input = nullptr;
	RenderingSystem2D* m_renderingSystem = nullptr;
	PhysicsSystem2D* m_physicsSystem = nullptr;
	ScriptSystem2D* m_scriptSystem = nullptr;

	SubSystem2D* m_subSystems[SubSystemInfo2D::INDEXED_SUBSYSTEMS_COUNT] = {};

	float m_lockedDt = 0;
	float m_dt = 0; // in sec
	//float padd;
	size_t m_prevTimeSinceEpoch = 0;
	size_t m_curTimeSinceEpoch = 0;
	size_t m_iterationCount = 0;

	///
	/// for long live-time object
	/// eg: static object
	/// 
	/// this list can be very large (hundreds of thousands to millions objs)
	/// 
	Array<Handle<GameObject2D>> m_stableObjects;

	///
	/// for short live-time object
	/// eg: dynamic and kinematic object that spawned to scene in runtime (bullet, spell, sensor, ...)
	/// 
	Array<Handle<GameObject2D>> m_tempObjects;

	// for defer delete
	Array<Handle<GameObject2D>> m_trash;

	std::Vector<GameObject2D*> m_removes;

	std::Vector<GameObject2D*> m_adds;

	size_t m_uidCount = 0;

private:
	ID m_oldStableValue = 0;
	ID m_id = 0;
	ID m_idMask = 0;
	ID m_uidCounter = 0;

protected:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_tempObjects);
		tracer->Trace(m_trash);
	}

public:
	Scene2D(Engine* engine);
	virtual ~Scene2D();

	void Setup();

	// call me at child class destructor
	void Dtor();

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

	//bool IsDynamicObject(GameObject2D* obj);

public:
	inline void BeginSetup()
	{
		//m_objsAccessor = &m_stableObjects;
		m_oldStableValue = mheap::internal::GetStableValue();
		byte value = SCENE_MEM_STABLE_VALUE1;
		value = m_id % 2 ? value : SCENE_MEM_STABLE_VALUE2;
		mheap::internal::SetStableValue(value);

		m_idMask = 0;
	}

	inline void EndSetup()
	{
		auto value = mheap::internal::GetStableValue();
		mheap::internal::SetStableValue((byte)m_oldStableValue);
		m_oldStableValue = value;
		//m_objsAccessor = &m_tempObjects;
		m_idMask = TEMP_OBJECT_ID_MASK;
		m_prevTimeSinceEpoch = m_curTimeSinceEpoch;
		m_curTimeSinceEpoch = Clock::ms::now();
	}

public:
	// for user use
	void AddObject(Handle<GameObject2D>& obj, bool isGhost = false);
	void RemoveObject(const Handle<GameObject2D>& obj);

protected:
	virtual void AddStaticObject(GameObject2D* obj) = 0;
	virtual void RemoveStaticObject(GameObject2D* obj) = 0;

	// call whenever a dynamic object need to add, or remove
	virtual void AddDynamicObject(GameObject2D* obj) = 0;
	virtual void RemoveDynamicObject(GameObject2D* obj) = 0;

	void RemoveObjectImpl(GameObject2D* obj);
	void ProcessRemoveList();

	void AddObjectImpl(GameObject2D* obj);
	void ProcessAddList();

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

public:
	friend class Scene2DQuerySession;

	virtual Scene2DQuerySession* NewQuerySession() = 0;
	virtual void DeleteQuerySession(Scene2DQuerySession*) = 0;

	// query static objects
	virtual void AABBStaticQueryAARect(const AARect& aaRect, Scene2DQuerySession* session) = 0;

	// query dynamic objects
	virtual void AABBDynamicQueryAARect(const AARect& aaRect, Scene2DQuerySession* session) = 0;

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

	inline auto GetSubSystem(ID id)
	{
		return m_subSystems[id];
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

	inline void LockDt(float dt)
	{
		m_dt = dt;
		m_lockedDt = dt;
	}

};

NAMESPACE_END