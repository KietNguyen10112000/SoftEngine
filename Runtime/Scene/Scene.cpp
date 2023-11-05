#include "Scene.h"

#include "Runtime.h"
#include "GameObject.h"

#include "MainSystem/MainSystem.h"
#include "MainSystem/Rendering/RenderingSystem.h"
#include "MainSystem/Scripting/ScriptingSystem.h"


NAMESPACE_BEGIN

Scene::Scene() : m_eventDispatcher(this)
{
	auto runtime = Runtime::Get();
	m_input = runtime->GetInput();

	//m_stableValue = runtime->GetNextStableValue();
	SetupMainSystemIterationTasks();
	SetupMainSystemModificationTasks();
	SetupDeferLists();

	m_mainSystems[MainSystemInfo::RENDERING_ID] = new RenderingSystem(this);
	m_mainSystems[MainSystemInfo::SCRIPTING_ID] = new ScriptingSystem(this);
}

Scene::~Scene()
{
	for (auto& system : m_mainSystems)
	{
		if (system)
		{
			system->Finalize();
		}
	}

	for (auto& system : m_mainSystems)
	{
		if (system)
		{
			delete system;
		}
	}
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

			auto system = scene->m_mainSystems[mainSystemId];
			if (!system)
			{
				//scene->EndReconstructForMainSystem(mainSystemId);
				return;
			}

			scene->ProcessModificationForMainSystem(mainSystemId);

			//scene->EndReconstructForMainSystem(mainSystemId);

			system->Iteration(scene->m_dt);
		};
	}

	m_endReconstructTask.Params() = this;
	m_endReconstructTask.Entry() = [](void* p)
	{
		auto scene = (Scene*)p;
		scene->EndReconstructForAllMainSystems();
	};

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
	//m_mainProcessingSystemIterationTasks[m_numMainProcessingSystem++]	= m_mainSystemIterationTasks[MainSystemInfo::PHYSICS_ID];
	m_mainProcessingSystemIterationTasks[m_numMainProcessingSystem++]	= m_mainSystemIterationTasks[MainSystemInfo::SCRIPTING_ID];
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

			auto system = scene->m_mainSystems[mainSystemId];
			if (!system)
			{
				//scene->EndReconstructForMainSystem(mainSystemId);
				return;
			}

			scene->ProcessModificationForMainSystem(mainSystemId);
		};
	}
}

void Scene::SetupDeferLists()
{
	constexpr size_t RESERVE_SIZE = 16 * KB;

	decltype(m_addList)* listss[] = { &m_addList, &m_removeList, &m_changedTransformList };

	for (auto& _lists : listss)
	{
		auto& lists = *_lists;
		for (auto& list : lists)
		{
			list.ReserveNoSafe(RESERVE_SIZE);
		}
	}

	for (auto& list : m_objectsHolder)
	{
		list.ReserveNoSafe(RESERVE_SIZE);
	}

	for (auto& list : m_componentsHolder)
	{
		list.ReserveNoSafe(RESERVE_SIZE);
	}
}

void Scene::ProcessAddObjectListForMainSystem(ID mainSystemId)
{
	auto& list = GetPrevComponentsAddList(mainSystemId); // GetPrevAddList();
	auto system = m_mainSystems[mainSystemId];
	for (auto& comp : list)
	{
		if (comp->m_modificationState != MODIFICATION_STATE::ADDING)
		{
			continue;
		}

		comp->m_modificationState = MODIFICATION_STATE::NONE;
		system->AddComponent(comp);
		comp->OnComponentAdded();
		comp->OnTransformChanged();
	}
}

void Scene::ProcessRemoveObjectListForMainSystem(ID mainSystemId)
{
	auto& list = GetPrevComponentsRemoveList(mainSystemId); // GetPrevRemoveList();
	auto& system = m_mainSystems[mainSystemId];
	for (auto& comp : list)
	{
		if (comp->m_modificationState != MODIFICATION_STATE::REMOVING)
		{
			continue;
		}

		comp->m_modificationState = MODIFICATION_STATE::NONE;
		comp->OnComponentRemoved();
		system->RemoveComponent(comp);
	}
}

