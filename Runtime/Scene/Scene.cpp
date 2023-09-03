#include "Scene.h"

#include "Runtime.h"
#include "GameObject.h"
#include "MainSystem/MainSystem.h"


NAMESPACE_BEGIN

Scene::Scene(Runtime* runtime)
{
	m_stableValue = runtime->GetNextStableValue();
	SetupNotifyTasks();
}

Scene::~Scene()
{
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

void Scene::SetupNotifyTasks()
{
	for (size_t i = 0; i < MainSystemInfo::COUNT; i++)
	{
		auto& param = m_taskParams[i];
		param.scene = this;
		param.mainSystemId = i;

		// add task
		auto& addTask = m_notifyAddListTasks[i];
		addTask.Params() = &param;
		addTask.Entry() = [](void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(NotifyTaskParam, p, scene, mainSystemId);

			auto& list = scene->m_addList;
			auto system = scene->m_mainSystems[mainSystemId];
			for (auto& obj : list)
			{
				obj->PreTraversal(
					[system, mainSystemId](GameObject* obj)
					{
						auto& comp = obj->m_mainComponents[mainSystemId];
						if (comp)
						{
							system->AddComponent(comp);
						}
					}
				);
			}
		};

		// remove task
		auto& removeTask = m_notifyRemoveListTasks[i];
		removeTask.Params() = &param;
		removeTask.Entry() = [](void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(NotifyTaskParam, p, scene, mainSystemId);

			auto& list = scene->m_removeList;
			auto& system = scene->m_mainSystems[mainSystemId];
			for (auto& obj : list)
			{
				obj->PostTraversal(
					[system, mainSystemId](GameObject* obj)
					{
						auto& comp = obj->m_mainComponents[mainSystemId];
						if (comp)
						{
							system->RemoveComponent(comp);
						}
					}
				);
			}
		};

		// change transform
		auto& changedTransformTask = m_notifyChangedTransformListTasks[i];
		changedTransformTask.Params() = &param;
		changedTransformTask.Entry() = [](void* p)
		{
			TASK_SYSTEM_UNPACK_PARAM_2(NotifyTaskParam, p, scene, mainSystemId);

			auto& list = scene->m_changedTransformList;
			auto& system = scene->m_mainSystems[mainSystemId];
			for (auto& obj : list)
			{
				obj->PreTraversal(
					[system, mainSystemId](GameObject* obj)
					{
						auto& comp = obj->m_mainComponents[mainSystemId];
						if (comp)
						{
							system->OnObjectTransformChanged(comp);
						}
					}
				);
			}
		};
	}
}

void Scene::NotifyAddObjectListForMainSystem()
{
	TaskSystem::SubmitAndWait(m_notifyAddListTasks, MainSystemInfo::COUNT, Task::CRITICAL);
	m_addList.clear();
}

void Scene::NotifyRemoveObjectListForMainSystem()
{
	TaskSystem::SubmitAndWait(m_notifyRemoveListTasks, MainSystemInfo::COUNT, Task::CRITICAL);
	m_removeList.clear();
	GetCurrentTrash().Clear();
}

void Scene::OnObjectTransformChanged(GameObject* obj)
{
	if (obj->m_isChangedTransform)
	{
		return;
	}

	obj->m_isChangedTransform = true;
	m_changedTransformList.push_back(obj);
}

void Scene::NotifyChangedTransformListForMainSystem()
{
	TaskSystem::SubmitAndWait(m_notifyChangedTransformListTasks, MainSystemInfo::COUNT, Task::CRITICAL);
	m_changedTransformList.clear();
}

void Scene::AddObject(const Handle<GameObject>& obj, bool indexedName)
{
	if (obj->m_scene != nullptr)
	{
		return;
	}

	if (indexedName)
	{
		obj->m_indexedName = obj->m_name;
		// do add indexing
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

	m_addList.push_back(obj);
}

void Scene::RemoveObject(const Handle<GameObject>& obj)
{
	if (obj->m_scene != this)
	{
		return;
	}

	if (obj->m_isLongLife)
	{
		MANAGED_ARRAY_ROLL_TO_FILL_BLANK(m_longLifeObjects, obj, m_sceneId);
	}
	else
	{
		MANAGED_ARRAY_ROLL_TO_FILL_BLANK(m_shortLifeObjects, obj, m_sceneId);
		GetCurrentTrash().Push(obj);
	}

	if (!obj->m_indexedName.empty())
	{
		// do remove indexing
	}

	m_removeList.push_back(obj);
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
	m_isSettingUpLongLifeObjects = false;
	mheap::internal::SetStableValue(m_oldStableValue);
}

void Scene::Iteration(float dt)
{


}

NAMESPACE_END