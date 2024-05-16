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

#include "Common/Utils/GenericStorage.h"
#include "Common/Utils/EventDispatcher.h"
#include "Common/Base/Serializable.h"

#include "Runtime/Config.h"

#include "DeferredBuffer.h"

NAMESPACE_BEGIN

class MainComponent;
class GameObject;
class Input;

// manage game objects and notify for main systems whenever game object add to scene, remove from scene, changed transform,...
class API Scene final : public Serializable
{
public:
	enum EVENT
	{
		// no args
		EVENT_SETUP_LONGLIFE_OBJECTS,

		// args[0] = <list game objects>: std::vector<GameObject*>
		EVENT_OBJECTS_ADDED,

		// args[0] = <list game objects>: std::vector<GameObject*>
		EVENT_OBJECTS_REMOVED,

		// no args
		EVENT_BEGIN_ITERATION,

		// no args
		EVENT_END_ITERATION,

		// args[0] = serializer: Serializer
		EVENT_SERIALIZE,

		// args[0] = serializer: Serializer
		EVENT_DESERIALIZE,

		// no args
		EVENT_BEGIN_RUNNING,

		// no args
		EVENT_END_RUNNING,

		COUNT
	};

private:
	friend class Runtime;
	friend class GameObject;
	MAIN_SYSTEM_FRIEND_CLASSES();

	SERIALIZABLE_CLASS(Scene);

	constexpr static size_t NUM_TRASH_ARRAY = 2;
	constexpr static size_t NUM_DEFER_LIST = Config::NUM_DEFER_BUFFER;

	struct IterationTaskParam
	{
		Scene* scene = nullptr;
		ID mainSystemId;
	};

	Array<Handle<GameObject>> m_longLifeObjects;
	Array<Handle<GameObject>> m_shortLifeObjects;

	Array<Handle<void>> m_trashObjects[NUM_TRASH_ARRAY];

	GenericStorage m_genericStorage;
	EventDispatcher<Scene, EVENT::COUNT, EVENT, ID> m_eventDispatcher;

	ID m_currentDeferBufferIdx = 0;
	ID m_prevDeferBufferIdx = 0;

	MainSystem*				 m_mainSystems[MainSystemInfo::COUNT] = {};

	bool m_isSettingUpLongLifeObjects = false;
	byte m_stableValue = 0;
	byte m_oldStableValue = 0;
	bool m_destroyed = false;

	spinlock m_lock;
	bool m_padd[3];

	size_t m_iterationCount = 0;
	float m_dt = 0;

	IterationTaskParam	m_taskParams							[MainSystemInfo::COUNT] = {};
	Task				m_mainSystemIterationTasks				[MainSystemInfo::COUNT] = {};
	Task				m_mainOutputSystemIterationTasks		[MainSystemInfo::COUNT] = {};
	uint32_t			m_numMainOutputSystem											= 0;
	Task				m_mainProcessingSystemIterationTasks	[MainSystemInfo::COUNT] = {};
	uint32_t			m_numMainProcessingSystem										= 0;

	Task				m_mainSystemModificationTasks			[MainSystemInfo::COUNT] = {};

	/*std::atomic<size_t>		m_numMainSystemEndReconstruct = 0;
	TaskWaitingHandle		m_endReconstructWaitingHandle = { 0,0 };
	Task					m_endReconstructTask = {};*/

	TaskWaitingHandle		m_objectsModificationTaskWaitingHandle = { 0,0 };

	Input* m_input = nullptr;
	ID m_runtimeID = INVALID_ID;
	ID m_UIDCounter = 0;

	raw::ConcurrentArrayList<DeferredBufferControlBlock*> m_deferredBuffers1;
	raw::ConcurrentArrayList<DeferredBufferControlBlock*> m_deferredBuffers2;

