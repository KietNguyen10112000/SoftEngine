#include "Scene.h"

#include "Components/Physics/PhysicsComponent.h"
#include "Objects/QueryStructures/AABBQueryStructure.h"

#include "Event/BuiltinEventManager.h"
#include "Event/EventManager.h"

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
	obj->m_uid = m_uidCounter++;
	obj->m_sceneId = GetSceneId();
	m_objsAccessor->Push(obj);

	auto physicsComp = obj->GetComponentRaw<PhysicsComponent>();
	if (physicsComp)
	{
		// insert to AABBQueryStructure base on PhysicsComponent
		auto type = physicsComp->Type();

		switch (type)
		{
		case soft::PhysicsComponent::STATIC:
			obj->m_aabbQueryId = m_staticObjsQueryStructure->Add(obj->m_aabb, obj.Get());
			break;
		case soft::PhysicsComponent::DYNAMIC:
		case soft::PhysicsComponent::KINEMATIC:
			obj->m_aabbQueryId = m_dynamicObjsQueryStructure->Add(obj->m_aabb, obj.Get());
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

	obj->m_aabbQueryId = m_dynamicObjsQueryStructure->Add(obj->m_aabb, obj.Get());
}

void Scene::RemoveObject(Handle<GameObject>& obj)
{

}

NAMESPACE_END