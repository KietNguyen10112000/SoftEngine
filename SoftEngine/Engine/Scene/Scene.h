#pragma once

#include <mutex>

#include "SceneObject.h"

#include "Core/ILightSystem.h"

namespace Math
{
class Rect2D;
class AARect2D;
class Trapezoid;
class Circle;

class Frustum;
class AABB;
class Sphere;
class Box;
};

class Engine;
class PhysicsEngine;
class ScriptEngine;

class Scene;
class SceneQueryContext;
class SceneIOContext;


class Scene
{
public:
	enum PHYSICS_ENGINE
	{
		BULLET
	};

	PHYSICS_ENGINE m_physicsEngineId = PHYSICS_ENGINE::BULLET;

	friend class PhysicsEngine;
	friend class ScriptEngine;
	PhysicsEngine* m_physicsEngine = 0;
	ScriptEngine* m_scriptEngine = 0;

	SceneQueryContext m_contexts[NUM_QUERY_CONTEXT];

	SceneIOContext* m_ioContext = 0;

	SceneObject* m_debugCamera = 0;

public:
	inline Scene(Engine* engine)
	{
	};

	inline virtual ~Scene() 
	{
	};

public:
	inline ScriptEngine* GetScriptEngine() { return m_scriptEngine; };
	inline PhysicsEngine* GetPhysicsEngine() { return m_physicsEngine; };

public:
	using SceneObjectList = std::vector<SceneObject*>;
	using SceneSharedObjectList = std::vector<SharedPtr<SceneSharedObject>>;
	using SharedObject = SharedPtr<SceneSharedObject>;

	///
	/// objects live on memory until scene deleted
	/// do not save any where because it will be deleted by scene
	/// 
	virtual void Query3DObjects(SceneQueryContext* context, Box* bounding, 
		SceneObjectList& output) = 0;
	virtual void Query3DObjects(SceneQueryContext* context, AABB* bounding,
		SceneObjectList& output) = 0;
	virtual void Query3DObjects(SceneQueryContext* context, Frustum* bounding,
		SceneObjectList& output) = 0;
	virtual void Query3DObjects(SceneQueryContext* context, Sphere* bounding,
		SceneObjectList& output) = 0;

	///
	/// shared for everyone
	/// can save every where
	/// 
	virtual void Query3DSharedObjects(SceneQueryContext* context, Box* bounding,
		SceneSharedObjectList& output) = 0;
	virtual void Query3DSharedObjects(SceneQueryContext* context, AABB* bounding,
		SceneSharedObjectList& output) = 0;
	virtual void Query3DSharedObjects(SceneQueryContext* context, Frustum* bounding,
		SceneSharedObjectList& output) = 0;
	virtual void Query3DSharedObjects(SceneQueryContext* context, Sphere* bounding,
		SceneSharedObjectList& output) = 0;


	virtual void AddSharedObjects(SharedObject* objects, size_t count) = 0;
	virtual void RemoveSharedObjects(SharedObject* objects, size_t count) = 0;


	///
	/// transfer owner
	///
	virtual void AddObjects(SceneObject* nodes, size_t count) = 0;
	virtual void RemoveObjects(SceneObject* nodes, size_t count) = 0;

	using FilterFunction = bool (*)(const SceneObjectBase&);

	virtual void FilterObjects(SceneObjectList& output, FilterFunction func = 0) = 0;
	virtual void FilterSharedObjects(SceneSharedObjectList& output, FilterFunction func = 0) = 0;


	inline auto GetDebugCamera()
	{
		return m_debugCamera;
	};

	inline void SetDebugCamera(SceneObject* camera)
	{
		m_debugCamera = camera;
	};

	inline SceneQueryContext* NewQueryContext(int64_t id)
	{
		auto ret = &m_contexts[id];
		ret->m_id = id;
		ret->m_owner = this;
		return ret;
	};

	inline void InterlockedAcquire(SceneQueryContext* context)
	{
		for (auto& ctx : m_contexts)
		{
			ctx.BeginFrame();
		}
	};

	inline void InterlockedRelease(SceneQueryContext* context)
	{
		for (auto& ctx : m_contexts)
		{
			ctx.EndFrame();
		}
	};

	inline void CheckNoQueries()
	{
		for (auto& ctx : m_contexts)
		{
			ctx.CheckIsNoQueringBlocking();
		}
	};

	////check if this scene is being updated by this thread
	//inline bool IsAcquired()
	//{
	//	for (auto& ctx : m_contexts)
	//	{
	//		std::unique_lock<std::mutex> lock(ctx.m_mutex, std::try_to_lock);
	//		if (!lock.owns_lock())
	//		{
	//			return false;
	//		}
	//	}

	//	return true;
	//};

	template <typename T>
	inline void QueryContextAcquireReadingObjects(SceneQueryContext* ctx, T objects)
	{
		for (auto& obj : objects)
		{
			obj->AcquireReading(ctx);
		}
	};

	template <typename T>
	inline void QueryContextReleaseReadingObjects(SceneQueryContext* ctx, T objects)
	{
		for (auto& obj : objects)
		{
			obj->ReleaseReading(ctx);
		}
	};

public:
	// helper functions
	inline SharedObject NewSharedLight()
	{
		auto ret = MakeShared<SceneSharedObject>();
		ret->Type() = SceneSharedObject::LIGHT_OBJECT;
		ret->ExternalData().Malloc<Light>();
		return ret;
	};

	inline SharedObject NewSharedObject()
	{
		auto ret = MakeShared<SceneSharedObject>();
		ret->Type() = SceneSharedObject::RENDERABLE_OBJECT;
		// no external data
		return ret;
	};

	inline SharedObject NewSharedCamera()
	{
		auto ret = MakeShared<SceneSharedObject>();
		ret->Type() = SceneSharedObject::CAMERA_OBJECT;
		ret->ExternalData().Malloc<Mat4x4>();
		return ret;
	};

	inline SceneObject* NewLight()
	{
		auto ret = new SceneObject();
		ret->Type() = SceneSharedObject::LIGHT_OBJECT;
		ret->ExternalData().Malloc<Light>();
		return ret;
	};

	inline SceneObject* NewObject()
	{
		auto ret = new SceneObject();
		ret->Type() = SceneSharedObject::RENDERABLE_OBJECT;
		// no external data
		return ret;
	};

	inline SceneObject* NewCamera()
	{
		auto ret = new SceneObject();
		ret->Type() = SceneSharedObject::CAMERA_OBJECT;
		ret->ExternalData().Malloc<Mat4x4>();
		return ret;
	};

};