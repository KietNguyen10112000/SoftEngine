#pragma once

//#include "GameObject.h"

//#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"
#include "Core/Structures/String.h"
#include "Core/Structures/Managed/Array.h"
#include "TaskSystem/TaskSystem.h"
#include "MainSystem/MainSystemInfo.h"

NAMESPACE_BEGIN

class GameObject;

// manage game objects and notify for main systems whenever game object add to scene, remove from scene, changed transform,...
class Scene final : Traceable<Scene>
{
private:
	MAIN_SYSTEM_FRIEND_CLASSES();

	constexpr static size_t NUM_TRASH_ARRAY = 2;

	struct NotifyTaskParam
	{
		Scene* scene = nullptr;
		ID mainSystemId;
	};

	Array<Handle<GameObject>> m_longLifeObjects;
	Array<Handle<GameObject>> m_shortLifeObjects;

	Array<Handle<GameObject>> m_trashObjects[NUM_TRASH_ARRAY];

	std::vector<GameObject*> m_addList;

	std::vector<GameObject*> m_removeList;

	std::vector<GameObject*> m_changedTransformList;

	MainSystem* m_mainSystems[MainSystemInfo::COUNT] = {};

	bool m_isSettingUpLongLifeObjects = false;
	byte m_stableValue = 0;
	byte m_oldStableValue = 0;
	byte padd;

	size_t m_iterationCount = 0;

	NotifyTaskParam		m_taskParams							[MainSystemInfo::COUNT] = {};
	Task				m_notifyAddListTasks					[MainSystemInfo::COUNT] = {};
	Task				m_notifyRemoveListTasks					[MainSystemInfo::COUNT] = {};
	Task				m_notifyChangedTransformListTasks		[MainSystemInfo::COUNT] = {};

public:
	Scene(Runtime* runtime);
	~Scene();

private:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		//tracer->Trace(m_longLifeObjects);
		tracer->Trace(m_shortLifeObjects);
		tracer->Trace(m_trashObjects);
		tracer->Trace(m_removeList);
	}

	void BakeAllMainSystems();
	void SetupNotifyTasks();

	/// 
	/// add all object's components to main system
	/// 
	void NotifyAddObjectListForMainSystem();

	/// 
	/// remove all object's components from main system
	/// 
	void NotifyRemoveObjectListForMainSystem();

	/// 
	/// when object transform changed, call me
	/// 
	void OnObjectTransformChanged(GameObject* obj);
	void NotifyChangedTransformListForMainSystem();

	inline auto& GetCurrentTrash()
	{
		return m_trashObjects[m_iterationCount % NUM_TRASH_ARRAY];
	}

public:
	// defer implementation
	void AddObject(const Handle<GameObject>& obj, bool indexedName = false);

	// defer implementation
	void RemoveObject(const Handle<GameObject>& obj);

	Handle<GameObject> FindObjectByIndexedName(String name);

	///
	/// setup the object that live in scene for the long time, eg: staic object like building, ...
	/// 
	/// if (scene->BeginSetupLongLifeObject())
	/// {
	///		// do setup here
	/// 
	///		scene->EndSetupLongLifeObject();
	/// }
	/// 
	bool BeginSetupLongLifeObject();
	void EndSetupLongLifeObject();

	void Iteration(float dt);

public:
	inline RenderingSystem* GetRenderingSystem()
	{
		return (RenderingSystem*)m_mainSystems[MainSystemInfo::RENDERING_ID];
	}

	inline PhysicsSystem* GetPhysicsSystem()
	{
		return (PhysicsSystem*)m_mainSystems[MainSystemInfo::PHYSICS_ID];
	}

	inline ScriptingSystem* GetScriptingSystem()
	{
		return (ScriptingSystem*)m_mainSystems[MainSystemInfo::SCRIPTING_ID];
	}

	inline AudioSystem* GetAudioSystem()
	{
		return (AudioSystem*)m_mainSystems[MainSystemInfo::AUDIO_ID];
	}

};

NAMESPACE_END