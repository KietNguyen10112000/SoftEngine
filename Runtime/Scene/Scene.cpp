#include "Scene.h"

#include "Runtime.h"
#include "GameObject.h"

#include "MainSystem/MainSystem.h"
#include "MainSystem/Rendering/RenderingSystem.h"
#include "MainSystem/Physics/PhysicsSystem.h"
#include "MainSystem/Scripting/ScriptingSystem.h"
#include "MainSystem/Animation/AnimationSystem.h"

#include "DeferredBuffer.h"
#include "ModifiedRecorder.h"
#include "GameObjectDependenciesRecorder.h"


NAMESPACE_BEGIN

Scene::Scene() : m_eventDispatcher(this)
{
	auto runtime = Runtime::Get();
	m_input = runtime->GetInput();

	//m_stableValue = runtime->GetNextStableValue();
	SetupMainSystemIterationTasks();
	SetupMainSystemModificationTasks();
	SetupDeferLists();

	m_mainSystems[MainSystemInfo::RENDERING_ID] = mheap::New<RenderingSystem>(this);
	m_mainSystems[MainSystemInfo::PHYSICS_ID]	= mheap::New<PhysicsSystem>(this);
	m_mainSystems[MainSystemInfo::SCRIPTING_ID] = mheap::New<ScriptingSystem>(this);
	m_mainSystems[MainSystemInfo::ANIMATION_ID] = mheap::New<AnimationSystem>(this);
}

Scene::~Scene()
{
	/*for (auto& system : m_mainSystems)
	{
		if (system)
		{
			system->Finalize();
		}
	}*/

	/*for (auto& system : m_mainSystems)
	{
		if (system)
		{
			delete system;
		}
	}*/

	//int x = 3;
}

void Scene::BakeAllMainSystems()
{
}

void Scene::SetupMainSystemIterationTasks()
{
	for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
	{
		auto& param = m_taskParams[i];
		param.scene = this;
		param.mainSystemId = i;

		auto& iterationTask = m_mainSystemIterationTasks[i];
		iterationTask.Params() = &param;
		iterationTask.Entry() = [](void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(IterationTaskParam, p, scene, mainSystemId);

			auto& system = scene->m_mainSystems[mainSystemId];
			if (!system)
			{
				//scene->EndReconstructForMainSystem(mainSystemId);
				return;
			}

			scene->PerformModificationForMainSystem(mainSystemId);

			//scene->EndReconstructForMainSystem(mainSystemId);

			system->Iteration(scene->m_dt);
		};
	}

	if (StartupConfig::Get().isEnableGUIEditing)
	{
		m_mainProcessingSystemIterationTasks[m_numMainProcessingSystem++] = m_mainSystemIterationTasks[MainSystemInfo::RENDERING_ID];
	}
	else
	{
		// output systems
		m_mainOutputSystemIterationTasks[m_numMainOutputSystem++] = m_mainSystemIterationTasks[MainSystemInfo::RENDERING_ID];
	}

	// output systems
	//m_mainOutputSystemIterationTasks[m_numMainOutputSystem++] = m_mainSystemIterationTasks[MainSystemInfo::AUDIO_ID];

	// processing systems
	m_mainProcessingSystemIterationTasks[m_numMainProcessingSystem++]	= m_mainSystemIterationTasks[MainSystemInfo::PHYSICS_ID];
	m_mainProcessingSystemIterationTasks[m_numMainProcessingSystem++]	= m_mainSystemIterationTasks[MainSystemInfo::SCRIPTING_ID];
	m_mainProcessingSystemIterationTasks[m_numMainProcessingSystem++]	= m_mainSystemIterationTasks[MainSystemInfo::ANIMATION_ID];
}

void Scene::SetupMainSystemModificationTasks()
{
	for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
	{
		auto& param = m_taskParams[i];
		auto& iterationTask = m_mainSystemModificationTasks[i];
		iterationTask.Params() = &param;
		iterationTask.Entry() = [](void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(IterationTaskParam, p, scene, mainSystemId);

			auto& system = scene->m_mainSystems[mainSystemId];
			if (!system)
			{
				//scene->EndReconstructForMainSystem(mainSystemId);
				return;
			}
		};
	}
}

