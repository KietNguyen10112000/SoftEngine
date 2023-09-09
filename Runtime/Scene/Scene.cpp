#include "Scene.h"

#include "Runtime.h"
#include "GameObject.h"
#include "MainSystem/MainSystem.h"
#include "MainSystem/Rendering/RenderingSystem.h"


NAMESPACE_BEGIN

Scene::Scene(Runtime* runtime)
{
	m_stableValue = runtime->GetNextStableValue();
	SetupMainSystemIterationTasks();

	m_mainSystems[MainSystemInfo::RENDERING_ID] = new RenderingSystem(this);
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

			scene->ProcessAddObjectListForMainSystem(mainSystemId);
			scene->ProcessChangedTransformListForMainSystem(mainSystemId);
			scene->ProcessRemoveObjectListForMainSystem(mainSystemId);

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
				}
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
	if (obj->m_isChangedTransform)
	{
		return;
	}

	obj->m_isChangedTransform = true;
	GetCurrentChangedTransformList().push_back(obj);
}

void Scene::ProcessChangedTransformListForMainSystem(ID mainSystemId)
{
	auto& list = GetPrevChangedTransformList();
	auto& system = m_mainSystems[mainSystemId];
	for (auto& obj : list)
	{
		obj->PreTraversal(
			[system, mainSystemId](GameObject* obj)
			{
				auto& comp = obj->m_mainComponents[mainSystemId];
				if (comp)
				{
					comp->OnTransformChanged();
					system->OnObjectTransformChanged(comp);
				}
			}
		);
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

void Scene::AddObject(const Handle<GameObject>& obj, bool indexedName)
{
	if (obj->m_scene != nullptr || obj->m_sceneId != INVALID_ID)
	{
		return;
	}

	obj->m_scene = this;

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

	GetCurrentAddList().push_back(obj);
}

void Scene::RemoveObject(const Handle<GameObject>& obj)
{
	if (obj->m_scene != this || obj->m_sceneId == INVALID_ID)
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

	//obj->m_scene = nullptr;
	obj->m_sceneId = INVALID_ID;

	if (!obj->m_indexedName.empty())
	{
		// do remove indexing
	}

	GetCurrentRemoveList().push_back(obj);
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
	m_iterationCount++;
	GetCurrentAddList().clear();
	GetCurrentRemoveList().clear();
	GetCurrentChangedTransformList().clear();
	GetCurrentTrash().clear();

	//m_numMainSystemEndReconstruct.store(MainSystemInfo::COUNT, std::memory_order_relaxed);
	//TaskSystem::PrepareHandle(&m_endReconstructWaitingHandle);
	
	TaskSystem::SubmitAndWait(m_mainSystemIterationTasks, MainSystemInfo::COUNT, Task::CRITICAL);

	//TaskSystem::WaitForHandle(&m_endReconstructWaitingHandle);

	std::cout << "Scene::Iteration\n";
}

NAMESPACE_END