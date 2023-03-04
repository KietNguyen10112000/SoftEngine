#include "DoubleDynamicLayersScene.h"

#include "TaskSystem/TaskSystem.h"

#include "Objects/QueryStructures/AABBQueryStructure.h"

NAMESPACE_BEGIN

void DoubleDynamicLayersScene::AddDynamicObject(GameObject* obj)
{
	auto id = m_addCounter++;
	auto treeId = id % 2;
	m_waitForAddObjs[treeId].Add(obj);
}

void DoubleDynamicLayersScene::RemoveDynamicObject(GameObject* obj)
{
	auto treeId = GetAABBQueryStructureIdOf(obj);
	m_waitForAddObjs[treeId].Add(obj);
}

void DoubleDynamicLayersScene::ProcessRemoveLists()
{
	struct CurrentTaskParam
	{
		DoubleDynamicLayersScene* scene;
		ID treeId;
	};

	void (*taskFn)(void*) = [](void* p)
	{
		auto param = (CurrentTaskParam*)p;
		auto scene = param->scene;
		auto id = param->treeId;

		auto& freeIds = scene->m_freeIds;
		auto queryStructure = scene->m_queryStructures[id];
		auto& list = scene->m_waitForRemoveObjs[id];

		list.ForEach(
			[&](GameObject* obj)
			{
				auto treeId = (AABBQueryID*)Scene::GetObjectAABBQueryId(obj);
				freeIds.AddToSpaceUnsafe(id, treeId);
				queryStructure->Remove(treeId->id[id]);
			}
		);
		list.Clear();
	};

	CurrentTaskParam params[2] = {
		{ this, 0 },
		{ this, 1 }
	};

	Task tasks[2] = {
		{ taskFn, &params[0] },
		{ taskFn, &params[1] }
	};

	TaskSystem::SubmitAndWait(tasks, 2, Task::CRITICAL);
}

void DoubleDynamicLayersScene::ProcessAddLists()
{
	struct CurrentTaskParam
	{
		DoubleDynamicLayersScene* scene;
		ID treeId;
	};

	Task tasks[2] = {};

	/*void (*taskFn)(void*) = [](void* p)
	{
		auto param = (CurrentTaskParam*)p;
		auto scene = param->scene;
		auto id = param->treeId;

		auto& freeIds = scene->m_freeIds;
		auto queryStructure = scene->m_queryStructures[id];
		auto& list = scene->m_waitForRemoveObjs[id];

		list.ForEach(
			[&](GameObject* obj)
			{
				auto treeId = (AABBQueryID*)Scene::GetObjectAABBQueryId(obj);
				freeIds.AddToSpaceUnsafe(id, treeId);
				queryStructure->Remove(treeId->id[id]);
			}
		);
	};*/
}

void DoubleDynamicLayersScene::ReConstruct()
{
	ProcessRemoveLists();
	ProcessAddLists();
}

void DoubleDynamicLayersScene::Synchronize()
{
}

NAMESPACE_END