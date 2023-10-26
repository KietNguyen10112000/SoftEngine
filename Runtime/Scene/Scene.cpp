#include "Scene.h"

#include "Runtime.h"
#include "GameObject.h"

#include "MainSystem/MainSystem.h"
#include "MainSystem/Rendering/RenderingSystem.h"
#include "MainSystem/Scripting/ScriptingSystem.h"


NAMESPACE_BEGIN

Scene::Scene(Runtime* runtime)
{
	m_input = runtime->GetInput();

	m_stableValue = runtime->GetNextStableValue();
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

	// output systems
	m_mainOutputSystemIterationTasks[m_numMainOutputSystem++]			= m_mainSystemIterationTasks[MainSystemInfo::RENDERING_ID];

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
	for (auto& list : m_addList)
	{
		list.ReserveNoSafe(8 * KB);
	}

	for (auto& list : m_removeList)
	{
		list.ReserveNoSafe(8 * KB);
	}

	for (auto& list : m_changedTransformList)
	{
		list.ReserveNoSafe(8 * KB);
	}
}

void Scene::ProcessAddObjectListForMainSystem(ID mainSystemId)
{
	auto& list = GetPrevAddList();
	auto system = m_mainSystems[mainSystemId];
	for (auto& obj : list)
	{
		obj->PreTraversal(
			[system, mainSystemId](GameObject* obj)
			{
				auto& comp = obj->m_mainComponents[mainSystemId];
				if (comp)
				{
					system->AddComponent(comp);
					comp->OnComponentAdded();
					comp->OnTransformChanged();
				}

				return false;
			}
		);
	}
}

void Scene::ProcessRemoveObjectListForMainSystem(ID mainSystemId)
{
	auto& list = GetPrevRemoveList();
	auto& system = m_mainSystems[mainSystemId];
	for (auto& obj : list)
	{
		obj->PostTraversal(
			[system, mainSystemId](GameObject* obj)
			{
				auto& comp = obj->m_mainComponents[mainSystemId];
				if (comp)
				{
					comp->OnComponentRemoved();
					system->RemoveComponent(comp);
				}
			}
		);
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
	auto scene = this;
	auto& list = GetCurrentAddList();
	for (auto& obj : list)
	{
		if (obj->m_modificationState != MODIFICATION_STATE::ADDING)
		{
			obj = nullptr;
			continue;
		}

		obj->m_modificationState = MODIFICATION_STATE::NONE;
		if (!obj->m_indexedName.empty())
		{
			// implement indexing
			assert(0);
		}

		if (m_isSettingUpLongLifeObjects)
		{
			obj->m_sceneId = m_longLifeObjects.size();
			obj->m_isLongLife = true;
			m_longLifeObjects.Push(obj);
		}
		else
		{
			obj->m_sceneId = m_shortLifeObjects.size();
			obj->m_isLongLife = false;
			m_shortLifeObjects.Push(obj);
		}

		obj->PostTraversal(
			[scene](GameObject* cur)
			{
				cur->m_scene = scene;
			}
		);
	}

	m_addListHolder.Clear();
}

void Scene::FilterRemoveList()
{
	auto& list = GetCurrentAddList();
	for (auto& obj : list)
	{
		if (obj->m_modificationState != MODIFICATION_STATE::REMOVING)
		{
			continue;
		}

		obj->m_modificationState = MODIFICATION_STATE::NONE;
		
		if (obj->m_isLongLife)
		{
			MANAGED_ARRAY_ROLL_TO_FILL_BLANK(m_longLifeObjects, obj, m_sceneId);
		}
		else
		{
			MANAGED_ARRAY_ROLL_TO_FILL_BLANK(m_shortLifeObjects, obj, m_sceneId);
			GetCurrentTrash().Push(obj);
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

	m_removeListHolder.Clear();
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
	{
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
	}

	m_iterationCount++;
	GetCurrentAddList().Clear();
	GetCurrentRemoveList().Clear();
	GetCurrentChangedTransformList().Clear();
	GetCurrentTrash().clear();
}

void Scene::EndIteration()
{
	SynchMainProcessingSystemForMainOutputSystems();
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

void Scene::AddObject(const Handle<GameObject>& obj, bool indexedName)
{
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

	m_addListHolder.Add(obj);
	GetCurrentAddList().Add(obj);

Return:
	obj->m_modificationLock.unlock();
}

void Scene::RemoveObject(const Handle<GameObject>& obj)
{
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

	m_removeListHolder.Add(obj);
	GetCurrentRemoveList().Add(obj);

Return:
	obj->m_modificationLock.unlock();
}

Handle<GameObject> Scene::FindObjectByIndexedName(String name)
{
	return Handle<GameObject>();
}

bool Scene::BeginSetupLongLifeObject()
{
	m_isSettingUpLongLifeObjects = true;

	m_oldStableValue = mheap::internal::GetStableValue();
	mheap::internal::SetStableValue(m_stableValue);

	return true;
}

void Scene::EndSetupLongLifeObject()
{
	mheap::internal::SetStableValue(m_oldStableValue);

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
		TaskSystem::SubmitAndWait(scene->m_mainOutputSystemIterationTasks, scene->m_numMainOutputSystem, Task::CRITICAL);
	};
	tasks[0].Params() = this;

	tasks[1].Entry() = [](void* p)
	{
		auto scene = (Scene*)p;
		TaskSystem::SubmitAndWait(scene->m_mainProcessingSystemIterationTasks, scene->m_numMainProcessingSystem, Task::CRITICAL);
		scene->SynchMainProcessingSystems();
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

NAMESPACE_END