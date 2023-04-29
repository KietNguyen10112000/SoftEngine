#include "Scene.h"

#include "Core/Time/Clock.h"
#include "Core/TemplateUtils/TemplateUtils.h"

#include "Components/Rendering/Rendering.h"
#include "Components/Physics/Physics.h"
#include "Components/Script/Script.h"

#include "Event/BuiltinEventManager.h"
#include "Event/EventManager.h"

#include "TaskSystem/TaskSystem.h"
#include "TaskSystem/TaskUtils.h"

#include "Objects/QueryStructures/AABBQueryStructure.h"
#include "Objects/QueryStructures/DBVTQueryTree.h"

#include "SubSystems/Rendering/RenderingSystem.h"
#include "SubSystems/Physics/PhysicsSystem.h"
#include "SubSystems/Script/ScriptSystem.h"

#include "Components/Dummy.h"

#include "Engine.h"

NAMESPACE_BEGIN

Scene::Scene(Engine* engine)
{
	m_objectEventMgr = rheap::New<BuiltinEventManager>(this);
	m_eventMgr = mheap::New<EventManager>(512);

	m_staticObjsQueryStructure = rheap::New<DBVTQueryTree>();

	m_engine			= engine;
	m_input				= engine->GetInput();
}

void Scene::Setup()
{
	m_renderingSystem = rheap::New<RenderingSystem>(this);
	m_physicsSystem = rheap::New<PhysicsSystem>(this);
	m_scriptSystem = rheap::New<ScriptSystem>(this);

	m_subSystems[Rendering::COMPONENT_ID] = m_renderingSystem;
	m_subSystems[Physics::COMPONENT_ID] = m_physicsSystem;
	m_subSystems[Script::COMPONENT_ID] = m_scriptSystem;
}

void Scene::Dtor()
{
	if (m_renderingSystem)
	{
		rheap::Delete(m_renderingSystem);
		m_renderingSystem = nullptr;
	}

	if (m_physicsSystem)
	{
		rheap::Delete(m_physicsSystem);
		m_physicsSystem = nullptr;
	}
	
	if (m_scriptSystem)
	{
		rheap::Delete(m_scriptSystem);
		m_scriptSystem = nullptr;
	}
}

Scene::~Scene()
{
	rheap::Delete(m_objectEventMgr);
	rheap::Delete(m_staticObjsQueryStructure);
}

void Scene::AddObject(Handle<GameObject>& obj, bool isGhost)
{
	obj->RecalculateAABB();

	if (isGhost)
	{
		m_waitForAdd.Add(obj);
		obj->m_sceneId = GHOST_OBJECT_ID_MASK;
		return;
	}

	m_waitForAdd.Add(obj);

	auto physicsComp = obj->GetComponentRaw<Physics>();
	if (physicsComp)
	{
		// insert to AABBQueryStructure base on PhysicsComponent
		auto type = physicsComp->Type();

		switch (type)
		{
		case soft::Physics::STATIC:
			break;
		case soft::Physics::DYNAMIC:
		case soft::Physics::KINEMATIC:
			AddDynamicObject(obj.Get());
			break;
		default:
			break;
		}

		return;
	}

	AddDynamicObject(obj.Get());
}

void Scene::RemoveObject(Handle<GameObject>& obj)
{
	m_waitForRemove->Add(obj);

	auto physicsComp = obj->GetComponentRaw<Physics>();
	if (physicsComp)
	{
		auto type = physicsComp->Type();
		switch (type)
		{
		case soft::Physics::DYNAMIC:
		case soft::Physics::KINEMATIC:
			RemoveDynamicObject(obj.Get());
		}
	}
}

void Scene::RefreshObject(GameObject* obj)
{
	//static dummy::SubSystemComponent dummyComp;

	// only on root object
	assert(obj->m_parent.IsNull());

#ifdef _DEBUG
	AABox oriAABB = obj->m_aabb;
#endif // _DEBUG

	obj->RecalculateAABB();

	if (IsDynamicObject(obj))
	{
		RefreshDynamicObject(obj);
	}

#ifdef _DEBUG
	else
	{
		// with static object, object can move only inside its initialized aabb
		AABox aabb = oriAABB;
		aabb.Joint(obj->m_aabb);
		assert(memcmp(&aabb, &oriAABB, sizeof(AABox)) == 0);
	}
#endif // _DEBUG

	m_waitForRefresh.Add(obj);
}

