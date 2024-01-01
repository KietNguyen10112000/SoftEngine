#include "Scene.h"

#include "Runtime.h"
#include "GameObject.h"

#include "MainSystem/MainSystem.h"
#include "MainSystem/Rendering/RenderingSystem.h"
#include "MainSystem/Scripting/ScriptingSystem.h"
#include "MainSystem/Animation/AnimationSystem.h"

#include "DeferredBuffer.h"


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
	m_mainSystems[MainSystemInfo::ANIMATION_ID] = new AnimationSystem(this);

	for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
	{
		auto system = m_mainSystems[i];
		if (system)
		{
			system->m_handleKeeper = &m_handleKeepers[i * 2];

			for (size_t j = 0; j < NUM_DEFER_LIST; j++)
			{
				system->m_handleKeeper[j].ReserveNoSafe(8 * KB);
			}
		}
	}
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

			system->m_handleKeeper[scene->GetPrevDeferBufferIdx()].Clear();
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

	decltype(m_addList)* listss[] = { &m_addList, &m_removeList, &m_changedTransformList, &m_stagedChangeTransformList };

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

		auto& comps = comp->GetGameObject()->m_mainComponents;
		if (comp != comps[mainSystemId].Get())
		{
			comps[mainSystemId] = comp;
		}

		comp->OnTransformChanged();
		system->AddComponent(comp);
		comp->OnComponentAdded();
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

		auto& comps = comp->GetGameObject()->m_mainComponents;
		if (comp == comps[mainSystemId].Get())
		{
			comps[mainSystemId] = nullptr;
		}

		comp->OnComponentRemoved();
		system->RemoveComponent(comp);
	}
}

void Scene::OnObjectTransformChanged(GameObject* obj)
{
	auto root = obj->m_root;

	if (!root)
	{
		return;
	}

	auto& atomicVar = root->m_updatedTransformIteration;

	auto& atomicVar1 = obj->m_isRecoredChangeTransformIteration;

	ATOMIC_EXCHANGE_ONCE(atomicVar1, m_iterationCount);

	ATOMIC_EXCHANGE_ONCE(atomicVar, m_iterationCount);

	assert(root->ParentUpToDate().Get() == nullptr);

	GetCurrentChangedTransformList().Add(root);
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
		if (obj->m_modificationStateScene != MODIFICATION_STATE::ADDING)
		{
			obj = nullptr;
			continue;
		}

		destList.push_back(obj);

		obj->m_modificationStateScene = MODIFICATION_STATE::NONE;
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

		obj->PostTraversalUpToDate(
			[scene](GameObject* cur)
			{
				cur->m_scene = scene;

				if (cur->m_UID == INVALID_ID)
					cur->m_UID = scene->m_UIDCounter++;
			}
		);

		//obj->RecalculateUpToDateTransform();
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
		if (obj->m_modificationStateScene != MODIFICATION_STATE::REMOVING)
		{
			obj = nullptr;
			continue;
		}

		destList.push_back(obj);

		obj->m_modificationStateScene = MODIFICATION_STATE::NONE;
		
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

		if (obj->Parent().Get() == nullptr)
		{
			obj->PostTraversalUpToDate(
				[](GameObject* cur)
				{
					cur->m_scene = nullptr;
					cur->m_sceneId = INVALID_ID;
			//cur->m_UID = INVALID_ID;
				}
			);
		}
		
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

	/*Task task;
	task.Entry() = [](void* p)
	{
		auto scene = (Scene*)p;
		scene->FilterAddList();
		scene->FilterRemoveList();
	};
	task.Params() = this;*/

	FilterAddList();
	FilterRemoveList();

	//TaskSystem::PrepareHandle(&m_objectsModificationTaskWaitingHandle);
	//TaskSystem::Submit(&m_objectsModificationTaskWaitingHandle, task, Task::CRITICAL);

	EventDispatcher()->Dispatch(EVENT::EVENT_BEGIN_ITERATION);
}

