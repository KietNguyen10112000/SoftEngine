#pragma once

//#include "GameObject.h"

//#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"
#include "Core/Structures/String.h"
#include "Core/Structures/Managed/Array.h"
#include "Core/Structures/Managed/ConcurrentList.h"
#include "Core/Structures/Raw/ConcurrentList.h"

#include "TaskSystem/TaskSystem.h"
#include "MainSystem/MainSystemInfo.h"

NAMESPACE_BEGIN

class GameObject;
class Input;

// manage game objects and notify for main systems whenever game object add to scene, remove from scene, changed transform,...
class Scene final : Traceable<Scene>
{
private:
	friend class GameObject;
	MAIN_SYSTEM_FRIEND_CLASSES();

	constexpr static size_t NUM_TRASH_ARRAY = 2;
	constexpr static size_t NUM_DEFER_LIST = 2;

	struct IterationTaskParam
	{
		Scene* scene = nullptr;
		ID mainSystemId;
	};

	Array<Handle<GameObject>> m_longLifeObjects;
	Array<Handle<GameObject>> m_shortLifeObjects;

	Array<Handle<GameObject>> m_trashObjects[NUM_TRASH_ARRAY];

	ConcurrentList<Handle<GameObject>> m_addListHolder;
	ConcurrentList<Handle<GameObject>> m_removeListHolder;

	raw::ConcurrentArrayList<GameObject*> m_addList					[NUM_DEFER_LIST] = {};
	raw::ConcurrentArrayList<GameObject*> m_removeList				[NUM_DEFER_LIST] = {};
	raw::ConcurrentArrayList<GameObject*> m_changedTransformList	[NUM_DEFER_LIST] = {};

	// no child, no parent, just an order to call MainComponent::OnTransformChanged
	std::vector<GameObject*> m_stagedChangeTransformList			[NUM_DEFER_LIST] = {};
	//std::vector<GameObject*> m_changedTransformRoots;

	MainSystem*				 m_mainSystems[MainSystemInfo::COUNT] = {};

	bool m_isSettingUpLongLifeObjects = false;
	byte m_stableValue = 0;
	byte m_oldStableValue = 0;
	byte padd;

	size_t m_iterationCount = 0;
	float m_dt = 0;

	IterationTaskParam	m_taskParams							[MainSystemInfo::COUNT] = {};
	Task				m_mainSystemIterationTasks				[MainSystemInfo::COUNT] = {};
	Task				m_mainOutputSystemIterationTasks		[MainSystemInfo::COUNT] = {};
	uint32_t			m_numMainOutputSystem											= 0;
	Task				m_mainProcessingSystemIterationTasks	[MainSystemInfo::COUNT] = {};
	uint32_t			m_numMainProcessingSystem										= 0;

	Task				m_mainSystemModificationTasks			[MainSystemInfo::COUNT] = {};

	std::atomic<size_t>		m_numMainSystemEndReconstruct = 0;
	TaskWaitingHandle		m_endReconstructWaitingHandle = { 0,0 };
	Task					m_endReconstructTask = {};

	Input* m_input = nullptr;

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
		tracer->Trace(m_addListHolder);
		tracer->Trace(m_removeListHolder);
		//tracer->Trace(m_mainSystems);
		//tracer->Trace(m_removeList);
	}

	void BakeAllMainSystems();
	void SetupMainSystemIterationTasks();
	void SetupMainSystemModificationTasks();

	void SetupDeferLists();

	/// 
	/// add all object's components to main system
	/// 
	void ProcessAddObjectListForMainSystem(ID mainSystemId);

	/// 
	/// remove all object's components from main system
	/// 
	void ProcessRemoveObjectListForMainSystem(ID mainSystemId);

	/// 
	/// when object transform changed, call me
	/// 
	void OnObjectTransformChanged(GameObject* obj);
	void ProcessChangedTransformListForMainSystem(ID mainSystemId);

	void ProcessModificationForMainSystem(ID mainSystemId);
	void ProcessModificationForAllMainSystems();

	void FilterAddList();
	void FilterRemoveList();

	void StageAllChangedTransformObjects();
	void SynchMainProcessingSystems();
	void SynchMainProcessingSystemForMainOutputSystems();

	void EndReconstructForMainSystem(ID mainSystemId);
	void EndReconstructForAllMainSystems();

	void BeginIteration();
	void EndIteration();

	inline auto& GetCurrentTrash()
	{
		return m_trashObjects[m_iterationCount % NUM_TRASH_ARRAY];
	}

	inline auto& GetCurrentAddList()
	{
		return m_addList[m_iterationCount % NUM_DEFER_LIST];
	}

	inline auto& GetCurrentRemoveList()
	{
		return m_removeList[m_iterationCount % NUM_DEFER_LIST];
	}

	inline auto& GetCurrentChangedTransformList()
	{
		return m_changedTransformList[m_iterationCount % NUM_DEFER_LIST];
	}

	inline auto& GetCurrentStagedChangeTransformList()
	{
		return m_stagedChangeTransformList[m_iterationCount % NUM_DEFER_LIST];
	}

	inline auto& GetPrevTrash()
	{
		return m_trashObjects[(m_iterationCount + NUM_DEFER_LIST - 1) % NUM_DEFER_LIST];
	}

	inline auto& GetPrevAddList()
	{
		return m_addList[(m_iterationCount + NUM_DEFER_LIST - 1) % NUM_DEFER_LIST];
	}

	inline auto& GetPrevRemoveList()
	{
		return m_removeList[(m_iterationCount + NUM_DEFER_LIST - 1) % NUM_DEFER_LIST];
	}

	inline auto& GetPrevChangedTransformList()
	{
		return m_changedTransformList[(m_iterationCount + NUM_DEFER_LIST - 1) % NUM_DEFER_LIST];
	}

	inline auto& GetPrevStagedChangeTransformList()
	{
		return m_stagedChangeTransformList[(m_iterationCount + NUM_DEFER_LIST - 1) % NUM_DEFER_LIST];
	}

public:
	// defer implementation, multithreaded
	void AddObject(const Handle<GameObject>& obj, bool indexedName = false);

	// defer implementation, multithreaded
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

	inline auto* GetInput()
	{
		return m_input;
	}

};

NAMESPACE_END