void Scene::SetupDeferLists()
{
	
}
void Scene::BeginIteration()
{
	m_iterationCount++;
	m_currentDeferBufferIdx = m_iterationCount % NUM_DEFER_LIST;
	m_prevDeferBufferIdx = (m_iterationCount + NUM_DEFER_LIST - 1) % NUM_DEFER_LIST;

	EventDispatcher()->Dispatch(EVENT::EVENT_BEGIN_ITERATION);
}

void Scene::EndIteration()
{
	SynchMainProcessingSystemForMainOutputSystems();

	//TaskSystem::WaitForHandle(&m_objectsModificationTaskWaitingHandle);

	EventDispatcher()->Dispatch(EVENT::EVENT_END_ITERATION);
}

void Scene::PerformModificationForMainSystem(ID id)
{
	auto& sys = m_mainSystems[id];
	if (!sys)
	{
		return;
	}

	auto& actions = sys->m_modificationActions;

	sys->BeginModification();

	for (auto& a : actions)
	{
		if (a.type == a.ADD)
		{
			sys->AddComponent(a.comp);
			a.comp->OnComponentAdded();
			a.comp->OnTransformChanged();
			continue;
		}

		if (a.type == a.REMOVE)
		{
			sys->RemoveComponent(a.comp);
			a.comp->OnComponentRemoved();
			continue;
		}

		if (a.type == a.MOVED)
		{
			sys->OnObjectTransformChanged(a.comp);
			a.comp->OnTransformChanged();
			continue;
		}
	}

	actions.clear();

	sys->EndModification();
}

void Scene::FlushAsyncTasksForMainSystem(ID id)
{
	auto& sys = m_mainSystems[id];
	if (!sys)
	{
		return;
	}

	sys->FlushAsyncTasks();
}

void Scene::SynchMainProcessingSystems()
{
	static TaskWaitingHandle handle = { 0,0 };

	if constexpr (Config::ENABLE_DEBUG_GRAPHICS)
		GetRenderingSystem()->RenderWithDebugGraphics();

	Task task;
	task.Entry() = [](void* p)
	{
		auto scene = (Scene*)p;
		scene->UpdateDeferredBuffers(scene->m_deferredBuffers2);
	};
	task.Params() = this;

	TaskSystem::PrepareHandle(&handle);
	TaskSystem::Submit(&handle, task, Task::CRITICAL);

	TaskSystem::WaitForHandle(&handle);
}

void Scene::SynchMainProcessingSystemForMainOutputSystems()
{
	static TaskWaitingHandle handle = { 0,0 };

	Task task;
	task.Entry() = [](void* p)
	{
		auto scene = (Scene*)p;
		scene->UpdateDeferredBuffers(scene->m_deferredBuffers1);
	};
	task.Params() = this;

	TaskSystem::PrepareHandle(&handle);
	TaskSystem::Submit(&handle, task, Task::CRITICAL);

	TaskSystem::WaitForHandle(&handle);
}

void Scene::UpdateDeferredBuffers(decltype(m_deferredBuffers1)& buffers)
{
	auto iteration = GetIterationCount();
	TaskUtils::ForEachConcurrentList(buffers,
		[iteration](DeferredBufferControlBlock* ctrlBlock, size_t)
		{
			ctrlBlock->Update(iteration);
		},
		TaskSystem::GetWorkerCount()
	);
	buffers.Clear();
}

void Scene::AddLongLifeObject(const Handle<GameObject>& obj, bool indexedName)
{
	obj->m_sceneId = m_longLifeObjects.size();
	obj->m_isLongLife = true;

	m_longLifeObjects.Push(obj);
}

void Scene::ResolveDependencies(GameObject* obj, GameObjectDependenciesRecorder* output)
{
	GameObjectDependenciesResolver* resolvers[MainSystemInfo::COUNT] = {};
	for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
	{
		if (m_mainSystems[i])
		{
			resolvers[i] = m_mainSystems[i]->GetDependenciesResolver();
		}
	}

	GameObjectDependenciesRecorder& recorder = *output;
	recorder.Record(obj);
	for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
	{
		auto resolver = resolvers[i];
		if (resolver)
		{
			for (auto& o : recorder.m_objects)
			{
				if (o->HasComponent(i))
				{
					resolver->Resolve(&recorder, o);
				}
			}
		}
	}
}

