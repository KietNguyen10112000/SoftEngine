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
class ModifiedRecorder;

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
	constexpr static byte NUM_ARGS		= 128;

	enum EVENT
	{
		// args[0] = <scene>
		EVENT_SCENE_CREATED,

		// args[0] = <the scene will be destroyed>
		EVENT_SCENE_DESTROYED,

#ifdef PLUGIN_ALLOW_HOT_RELOAD
		EVENT_HOT_RELOAD_SCRIPTS_BEGIN,
		EVENT_HOT_RELOAD_SCRIPTS_END,
#endif

		COUNT
	};

private:
	friend class Scene;
	friend class GameObject;

	struct DestroyingSceneInfo
	{
		Scene* scene;

		// the scene need to be destroyed after 3 iterations after calling destroy because ModifiedRecord keeps the trace to the objects of destroying scene, 
		// so when we perform GC, the object of destroyed scene will not be actually destroyed, defer destroy prevents some unexpected behaviours like this
		size_t count = 3;

		inline DestroyingSceneInfo(Scene* scene) : scene(scene) {};
	};

	Array<Handle<Scene>> m_scenes;

	GenericStorage m_genericStorage;
	EventDispatcher<Runtime, EVENT::COUNT, EVENT, ID> m_eventDispatcher;

	Handle<GameObjectCache> m_gameObjectCache;

	Handle<ModifiedRecorder> m_modifiedRecorder[2];
	ID m_curModifiedRecorderId = 0;

	//std::bitset<2 * MAX_RUNNING_SCENES> m_runningSceneStableValue;

	Input* m_input = nullptr;
	void* m_window = nullptr;

	bool m_isRunning = true;

	std::atomic<bool> m_gcIsRunning = false;

	std::Vector<Plugin*> m_plugins;
	std::Vector<Plugin*> m_intevalPlugins;

	void* m_eventArgv[NUM_ARGS] = {};

	IterationHandler* m_iterationHandler = nullptr;

	spinlock m_createSceneLock;
	Scene* m_runningScene = nullptr;
	Scene* m_nextRunningScene = nullptr;

	std::vector<DestroyingSceneInfo> m_destroyingScenes = {};

	//spinlock m_lock;

#ifdef PLUGIN_ALLOW_HOT_RELOAD
	bool m_reloadScripts = false;
#endif

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
		tracer->Trace(m_modifiedRecorder);
	}

	void InitGraphics();
	void FinalGraphics();

	void InitNetwork();
	void FinalNetwork();

	void InitPlugins();
	void FinalPlugins();

	void ProcessDestroyScenes();

	void DestroySceneImpl(Scene* scene);

	/*inline auto& NoneStableValueLock()
	{
		return m_noneStableValueLock;
	}*/

	inline auto& GetModifiedRecorder()
	{
		return m_modifiedRecorder[m_curModifiedRecorderId];
	}

	void SwapModifiedRecorder();
	void ProcessSwapRunningScene();

public:
	void Setup();

	void Run();

	void Iteration();

	void ProcessInput();

	void SynchronizeAllSubSystems();

	Handle<Scene> CreateScene(Scene* holder = nullptr);
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

	inline Scene* GetCurrentRunningScene()
	{
		return m_runningScene;
	}

	void* GetNativeHWND();

	inline GameObjectCache* GameObjectCache()
	{
		return m_gameObjectCache.Get();
	}

#ifdef PLUGIN_ALLOW_HOT_RELOAD
private:
	void HotReloadScriptsImpl();

public:
	void HotReloadScripts();
#endif

};


NAMESPACE_END