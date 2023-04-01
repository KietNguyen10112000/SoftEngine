#include "MultipleDynamicLayersScene.h"

#include "Core/Time/Clock.h"

#include "TaskSystem/TaskSystem.h"

#include "Objects/QueryStructures/AABBQueryStructure.h"


NAMESPACE_BEGIN

MultipleDynamicLayersScene::MultipleDynamicLayersScene(Engine* engine) : Scene(engine)
{
	for (size_t i = 0; i < NUM_LAYERS; i++)
	{
		m_layerObjsCount[i] = 0;
		m_layerIDs[i] = i;
	}
}

MultipleDynamicLayersScene::~MultipleDynamicLayersScene()
{
	
}

void MultipleDynamicLayersScene::AddDynamicObject(GameObject* obj)
{
	ID layerId = 0;
	size_t minObjCount = m_layerObjsCount[0].load();

	for (size_t i = 1; i < NUM_LAYERS; i++)
	{
		auto cur = m_layerObjsCount[i].load();
		if (cur < minObjCount)
		{
			layerId = i;
			minObjCount = cur;
		}
	}

	m_layers[layerId].AddDynamicObject(obj);
	obj->m_sceneDynamicId = (m_layerIDs[layerId] += NUM_LAYERS);
	m_layerObjsCount[layerId]++;
}

void MultipleDynamicLayersScene::RemoveDynamicObject(GameObject* obj)
{
	m_layers[obj->m_sceneDynamicId % NUM_LAYERS].RemoveDynamicObject(obj);
	obj->m_sceneDynamicId = INVALID_ID;
}

void MultipleDynamicLayersScene::RefreshDynamicObject(GameObject* obj)
{
	m_layers[obj->m_sceneDynamicId % NUM_LAYERS].RefreshDynamicObject(obj);
}

void MultipleDynamicLayersScene::ReConstruct()
{
	Task tasks[NUM_LAYERS];

	void (*fn)(void*) = [](void* l)
	{
		auto layer = (DynamicLayer*)l;
		layer->ReConstruct();
	};

	for (size_t i = 0; i < NUM_LAYERS; i++)
	{
		tasks[i].Entry() = fn;
		tasks[i].Params() = &m_layers[i];
	}

	//auto start = Clock::ns::now();
	TaskSystem::SubmitAndWait(tasks, NUM_LAYERS, Task::CRITICAL);
	//auto dt = (Clock::ns::now() - start) / 1000000;
	//std::cout << "MultipleDynamicLayersScene::ReConstruct() --- " << dt << " ms\n";
}

void MultipleDynamicLayersScene::Synchronize()
{
}

UniquePtr<SceneQuerySession> MultipleDynamicLayersScene::NewDynamicQuerySession()
{
	auto ret = MakeUnique<MultipleDynamicLayersSceneQuerySession>();
	m_layers[0].InitSession(ret->session);
	return ret;
}

void MultipleDynamicLayersScene::AABBDynamicQueryAABox(const AABox& aaBox, SceneQuerySession* session)
{
	auto ss = ((MultipleDynamicLayersSceneQuerySession*)session)->session;
	for (size_t i = 0; i < NUM_LAYERS; i++)
	{
		m_layers[i].AABBQueryAABox(aaBox, ss);
	}

	auto& ret = ss.querySession->m_result;
	if (ret.size() != 0)
	{
		session->begin = (GameObject**)&ret[0];
		session->end = session->begin + ret.size();
	}
}

void MultipleDynamicLayersScene::AABBDynamicQueryFrustum(const Frustum& frustum, SceneQuerySession* session)
{
	auto ss = ((MultipleDynamicLayersSceneQuerySession*)session)->session;
	for (size_t i = 0; i < NUM_LAYERS; i++)
	{
		m_layers[i].AABBQueryFrustum(frustum, ss);
	}

	auto& ret = ss.querySession->m_result;
	if (ret.size() != 0)
	{
		session->begin = (GameObject**)&ret[0];
		session->end = session->begin + ret.size();
	}
}

NAMESPACE_END