void Scene::OnObjectTransformChanged(GameObject* obj)
{
	if (obj->m_isRecoredChangeTransformIteration.exchange(m_iterationCount, std::memory_order_acquire) == m_iterationCount)
	{
		return;
	}

	GetCurrentChangedTransformList().Add(obj);
}

void Scene::ProcessChangedTransformListForMainSystem(ID mainSystemId)
{
	auto& system = m_mainSystems[mainSystemId];
	auto& list = GetPrevStagedChangeTransformList();
	for (auto& obj : list)
	{
		auto& comp = obj->m_mainComponents[mainSystemId];
		if (comp)
		{
			comp->OnTransformChanged();
			system->OnObjectTransformChanged(comp);
		}
	}
}

void Scene::ProcessModificationForMainSystem(ID mainSystemId)
{
	auto system = m_mainSystems[mainSystemId];
	system->BeginModification();
	ProcessAddObjectListForMainSystem(mainSystemId);
	ProcessChangedTransformListForMainSystem(mainSystemId);
	ProcessRemoveObjectListForMainSystem(mainSystemId);
	system->EndModification();
}

void Scene::ProcessModificationForAllMainSystems()
{
	TaskSystem::SubmitAndWait(m_mainSystemModificationTasks, MainSystemInfo::COUNT, Task::CRITICAL);
}

void Scene::FilterAddList()
{
	//assert(m_isSettingUpLongLifeObjects == false);

	auto scene = this;
	auto& list = GetPrevAddList();
	auto& destList = m_filteredAddList;
	destList.clear();
	for (auto& obj : list)
	{
		if (obj->m_modificationState != MODIFICATION_STATE::ADDING)
		{
			obj = nullptr;
			continue;
		}

		destList.push_back(obj);

		obj->m_modificationState = MODIFICATION_STATE::NONE;
		if (!obj->m_indexedName.empty())
		{
			// implement indexing
			assert(0);
		}

		{
			obj->m_sceneId = m_shortLifeObjects.size();
			obj->m_isLongLife = false;
			m_shortLifeObjects.Push(obj);
		}

		obj->PostTraversal(
			[scene](GameObject* cur)
			{
				cur->m_scene = scene;
				cur->m_UID = scene->m_UIDCounter++;
			}
		);
	}

	if (m_filteredAddList.size() != 0)
	{
		EventDispatcher()->Dispatch(EVENT::EVENT_OBJECTS_ADDED, &m_filteredAddList);
	}
}

void Scene::FilterRemoveList()
{
	auto& list = GetPrevRemoveList();
	auto& destList = m_filteredRemoveList;
	destList.clear();

	auto& currentTrashes = GetCurrentTrash();
	currentTrashes.clear();

	for (auto& obj : list)
	{
		if (obj->m_modificationState != MODIFICATION_STATE::REMOVING)
		{
			obj = nullptr;
			continue;
		}

		destList.push_back(obj);

		obj->m_modificationState = MODIFICATION_STATE::NONE;
		
		if (obj->m_isLongLife)
		{
			MANAGED_ARRAY_ROLL_TO_FILL_BLANK(m_longLifeObjects, obj, m_sceneId);
		}
		else
		{
			MANAGED_ARRAY_ROLL_TO_FILL_BLANK(m_shortLifeObjects, obj, m_sceneId);
			currentTrashes.Push(obj);
		}

		//obj->m_sceneId = INVALID_ID;

		if (!obj->m_indexedName.empty())
		{
			// do remove indexing
		}

		obj->PostTraversal(
			[](GameObject* cur)
			{
				cur->m_sceneId = INVALID_ID;
			}
		);
	}

	if (m_filteredRemoveList.size() != 0)
	{
		EventDispatcher()->Dispatch(EVENT::EVENT_OBJECTS_REMOVED, &m_filteredRemoveList);
	}
}