void Scene::ProcessRemoveLists()
{
	auto list = (m_waitForRemove == &m_waitForRemoves[0]) ? &m_waitForRemoves[1] : &m_waitForRemoves[0];
	list->ForEach(
		[&](Handle<GameObject>& obj)
		{
			decltype(m_stableObjects)* objs;

			auto id = obj->m_sceneId;
			
			if (obj->m_sceneDynamicId != INVALID_ID)
			{
				id = UnmaskId(obj->m_sceneId);
				objs = &m_tempObjects;
			}
			else
			{
				objs = &m_stableObjects;
			}

			auto& back = objs->back();
			back->m_sceneId = id;
			objs->operator[](id) = back;
			objs->Pop();
		}
	);
	list->Clear();

	m_waitForRemove->ForEach(
		[&](Handle<GameObject>& obj)
		{
			if (obj->m_sceneDynamicId == INVALID_ID)
			{
				m_staticObjsQueryStructure->Remove(obj->m_aabbQueryId);
			}

			obj->InvokeOnComponentRemovedFromScene();
			obj->InvokeEvent(GameObject::REMOVED_FROM_SCENE);
			obj->m_scene = nullptr;
		}
	);

	m_waitForRemove = list;
}

void Scene::ProcessAddLists()
{
	m_waitForAdd.ForEach(
		[&](Handle<GameObject>& obj)
		{
			obj->m_scene = this;
			obj->m_uid = m_uidCounter++;

			decltype(m_stableObjects)* objs;
			auto physicsComp = obj->GetComponentRaw<Physics>();
			if (physicsComp)
			{
				auto type = physicsComp->Type();

				switch (type)
				{
				case soft::Physics::STATIC:
					obj->m_aabbQueryId = m_staticObjsQueryStructure->Add(obj->m_aabb, obj.Get());
					objs = &m_stableObjects;
					break;
				case soft::Physics::DYNAMIC:
				case soft::Physics::KINEMATIC:
				default:
					objs = &m_tempObjects;
					break;
				}
			}
			else if (m_idMask == 0)
			{
				obj->m_aabbQueryId = m_staticObjsQueryStructure->Add(obj->m_aabb, obj.Get());
				objs = &m_stableObjects;
			}
			else
			{
				objs = &m_tempObjects;
			}

			auto isGhost = obj->m_sceneId == GHOST_OBJECT_ID_MASK;
			obj->m_sceneId = GetSceneId(objs == &m_tempObjects);

			if (isGhost)
			{
				obj->m_sceneId |= GHOST_OBJECT_ID_MASK;
			}

			objs->Push(obj);

			obj->InvokeOnComponentAddedToScene();
			obj->InvokeEvent(GameObject::ADDED_TO_SCENE);
		}
	);
	m_waitForAdd.Clear();
}

void Scene::ProcessRefreshLists()
{
	m_waitForRefresh.ForEach([](GameObject* obj) {
			obj->InvokeOnObjectRefresh();
		}
	);
}

void Scene::ProcessRecordedBranchedLists()
{
	/*m_branchedObjects.ForEach(
		[&](GameObject* obj)
		{
			obj->m_isBranched.store(false, std::memory_order_relaxed);
			if (obj->m_numBranchCount.load(std::memory_order_relaxed) != obj->m_numBranch.load(std::memory_order_relaxed))
			{
				obj->MergeSubSystemComponentsData();

				if (obj->m_isNeedRefresh)
				{
					RefreshObject(obj);
					obj->m_isNeedRefresh = false;
				}
			}
		}
	);*/

	TaskUtils::ForEachConcurrentList(
		m_branchedObjects,
		[&](GameObject* obj,ID)
		{
			auto ret = obj->m_isBranched.exchange(false);
			if (ret == true && obj->m_numBranchCount.load(std::memory_order_relaxed) != obj->m_numBranch.load(std::memory_order_relaxed))
			{
				obj->MergeSubSystemComponentsData();

				if (obj->m_isNeedRefresh)
				{
					RefreshObject(obj);
					obj->m_isNeedRefresh = false;
				}
			}
			return ret == true;
		}, 
		TaskSystem::GetWorkerCount() / 2
	);

	m_branchedObjects.Clear();
}