void Scene::AddObjectImpl(GameObject* obj, bool indexedName)
{
	obj->RecordAllComponetsAsModified();
	Runtime::Get()->GetModifiedRecorder()->RecordGameObject(obj, GameObject::ModifiedFlag::HEIRARCHY);

	//obj->m_scene = this;

	m_addedObjectInFrame.push_back(obj);
	EventDispatcher()->Dispatch(EVENT::EVENT_OBJECTS_ADDED, &m_addedObjectInFrame);
	m_addedObjectInFrame.clear();

	obj->ForceRefreshTransform(INVALID_ID - 1, true);

	obj->PreTraversal1([this](GameObject* o)
		{
			o->m_scene = this;
			Runtime::Get()->GetModifiedRecorder()->RecordGameObject(o, GameObject::ModifiedFlag::HEIRARCHY);
		}
	);

	if (m_isSettingUpLongLifeObjects)
	{
		AddLongLifeObject(obj, indexedName);
		return;
	}

	obj->m_sceneId = m_shortLifeObjects.size();
	obj->m_isLongLife = false;

	m_shortLifeObjects.Push(obj);
}

void Scene::AddObject(const Handle<GameObject>& obj, bool indexedName)
{
	GameObjectDependenciesRecorder recorder = this;
	ResolveDependencies(obj, &recorder);

	for (auto& root : recorder.m_rootObjects)
	{
		AddObjectImpl(root, indexedName);
	}
}

void Scene::RemoveObjectImpl(GameObject* obj)
{
	obj->RecordAllComponetsAsModified();
	Runtime::Get()->GetModifiedRecorder()->RecordGameObject(obj, GameObject::ModifiedFlag::HEIRARCHY);

	m_removedObjectInFrame.push_back(obj);
	EventDispatcher()->Dispatch(EVENT::EVENT_OBJECTS_REMOVED, &m_removedObjectInFrame);
	m_removedObjectInFrame.clear();

	obj->PreTraversal1([this](GameObject* o)
		{
			o->m_scene = nullptr;
			Runtime::Get()->GetModifiedRecorder()->RecordGameObject(o, GameObject::ModifiedFlag::HEIRARCHY);
		}
	);

	if (obj->m_isLongLife)
	{
		MANAGED_ARRAY_ROLL_TO_FILL_BLANK(m_longLifeObjects, obj, m_sceneId);
	}
	else
	{
		MANAGED_ARRAY_ROLL_TO_FILL_BLANK(m_shortLifeObjects, obj, m_sceneId);
	}

	obj->m_scene = nullptr;
	obj->m_sceneId = INVALID_ID;

	GetCurrentTrash().Push(obj);
}

void Scene::RemoveObject(const Handle<GameObject>& obj)
{
	GameObjectDependenciesRecorder recorder = this;
	ResolveDependencies(obj, &recorder);

	for (auto& root : recorder.m_rootObjects)
	{
		assert(root->m_scene == this);
		RemoveObjectImpl(root);
	}
}

Handle<GameObject> Scene::FindObjectByIndexedName(String name)
{
	return Handle<GameObject>();
}

bool Scene::BeginSetupLongLifeObject()
{
	//Runtime::Get()->NoneStableValueLock().lock();

	m_isSettingUpLongLifeObjects = true;

	mheap::internal::SetHeapId(mheap::internal::HEAP_ID::STABLE_HEAP);

	for (auto& system : m_mainSystems)
	{
		if (system)
		{
			system->BeginModification();
		}
	}

	EventDispatcher()->Dispatch(EVENT::EVENT_SETUP_LONGLIFE_OBJECTS);

	return true;
}