void Scene::EndIteration()
{
	SynchMainProcessingSystemForMainOutputSystems();

	//TaskSystem::WaitForHandle(&m_objectsModificationTaskWaitingHandle);

	EventDispatcher()->Dispatch(EVENT::EVENT_END_ITERATION);
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

	StageAllChangedTransformObjects();

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

	StageAllChangedTreeStruct();

	auto& list = GetCurrentStagedChangeTransformList();
	for (auto& obj : list)
	{
		if (obj->m_isRecoredChangeTransformIteration.load(std::memory_order_relaxed) == m_iterationCount)
			obj->UpdateTransformReadWrite();
	}

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

void Scene::StageAllChangedTransformObjects()
{
	static TaskWaitingHandle handle = { 0,0 };

	auto& destList = GetCurrentStagedChangeTransformList();
	destList.Clear();
	auto& list = GetCurrentChangedTransformList();

	TaskSystem::PrepareHandle(&handle);

	TaskUtils::ForEachConcurrentList(
		list, 
		[&](GameObject* obj, ID) 
		{
			/*if (obj->m_updatedTransformIteration == m_iterationCount)
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

				auto parent = root->ParentUpToDate().Get();
				if (!parent)
				{
					break;
				}

				root = parent;
			}

			if (!nearestRootChangedTransform)
			{
				continue;
			}*/

			auto nearestRootChangedTransform = obj;

			//assert(obj == obj->m_root);
			if (obj != obj->m_root)
			{
				//assert(obj->m_root == nullptr);
				return;
			}

			Task recalculateTransformTask;
			recalculateTransformTask.Entry() = [](void* p)
			{
				auto gameObject = (GameObject*)p;
				//auto parent = gameObject->ParentUpToDate().Get();
				gameObject->RecalculateUpToDateTransformBegin(INVALID_ID);
			};
			recalculateTransformTask.Params() = nearestRootChangedTransform;

			TaskSystem::Submit(&handle, recalculateTransformTask, Task::CRITICAL);

			nearestRootChangedTransform->PreTraversal1UpToDate(
				[&](GameObject* cur)
				{
					/*if (cur->m_updatedTransformIteration == m_iterationCount)
					{
						return true;
					}

					cur->m_updatedTransformIteration = m_iterationCount;*/

					//if (cur->m_isRecoredChangeTransformIteration.load(std::memory_order_relaxed) == m_iterationCount)
					{
						destList.Add(cur);
					}

					//destList.push_back(cur);
					//return false;
				}
			);
		},
		std::max(TaskSystem::GetWorkerCount() / 2, (size_t)4)
	);

	TaskSystem::WaitForHandle(&handle);
}

void Scene::StageAllChangedTreeStruct()
{
	for (auto& obj : m_changedTreeStructList)
	{
		obj->UpdateTreeBuffer();

		if (obj->m_modificationStateTree == MODIFICATION_STATE::ADDING)
		{
			obj->PostTraversalUpToDate(
				[&](GameObject* cur)
				{
					//cur->m_scene = this;
					if (cur->m_UID == INVALID_ID)
						cur->m_UID = m_UIDCounter++;
				}
			);

			obj->m_modificationStateTree = MODIFICATION_STATE::NONE;
		}

		if (obj->m_modificationStateTree == MODIFICATION_STATE::REMOVING)
		{
			obj->PostTraversalUpToDate(
				[&](GameObject* cur)
				{
					cur->m_scene = nullptr;
					//cur->m_UID = INVALID_ID;
				}
			);

			obj->m_modificationStateTree = MODIFICATION_STATE::NONE;
		}
	}
	m_changedTreeStructList.Clear();
}

void Scene::AddLongLifeObject(const Handle<GameObject>& obj, bool indexedName)
{
	obj->m_modificationLock.lock();

	obj->m_sceneId = m_longLifeObjects.size();
	obj->m_isLongLife = true;

	/*mheap::internal::SetStableValue(Runtime::NONE_STABLE_VALUE);
	m_longLifeObjects.Push(obj);
	mheap::internal::SetStableValue(m_stableValue);*/

	obj->PreTraversal1UpToDate(
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
	GetCurrrentComponentsHolderList().Add(component);
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
	GetCurrrentComponentsHolderList().Add(component);
}

void Scene::DoAddToParent(GameObject* parent, const Handle<GameObject>& child)
{
	assert(parent->IsInAnyScene() && parent->GetScene() == this);

#ifdef _DEBUG
	if (child->IsInAnyScene())
	{
		assert(child->m_modificationStateScene == MODIFICATION_STATE::REMOVING || child->m_modificationStateTree == MODIFICATION_STATE::REMOVING);
	}
#endif // _DEBUG

	child->m_modificationStateTree = MODIFICATION_STATE::ADDING;
	
	child->PreTraversal1UpToDate(
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

			cur->m_scene = this;
		}
	);

	if (parent->m_lastChangeTreeIterationCount != GetIterationCount())
	{
		m_changedTreeStructList.Add(parent);
	}

	if (child->m_lastChangeTreeIterationCount != GetIterationCount())
	{
		m_changedTreeStructList.Add(child);
	}
	
	GetCurrrentObjectsHolderList().Add(child);
}

void Scene::DoRemoveFromParent(GameObject* parent, const Handle<GameObject>& child)
{
	assert(parent->IsInAnyScene() && parent->GetScene() == this);

	child->m_modificationStateTree = MODIFICATION_STATE::REMOVING;
	child->PreTraversal1UpToDate(
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

	if (parent->m_lastChangeTreeIterationCount != GetIterationCount())
	{
		m_changedTreeStructList.Add(parent);
	}

	if (child->m_lastChangeTreeIterationCount != GetIterationCount())
	{
		m_changedTreeStructList.Add(child);
	}

	GetCurrrentObjectsHolderList().Add(child);
}

void Scene::AddObject(const Handle<GameObject>& obj, bool indexedName)
{
	assert(obj->ParentUpToDate().Get() == nullptr);

	if (m_isSettingUpLongLifeObjects)
	{
		AddLongLifeObject(obj, indexedName);
		return;
	}

	obj->m_modificationLock.lock();

#ifdef _DEBUG
	if (obj->m_scene != nullptr || obj->m_sceneId == INVALID_ID)
	{
		assert(obj->m_modificationStateScene == MODIFICATION_STATE::REMOVING
			|| obj->m_modificationStateScene == MODIFICATION_STATE::NONE
		);
	}
#endif // _DEBUG

	if (obj->m_scene == this && obj->m_modificationStateScene == MODIFICATION_STATE::REMOVING)
	{
		obj->m_modificationStateScene = MODIFICATION_STATE::NONE;
		goto Return;
	}

	if (obj->m_scene != nullptr && obj->m_scene != this)
	{
		// cross the scenes, haven't implemented yet
		assert(0);
	}

	obj->m_scene = this;
	obj->m_modificationStateScene = MODIFICATION_STATE::ADDING;

	if (indexedName)
	{
		obj->m_indexedName = obj->m_name;
	}

	GetCurrrentObjectsHolderList().Add(obj);
	GetCurrentAddList().Add(obj);

Return:
	obj->PreTraversal1UpToDate(
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
	assert(obj->ParentUpToDate().Get() == nullptr);
	assert(obj->m_scene == this);

	obj->m_modificationLock.lock();

#ifdef _DEBUG
	assert(obj->m_modificationStateScene == MODIFICATION_STATE::ADDING || obj->m_modificationStateScene == MODIFICATION_STATE::NONE);
#endif // _DEBUG

	if (obj->m_modificationStateScene == MODIFICATION_STATE::ADDING)
	{
		obj->m_modificationStateScene = MODIFICATION_STATE::NONE;
		goto Return;
	}

	obj->m_modificationStateScene = MODIFICATION_STATE::REMOVING;

	GetCurrrentObjectsHolderList().Add(obj);
	GetCurrentRemoveList().Add(obj);

Return:
	obj->PostTraversalUpToDate(
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
	//Runtime::Get()->NoneStableValueLock().lock();

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

	byte resetValues[2] = { MARK_COLOR::WHITE, MARK_COLOR::BLACK };
	gc::PerformFullSystemGC(255, resetValues);

	/*if (m_longLifeObjects.size() != 0)
	{
		mheap::internal::ChangeStableValue(m_stableValue, ((ManagedHandle*)m_longLifeObjects.data()) - 1);
		mheap::internal::FreeStableObjects(Runtime::NONE_STABLE_VALUE, 0, 0);
	}*/
	

	//Runtime::Get()->NoneStableValueLock().unlock();

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
		list.Clear();
	}

	mheap::internal::FreeStableObjects(m_stableValue, 0, 0);
	for (size_t i = 0; i < 5; i++)
	{
		gc::Run(-1);
	}

	std::cout << "Scene::CleanUp()\n";
}

NAMESPACE_END