void Scene::EndReconstructForMainSystem(ID mainSystemId)
{
	if (!((--m_numMainSystemEndReconstruct) == 0))
	{
		return;
	}

	TaskSystem::Submit(&m_endReconstructWaitingHandle, m_endReconstructTask, Task::CRITICAL);
}

void Scene::EndReconstructForAllMainSystems()
{
	auto& list = GetPrevRemoveList();
	for (auto& obj : list)
	{
		obj->m_scene = nullptr;
		obj->m_sceneId = INVALID_ID;
	}
}

void Scene::BeginIteration()
{
	/*{
		Task tasks[2];
		auto& filterAdd = tasks[0];
		filterAdd.Entry() = [](void* p)
		{
			auto scene = (Scene*)p;
			scene->FilterAddList();
		};
		filterAdd.Params() = this;

		auto& filterRemove = tasks[1];
		filterRemove.Entry() = [](void* p)
		{
			auto scene = (Scene*)p;
			scene->FilterRemoveList();
		};
		filterRemove.Params() = this;

		TaskSystem::SubmitAndWait(tasks, 2, Task::CRITICAL);
	}*/

	m_iterationCount++;
	m_currentDeferBufferIdx = m_iterationCount % NUM_DEFER_LIST;
	m_prevDeferBufferIdx = (m_iterationCount + NUM_DEFER_LIST - 1) % NUM_DEFER_LIST;

	GetCurrentAddList().Clear();
	GetCurrentRemoveList().Clear();
	GetCurrentChangedTransformList().Clear();

	for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
	{
		GetCurrrentComponentsAddList(i).Clear();
		GetCurrrentComponentsRemoveList(i).Clear();
	}

	GetCurrrentObjectsHolderList().Clear();
	GetCurrrentComponentsHolderList().Clear();

	//GetCurrentTrash().clear();

	Task task;
	task.Entry() = [](void* p)
	{
		auto scene = (Scene*)p;
		scene->FilterAddList();
		scene->FilterRemoveList();
	};
	task.Params() = this;

	TaskSystem::PrepareHandle(&m_objectsModificationTaskWaitingHandle);
	TaskSystem::Submit(&m_objectsModificationTaskWaitingHandle, task, Task::CRITICAL);

	EventDispatcher()->Dispatch(EVENT::EVENT_BEGIN_ITERATION);
}

void Scene::EndIteration()
{
	SynchMainProcessingSystemForMainOutputSystems();

	TaskSystem::WaitForHandle(&m_objectsModificationTaskWaitingHandle);

	EventDispatcher()->Dispatch(EVENT::EVENT_END_ITERATION);
}

void Scene::SynchMainProcessingSystems()
{
	StageAllChangedTransformObjects();
}

void Scene::SynchMainProcessingSystemForMainOutputSystems()
{
	auto& list = GetCurrentStagedChangeTransformList();
	for (auto& obj : list)
	{
		obj->UpdateTransformReadWrite();
	}
}

void Scene::StageAllChangedTransformObjects()
{
	static TaskWaitingHandle handle = { 0,0 };

	auto& destList = GetCurrentStagedChangeTransformList();
	destList.clear();
	auto& list = GetCurrentChangedTransformList();

	TaskSystem::PrepareHandle(&handle);
	for (auto& obj : list)
	{
		if (obj->m_updatedTransformIteration == m_iterationCount)
		{
			continue;
		}

		auto root = obj;
		GameObject* nearestRootChangedTransform = nullptr;
		while (true)
		{
			if (root->m_isRecoredChangeTransformIteration.load(std::memory_order_relaxed) == m_iterationCount)
			{
				nearestRootChangedTransform = root;
			}

			auto parent = root->Parent().Get();
			if (!parent)
			{
				break;
			}

			root = parent;
		}

		if (!nearestRootChangedTransform)
		{
			continue;
		}

		Task recalculateTransformTask;
		recalculateTransformTask.Entry() = [](void* p)
		{
			auto gameObject = (GameObject*)p;
			auto numTransformContributors = gameObject->m_numTransformContributors.load(std::memory_order_relaxed);
			gameObject->m_numTransformContributors.store(0, std::memory_order_relaxed);

			auto& contributors = gameObject->m_transformContributors;
			for (uint32_t i = 0; i < numTransformContributors; i++)
			{
				contributors[i].func(gameObject, contributors[i].comp);
			}

			gameObject->RecalculateWriteTransform();
		};
		recalculateTransformTask.Params() = nearestRootChangedTransform;

		TaskSystem::Submit(&handle, recalculateTransformTask, Task::CRITICAL);

		nearestRootChangedTransform->PreTraversal(
			[&](GameObject* cur)
			{
				if (cur->m_updatedTransformIteration == m_iterationCount)
				{
					return true;
				}

				cur->m_updatedTransformIteration = m_iterationCount;
				destList.push_back(cur);
				return false;
			}
		);
	}

	TaskSystem::WaitForHandle(&handle);
}