	std::vector<GameObject*> m_addedObjectInFrame;
	std::vector<GameObject*> m_removedObjectInFrame;

public:
	Scene();
	//Scene(Runtime* runtime);
	~Scene();

private:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_longLifeObjects);
		tracer->Trace(m_shortLifeObjects);
		tracer->Trace(m_trashObjects);
		tracer->Trace(m_genericStorage);
		//tracer->Trace(m_mainSystems);
		//tracer->Trace(m_removeList);
	}

	void BakeAllMainSystems();
	void SetupMainSystemIterationTasks();
	void SetupMainSystemModificationTasks();

	void SetupDeferLists();

	void SynchMainProcessingSystems();
	void SynchMainProcessingSystemForMainOutputSystems();
	void UpdateDeferredBuffers(decltype(m_deferredBuffers1)& buffers);

	void BeginIteration();
	void EndIteration();

	void PerformModificationForMainSystem(ID id);
	void FlushAsyncTasksForMainSystem(ID id);

	inline auto& GetCurrentTrash()
	{
		return m_trashObjects[m_iterationCount % NUM_TRASH_ARRAY];
	}

	inline auto& GetPrevTrash()
	{
		return m_trashObjects[(m_iterationCount + NUM_TRASH_ARRAY - 1) % NUM_TRASH_ARRAY];
	}

	void AddLongLifeObject(const Handle<GameObject>& obj, bool indexedName);
	void AddLongLifeComponent(ID COMPONENT_ID, const Handle<MainComponent>& component);

	// Inherited via Serializable
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;
	Handle<ClassMetadata> GetMetadata(size_t sign) override;
	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

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

	inline ID GetCurrentDeferBufferIdx()
	{
		return m_currentDeferBufferIdx;
	}

	inline ID GetPrevDeferBufferIdx()
	{
		return m_prevDeferBufferIdx;
	}

public:
	inline MainSystem* GetMainSystem(ID id)
	{
		return m_mainSystems[id];
	}

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

	inline AnimationSystem* GetAnimationSystem()
	{
		return (AnimationSystem*)m_mainSystems[MainSystemInfo::ANIMATION_ID];
	}

	inline auto* GetInput()
	{
		return m_input;
	}

	inline auto GetIterationCount() const
	{
		return m_iterationCount;
	}

	inline auto* GenericStorage()
	{
		return &m_genericStorage;
	}

	inline auto* EventDispatcher()
	{
		return &m_eventDispatcher;
	}

public:
	void BeginRunning();
	void EndRunning();

	void CleanUp();
	//virtual Handle<ClassMetadata> GetMetadata(size_t sign) override;
	//virtual void OnPropertyChanged(const UnknownAddress& var) override;

private:
	template <bool PROCESSING_SYS_TO_OUTPUT_SYS, typename T, uint32_t N>
	inline void Update(DeferredBuffer<T, N>& buffer)
	{
		auto ctrlBlock = (DeferredBufferControlBlock*)&buffer;
		if (ctrlBlock->m_lastUpdateIteration.load(std::memory_order_relaxed) == GetIterationCount())
		{
			return;
		}

		if (ctrlBlock->m_lastUpdateIteration.exchange(GetIterationCount()) == GetIterationCount())
		{
			return;
		}

		if constexpr (PROCESSING_SYS_TO_OUTPUT_SYS)
		{
			m_deferredBuffers1.Add(ctrlBlock);
		}
		else
		{
			m_deferredBuffers2.Add(ctrlBlock);
		}
	}

public:
	template <bool COPY_ON_WRITE = true, bool PROCESSING_SYS_TO_OUTPUT_SYS = true, typename T, uint32_t N>
	inline void BeginWrite(DeferredBuffer<T, N>& buffer)
	{
		auto ctrlBlock = (DeferredBufferControlBlock*)&buffer;

#ifdef _DEBUG
		ctrlBlock->m_isWriting++;
#endif // _DEBUG

		if (ctrlBlock->m_lastUpdateIteration.load(std::memory_order_relaxed) == GetIterationCount())
		{
			return;
		}

		if constexpr (COPY_ON_WRITE)
		{
			buffer.CopyOnWrite();
		}
	}

	template <bool UPDATE = true, bool PROCESSING_SYS_TO_OUTPUT_SYS = true, typename T, uint32_t N>
	inline void EndWrite(DeferredBuffer<T, N>& buffer)
	{
#ifdef _DEBUG
		buffer.m_isWriting--;
#endif // _DEBUG
		if constexpr (UPDATE)
			Update<PROCESSING_SYS_TO_OUTPUT_SYS>(buffer);
	}

	inline float Dt() const
	{
		return m_dt;
	}

};

NAMESPACE_END