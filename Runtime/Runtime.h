#pragma once

#include "Core/TypeDef.h"

#include "Core/Pattern/Singleton.h"
#include "Core/Structures/Managed/Array.h"
#include "Core/Structures/STD/STDContainers.h"

#include "Common/Utils/GenericStorage.h"
#include "Common/Utils/EventDispatcher.h"

#include <bitset>

NAMESPACE_BEGIN

class Input;
class Plugin;
class Scene;

class GameObjectCache;

class IterationHandler
{
public:
	// sumDt is sum of delta time from previous iteration
	// return remain sumDt
	virtual float DoIteration(float sumDt, Scene* scene) = 0;

};

class API Runtime : public Singleton<Runtime>
{
public:
	constexpr static byte NONE_STABLE_VALUE = 127;
	constexpr static byte STABLE_VALUE	= 0;
	constexpr static byte NUM_ARGS		= 128;

	constexpr static byte MAX_RUNNING_SCENES = 32;

	enum EVENT
	{
		// args[0] = <scene>
		EVENT_SCENE_CREATED,

		// args[0] = <the scene will be destroyed>
		EVENT_SCENE_DESTROYED,

		COUNT
	};

private:
	friend class Scene;

	Handle<Scene> m_scenes[MAX_RUNNING_SCENES];

	GenericStorage m_genericStorage;
	EventDispatcher<Runtime, EVENT::COUNT, EVENT, ID> m_eventDispatcher;

	Handle<GameObjectCache> m_gameObjectCache;

	std::bitset<2 * MAX_RUNNING_SCENES> m_runningSceneStableValue;

	Input* m_input = nullptr;
	void* m_window = nullptr;

	bool m_isRunning = true;

	std::atomic<bool> m_gcIsRunning = false;

	std::Vector<Plugin*> m_plugins;
	std::Vector<Plugin*> m_intevalPlugins;

	void* m_eventArgv[NUM_ARGS] = {};

	IterationHandler* m_iterationHandler = nullptr;

	spinlock m_createSceneLock;
	ID m_runningSceneIdx = INVALID_ID;
	ID m_nextRunningSceneIdx = INVALID_ID;

	ID m_destroyingScenes[MAX_RUNNING_SCENES] = {};
	size_t m_destroyingScenesCount = 0;

	spinlock m_noneStableValueLock;

public:
	static Handle<Runtime> Initialize();
	static void Finalize();

	Runtime();
	~Runtime();

	void InitializeModules();
	void FinalizeModules();

private:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_scenes);
		tracer->Trace(m_genericStorage);
		tracer->Trace(m_gameObjectCache);
	}

	void InitGraphics();
	void FinalGraphics();

	void InitNetwork();
	void FinalNetwork();

	void InitPlugins();
	void FinalPlugins();

	void ProcessDestroyScenes();

	byte GetNextStableValue();

	void DestroySceneImpl(Scene* scene);

	inline auto& NoneStableValueLock()
	{
		return m_noneStableValueLock;
	}

public:
	void Setup();

	void Run();

	void Iteration();

	void ProcessInput();

	void SynchronizeAllSubSystems();

	Handle<Scene> CreateScene();
	void DestroyScene(Scene* scene);

	void SetRunningScene(Scene* scene);

public:
	inline auto GetInput()
	{
		return m_input;
	}

	inline auto& IsRunning()
	{
		return m_isRunning;
	}

	inline auto SetIterationHandler(IterationHandler* handler)
	{
		m_iterationHandler = handler;
	}

	inline auto& GetScenes() const
	{
		return m_scenes;
	}

	inline auto* GenericStorage()
	{
		return &m_genericStorage;
	}

	inline auto* EventDispatcher()
	{
		return &m_eventDispatcher;
	}

	inline const Handle<Scene>& GetCurrentRunningScene() const
	{
		if (m_runningSceneIdx == INVALID_ID) return nullptr;

		return m_scenes[m_runningSceneIdx];
	}

	void* GetNativeHWND();

	inline GameObjectCache* GameObjectCache()
	{
		return m_gameObjectCache.Get();
	}
};


NAMESPACE_END