void Scene::PrevIteration()
{
	Task tasks[4] = {};

	auto& processAddRemove = tasks[0];
	processAddRemove.Entry() = [](void* s) 
	{
		Scene* scene = (Scene*)s;
		scene->ProcessRemoveLists();
		scene->ProcessAddLists();
	};
	processAddRemove.Params() = this;

	auto& processRefresh = tasks[1];
	processRefresh.Entry() = [](void* s)
	{
		Scene* scene = (Scene*)s;
		scene->ProcessRefreshLists();
	};
	processRefresh.Params() = this;

	auto& reconstruct = tasks[2];
	reconstruct.Entry() = [](void* s)
	{
		Scene* scene = (Scene*)s;
		scene->ProcessRecordedBranchedLists();
		scene->ReConstruct();
	};
	reconstruct.Params() = this;

	auto& subSysPrevIteration = tasks[3];
	subSysPrevIteration.Entry() = [](void* s)
	{
		Scene* scene = (Scene*)s;
		for (auto& s : scene->m_subSystems)
		{
			if (s)
				s->PrevIteration(scene->Dt());
		}
	};
	subSysPrevIteration.Params() = this;

	TaskSystem::SubmitAndWait(tasks, 4, Task::CRITICAL);
}

void Scene::Iteration()
{
	m_prevTimeSinceEpoch = m_curTimeSinceEpoch;
	m_curTimeSinceEpoch = Clock::ms::now();
	m_dt = (m_curTimeSinceEpoch - m_prevTimeSinceEpoch) / 1'000.0f;
	m_iterationCount++;

	Task tasks[16];

	// process rendering, physics, script... => 3 tasks, currently
	auto& rendering = tasks[0];
	rendering.Params() = this;
	rendering.Entry() = [](void* s)
	{
		auto scene = (Scene*)s;
		scene->GetRenderingSystem()->Iteration(scene->m_dt);
	};

	auto& physics = tasks[1];
	physics.Params() = this;
	physics.Entry() = [](void* s)
	{
		auto scene = (Scene*)s;
		scene->GetPhysicsSystem()->Iteration(scene->m_dt);
	};

	auto& script = tasks[2];
	script.Params() = this;
	script.Entry() = [](void* s)
	{
		auto scene = (Scene*)s;
		scene->GetScriptSystem()->Iteration(scene->m_dt);
	};

	TaskSystem::SubmitAndWait(tasks, 3, Task::CRITICAL);
}

void Scene::PostIteration()
{
	Synchronize();
}

void Scene::AABBStaticQueryAABox(const AABox& aaBox, SceneStaticQuerySession* session)
{
	assert(m_staticObjsQueryStructure == session->queryStructure);

	auto& ss = session->session;
	m_staticObjsQueryStructure->QueryAABox(aaBox, ss);

	if (ss->m_result.size() != 0)
	{
		session->begin = (GameObject**)&ss->m_result[0];
		session->end = session->begin + ss->m_result.size();
	}
}

void Scene::AABBStaticQueryFrustum(const Frustum& frustum, SceneStaticQuerySession* session)
{
	assert(m_staticObjsQueryStructure == session->queryStructure);

	auto& ss = session->session;
	m_staticObjsQueryStructure->QueryFrustum(frustum, ss);

	if (ss->m_result.size() != 0)
	{
		session->begin = (GameObject**)&ss->m_result[0];
		session->end = session->begin + ss->m_result.size();
	}
}

NAMESPACE_END