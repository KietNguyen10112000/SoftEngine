#include "Scene.h"

#include "Components/Physics/Physics.h"
#include "Objects/QueryStructures/AABBQueryStructure.h"

#include "Event/BuiltinEventManager.h"
#include "Event/EventManager.h"

#include "TaskSystem/TaskSystem.h"

NAMESPACE_BEGIN

Scene::Scene()
{
	m_objectEventMgr = rheap::New<BuiltinEventManager>(this);
	m_eventMgr = mheap::New<EventManager>(512);
}

Scene::~Scene()
{
	rheap::Delete(m_objectEventMgr);
}

void Scene::AddObject(Handle<GameObject>& obj)
{
	m_waitForAdd.Add(obj);

	auto physicsComp = obj->GetComponentRaw<Physics>();
	if (physicsComp)
	{
		// insert to AABBQueryStructure base on PhysicsComponent
		auto type = physicsComp->Type();

		switch (type)
		{
		case soft::Physics::STATIC:
			obj->m_aabbQueryId = m_staticObjsQueryStructure->Add(obj->m_aabb, obj.Get());
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

	// insert by default
	if (m_idMask == TEMP_OBJECT_ID_MASK)
	{
		// stable object treat as static object
		obj->m_aabbQueryId = m_staticObjsQueryStructure->Add(obj->m_aabb, obj.Get());
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

void Scene::ProcessRemoveLists()
{
	auto list = (m_waitForRemove == &m_waitForRemoves[0]) ? &m_waitForRemoves[1] : &m_waitForRemoves[0];
	list->ForEach(
		[&](Handle<GameObject>& obj)
		{
			auto id = obj->m_sceneId;
			auto& back = m_objsAccessor->back();
			m_objsAccessor->operator[](id) = back;
			m_objsAccessor->Pop();
		}
	);
	list->Clear();
	m_waitForRemove = list;
}

void Scene::ProcessAddLists()
{
	m_waitForAdd.ForEach(
		[&](Handle<GameObject>& obj)
		{
			obj->m_uid = m_uidCounter++;
			obj->m_sceneId = GetSceneId();
			m_objsAccessor->Push(obj);
		}
	);
	m_waitForAdd.Clear();
}

void Scene::PrevIteration()
{
	Task tasks[2] = {};

	auto& processAddRemove = tasks[0];
	processAddRemove.Entry() = [](void* s) 
	{
		Scene* scene = (Scene*)s;
		scene->ProcessRemoveLists();
		scene->ProcessAddLists();
	};
	processAddRemove.Params() = this;

	auto& reconstruct = tasks[1];
	reconstruct.Entry() = [](void* s)
	{
		Scene* scene = (Scene*)s;
		scene->ReConstruct();
	};
	reconstruct.Params() = this;

	TaskSystem::SubmitAndWait(tasks, 2, Task::CRITICAL);
}

void Scene::PostIteration()
{
	Synchronize();
}

NAMESPACE_END