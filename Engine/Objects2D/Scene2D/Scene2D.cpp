#include "Scene2D.h"

#include "SubSystems2D/Rendering/RenderingSystem2D.h"
#include "SubSystems2D/Physics/PhysicsSystem2D.h"
#include "SubSystems2D/Script/ScriptSystem2D.h"

#include "Engine.h"
#include "Engine/DebugVar.h"

#include "Objects2D/GameObject2D.h"

#include "Components2D/Script/Script2D.h"
#include "Components2D/Rendering/Sprite.h"
#include "Components2D/Physics/Physics2D.h"

NAMESPACE_BEGIN

Scene2D::Scene2D(Engine* engine)
{
	m_engine = engine;
	m_input = engine->GetInput();
}

Scene2D::~Scene2D()
{
	Dtor();
}

void Scene2D::Setup()
{
	m_renderingSystem	= rheap::New<RenderingSystem2D>(this);
	m_physicsSystem		= rheap::New<PhysicsSystem2D>(this);
	m_scriptSystem		= rheap::New<ScriptSystem2D>(this);

	m_subSystems[Rendering2D::COMPONENT_ID]		= m_renderingSystem;
	m_subSystems[Physics2D::COMPONENT_ID]		= m_physicsSystem;
	m_subSystems[Script2D::COMPONENT_ID]		= m_scriptSystem;

	m_stableObjects.Reserve(1 * MB);

	m_removes.reserve(64 * KB);
	m_adds.reserve(64 * KB);
}

void Scene2D::Dtor()
{
	if (m_oldStableValue != -1)
	{
		mheap::internal::FreeStableObjects((byte)m_oldStableValue, nullptr, nullptr);
		m_oldStableValue = -1;
	}

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

void Scene2D::PrevIteration()
{
	//m_scriptSystem->PrevIteration(Dt());
	//m_physicsSystem->PrevIteration(Dt());
	//m_renderingSystem->PrevIteration(Dt());
	m_trash.clear();
	m_iterationCount++;
	ReConstruct();
}

void Scene2D::Iteration()
{
	if (m_lockedDt == 0)
	{
		m_prevTimeSinceEpoch = m_curTimeSinceEpoch;
		m_curTimeSinceEpoch = Clock::ms::now();
		m_dt = (m_curTimeSinceEpoch - m_prevTimeSinceEpoch) / 1'000.0f;
	}

	m_scriptSystem->Iteration(Dt());
	m_physicsSystem->Iteration(Dt());

	ProcessRemoveList();
	ProcessAddList();
}

void Scene2D::PostIteration()
{
	m_renderingSystem->Iteration(Dt());
}

void Scene2D::AddObject(Handle<GameObject2D>& obj, bool isGhost)
{
	GameObject2D::PostTraversal(obj.Get(),
		[&](GameObject2D* o) {
			o->m_scene = this;
		}
	);

	//obj->RecalculateAABB();

	if (obj->m_type == GameObject2D::STATIC)
	{
		//AddStaticObject(obj.Get());
		obj->m_sceneId = m_stableObjects.size();
		m_stableObjects.Push(obj);
		goto Return;
	}

	//if (obj->m_type != GameObject2D::GHOST)
	//{
	//	//AddDynamicObject(obj.Get());
	//}

	obj->m_sceneId = m_tempObjects.size();
	m_tempObjects.Push(obj);

Return:
//	obj->InvokeOnComponentAddedToScene();
	m_adds.push_back(obj);
}

#define ROLL_TO_FILL_BLANK(v, objName, idName)				\
if (v.size() == 1)											\
{															\
	v.clear();												\
} else {													\
auto& blank = v[objName->idName];							\
auto& back = v.back();										\
back->idName = objName->idName;								\
blank = back;												\
v.Pop();}

void Scene2D::RemoveObject(const Handle<GameObject2D>& obj)
{
#ifdef _DEBUG
	assert((obj->m_sceneDebugVar1 == INVALID_ID) && "object removed twices");
	obj->m_sceneDebugVar1 = 0;
#endif // _DEBUG
	
	m_removes.push_back(obj.Get());
}

void Scene2D::RemoveObjectImpl(GameObject2D* obj)
{
	//obj->m_scene = this;

	if (obj->m_type == GameObject2D::STATIC)
	{
		RemoveStaticObject(obj);

		ROLL_TO_FILL_BLANK(m_stableObjects, obj, m_sceneId);

		goto Return;
	}

	if (obj->m_type != GameObject2D::GHOST)
	{
		RemoveDynamicObject(obj);
	}

	m_trash.Push(obj);
	ROLL_TO_FILL_BLANK(m_tempObjects, obj, m_sceneId);

Return:
	obj->InvokeOnComponentRemovedFromScene();
}

void Scene2D::ProcessRemoveList()
{
	for (auto& obj : m_removes)
	{
		RemoveObjectImpl(obj);
	}
	m_removes.clear();
}

void Scene2D::AddObjectImpl(GameObject2D* obj)
{
	obj->RecalculateAABB();

	if (obj->m_type == GameObject2D::STATIC)
	{
		AddStaticObject(obj);
		goto Return;
	}

	if (obj->m_type != GameObject2D::GHOST)
	{
		AddDynamicObject(obj);
	}

Return:
	obj->InvokeOnComponentAddedToScene();
}

void Scene2D::ProcessAddList()
{
	for (auto& obj : m_adds)
	{
		AddObjectImpl(obj);
	}
	m_adds.clear();
}

NAMESPACE_END