void Scene::EndSetupLongLifeObject()
{
	mheap::internal::SetHeapId(mheap::internal::HEAP_ID::GC_HEAP);

	byte resetValues[2] = { MARK_COLOR::WHITE, MARK_COLOR::BLACK };
	gc::PerformFullSystemGC(255, resetValues);

	for (auto& system : m_mainSystems)
	{
		if (system)
		{
			system->EndModification();
		}
	}

	BeginIteration();
	EndIteration();

	m_isSettingUpLongLifeObjects = false;
}

void Scene::Iteration(float dt)
{
	GetCurrentTrash().clear();

	m_dt = dt;
	BeginIteration();

	//m_numMainSystemEndReconstruct.store(MainSystemInfo::COUNT, std::memory_order_relaxed);
	//TaskSystem::PrepareHandle(&m_endReconstructWaitingHandle);

	for (auto& sys : m_mainSystems)
	{
		if (sys)
		{
			sys->PrevIteration();
		}
	}

	Task tasks[2];
	tasks[0].Entry() = [](void* p)
	{
		auto scene = (Scene*)p;
		if (scene->m_numMainOutputSystem)
		{
			TaskSystem::SubmitAndWait(scene->m_mainOutputSystemIterationTasks, scene->m_numMainOutputSystem, Task::CRITICAL);
		}
	};
	tasks[0].Params() = this;

	tasks[1].Entry() = [](void* p)
	{
		auto scene = (Scene*)p;
		if (scene->m_numMainProcessingSystem)
		{
			TaskSystem::SubmitAndWait(scene->m_mainProcessingSystemIterationTasks, scene->m_numMainProcessingSystem, Task::CRITICAL);
			scene->SynchMainProcessingSystems();
		}
	};
	tasks[1].Params() = this;
	
	TaskSystem::SubmitAndWait(tasks, 2, Task::CRITICAL);

	//TaskSystem::WaitForHandle(&m_endReconstructWaitingHandle);

	//std::cout << "Scene::Iteration\n";

	for (auto& sys : m_mainSystems)
	{
		if (sys)
		{
			sys->PostIteration();
		}
	}

	EndIteration();
}

void Scene::BeginRunning()
{
	EventDispatcher()->Dispatch(EVENT::EVENT_BEGIN_RUNNING);
}

void Scene::EndRunning()
{
	EventDispatcher()->Dispatch(EVENT::EVENT_END_RUNNING);
}

void Scene::CleanUp()
{
	m_longLifeObjects.clear();
	m_shortLifeObjects.clear();

	for (auto& list : m_trashObjects)
	{
		list.clear();
	}

	byte resetValues[2] = { MARK_COLOR::WHITE, MARK_COLOR::BLACK };
	gc::PerformFullSystemGC(255, resetValues);

	for (auto& system : m_mainSystems)
	{
		if (system)
		{
			system->Finalize();
		}
	}

	std::cout << "Scene::CleanUp()\n";
}

void Scene::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void Scene::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void Scene::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void Scene::SerializeToJson(Serializer* serializer, json& j) const
{
	{
		auto arr = json::array();
		for (auto& o : m_longLifeObjects)
		{
			arr.push_back(serializer->Serialize(o));
		}
		j["LongLifeObjects"] = arr;
	}

	{
		auto arr = json::array();
		for (auto& o : m_shortLifeObjects)
		{
			arr.push_back(serializer->Serialize(o));
		}
		j["ShortLifeObjects"] = arr;
	}
}

void Scene::DeserializeFromJson(Serializer* serializer, const json& j)
{
	auto self = Runtime::Get()->CreateScene(this);
	{
		if (BeginSetupLongLifeObject())
		{
			Handle<GameObject> obj;
			auto& arr = j["LongLifeObjects"];
			for (auto& j1 : arr)
			{
				serializer->Deserialize(j1, obj);
				AddObject(obj);
			}

			EndSetupLongLifeObject();
		}
	}

	{
		Handle<GameObject> obj;
		auto& arr = j["ShortLifeObjects"];
		for (auto& j1 : arr)
		{
			serializer->Deserialize(j1, obj);
			AddObject(obj);
		}
	}
}

Handle<ClassMetadata> Scene::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void Scene::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END