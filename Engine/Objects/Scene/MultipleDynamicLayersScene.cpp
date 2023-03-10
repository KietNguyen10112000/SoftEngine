#include "MultipleDynamicLayersScene.h"

#include "Core/Time/Clock.h"

#include "TaskSystem/TaskSystem.h"

#include "Objects/QueryStructures/AABBQueryStructure.h"


NAMESPACE_BEGIN

MultipleDynamicLayersScene::MultipleDynamicLayersScene()
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

	auto start = Clock::ns::now();
	TaskSystem::SubmitAndWait(tasks, NUM_LAYERS, Task::CRITICAL);
	auto dt = (Clock::ns::now() - start) / 1000000;
	std::cout << "MultipleDynamicLayersScene::ReConstruct() --- " << dt << " ms\n";
}

void MultipleDynamicLayersScene::Synchronize()
{
}

NAMESPACE_END