void Scene::AddLongLifeObject(const Handle<GameObject>& obj, bool indexedName)
{
	obj->m_modificationLock.lock();

	obj->m_sceneId = m_longLifeObjects.size();
	obj->m_isLongLife = true;

	mheap::internal::SetStableValue(Runtime::NONE_STABLE_VALUE);
	m_longLifeObjects.Push(obj);
	mheap::internal::SetStableValue(m_stableValue);

	obj->PreTraversal1(
		[&](GameObject* cur)
		{
			for (uint32_t i = 0; i < MainSystemInfo::COUNT; i++)
			{
				auto& comp = cur->m_mainComponents[i];
				if (comp.Get())
				{
					AddLongLifeComponent(i, comp);
				}
			}
		}
	);

	obj->m_modificationLock.unlock();
}

void Scene::AddLongLifeComponent(ID COMPONENT_ID, const Handle<MainComponent>& component)
{
	auto system = m_mainSystems[COMPONENT_ID];
	system->AddComponent(component.Get());
	component->OnComponentAdded();
	component->OnTransformChanged();
}

void Scene::AddComponent(ID COMPONENT_ID, const Handle<MainComponent>& component)
{
	assert(component->m_modificationState == MODIFICATION_STATE::NONE || component->m_modificationState == MODIFICATION_STATE::REMOVING);

	if (component->m_modificationState == MODIFICATION_STATE::REMOVING)
	{
		component->m_modificationState = MODIFICATION_STATE::NONE;
		return;
	}

	component->m_modificationState = MODIFICATION_STATE::ADDING;
	GetCurrrentComponentsAddList(COMPONENT_ID).Add(component);
}

void Scene::RemoveComponent(ID COMPONENT_ID, const Handle<MainComponent>& component)
{
	assert(component->m_modificationState == MODIFICATION_STATE::NONE || component->m_modificationState == MODIFICATION_STATE::ADDING);

	if (component->m_modificationState == MODIFICATION_STATE::ADDING)
	{
		component->m_modificationState = MODIFICATION_STATE::NONE;
		return;
	}

	component->m_modificationState = MODIFICATION_STATE::REMOVING;
	GetCurrrentComponentsRemoveList(COMPONENT_ID).Add(component);
}

void Scene::AddObject(const Handle<GameObject>& obj, bool indexedName)
{
	assert(obj->Parent().Get() == nullptr);

	if (m_isSettingUpLongLifeObjects)
	{
		AddLongLifeObject(obj, indexedName);
		return;
	}

	obj->m_modificationLock.lock();

#ifdef _DEBUG
	if (obj->m_scene != nullptr || obj->m_sceneId == INVALID_ID)
	{
		assert(obj->m_modificationState == MODIFICATION_STATE::REMOVING || obj->m_modificationState == MODIFICATION_STATE::NONE);
	}
#endif // _DEBUG

	if (obj->m_scene == this && obj->m_modificationState == MODIFICATION_STATE::REMOVING)
	{
		obj->m_modificationState = MODIFICATION_STATE::NONE;
		goto Return;
	}

	if (obj->m_scene != nullptr && obj->m_scene != this)
	{
		// cross the scenes, haven't implemented yet
		assert(0);
	}

	obj->m_scene = this;
	obj->m_modificationState = MODIFICATION_STATE::ADDING;

	if (indexedName)
	{
		obj->m_indexedName = obj->m_name;
	}

	GetCurrrentObjectsHolderList().Add(obj);
	GetCurrentAddList().Add(obj);

Return:
	obj->PreTraversal1(
		[&](GameObject* cur)
		{
			for (uint32_t i = 0; i < MainSystemInfo::COUNT; i++)
			{
				auto& comp = cur->m_mainComponents[i];
				if (comp.Get())
				{
					AddComponent(i, comp);
				}
			}
		}
	);

	obj->m_modificationLock.unlock();
}

void Scene::RemoveObject(const Handle<GameObject>& obj)
{
	assert(m_isSettingUpLongLifeObjects == false);
	assert(obj->Parent().Get() == nullptr);
	assert(obj->m_scene == this);

	obj->m_modificationLock.lock();

#ifdef _DEBUG
	assert(obj->m_modificationState == MODIFICATION_STATE::ADDING || obj->m_modificationState == MODIFICATION_STATE::NONE);
#endif // _DEBUG

	if (obj->m_modificationState == MODIFICATION_STATE::ADDING)
	{
		obj->m_modificationState = MODIFICATION_STATE::NONE;
		goto Return;
	}

	obj->m_modificationState = MODIFICATION_STATE::REMOVING;

	GetCurrrentObjectsHolderList().Add(obj);
	GetCurrentRemoveList().Add(obj);

Return:
	obj->PostTraversal(
		[&](GameObject* cur)
		{
			for (uint32_t i = 0; i < MainSystemInfo::COUNT; i++)
			{
				auto& comp = cur->m_mainComponents[i];
				if (comp.Get())
				{
					RemoveComponent(i, comp);
				}
			}
		}
	);

	obj->m_modificationLock.unlock();
}

Handle<GameObject> Scene::FindObjectByIndexedName(String name)
{
	return Handle<GameObject>();
}

bool Scene::BeginSetupLongLifeObject()
{
	Runtime::Get()->NoneStableValueLock().lock();

	m_isSettingUpLongLifeObjects = true;

	m_oldStableValue = mheap::internal::GetStableValue();
	mheap::internal::SetStableValue(m_stableValue);

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
	mheap::internal::SetStableValue(m_oldStableValue);

	if (m_longLifeObjects.size() != 0)
	{
		mheap::internal::ChangeStableValue(m_stableValue, ((ManagedHandle*)m_longLifeObjects.data()) - 1);
		mheap::internal::FreeStableObjects(Runtime::NONE_STABLE_VALUE, 0, 0);
	}
	

	Runtime::Get()->NoneStableValueLock().unlock();

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

void Scene::Serialize(Serializer* serializer)
{
}

void Scene::Deserialize(Serializer* serializer)
{
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

	decltype(m_addList)* listss[] = { &m_addList, &m_removeList, &m_changedTransformList };

	for (auto& _lists : listss)
	{
		auto& lists = *_lists;
		for (auto& list : lists)
		{
			list.Clear();
		}
	}

	for (auto& list : m_objectsHolder)
	{
		list.Clear();
	}

	for (auto& list : m_componentsHolder)
	{
		list.Clear();
	}

	//m_genericStorage.Clear();
	//m_eventDispatcher.Clear();

	for (auto& list : m_trashObjects)
	{
		list.clear();
	}

	for (auto& list : m_stagedChangeTransformList)
	{
		list.clear();
	}

	mheap::internal::FreeStableObjects(m_stableValue, 0, 0);
	for (size_t i = 0; i < 5; i++)
	{
		gc::Run(-1);
	}

	std::cout << "Scene::CleanUp()\n";
}

NAMESPACE_END