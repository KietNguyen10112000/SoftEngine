#include "DoubleDynamicLayersScene.h"

#include "Core/Time/Clock.h"

#include "TaskSystem/TaskSystem.h"

#include "Objects/QueryStructures/AABBQueryStructure.h"

NAMESPACE_BEGIN

void DoubleDynamicLayersScene::AddDynamicObject(GameObject* obj)
{
	//auto id = m_addCounter++;
	//auto treeId = id % 2;
	m_waitForAddObj.Add(obj);
}

void DoubleDynamicLayersScene::RemoveDynamicObject(GameObject* obj)
{
	auto treeId = GetAABBQueryStructureIdOf(obj);
	m_waitForRemoveObjs[treeId].Add(obj);
}

void DoubleDynamicLayersScene::RefreshDynamicObject(GameObject* obj)
{
	auto treeId = GetAABBQueryStructureIdOf(obj);
	m_waitForRefreshObjs[treeId].Add(obj);
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
		auto& objects = scene->m_objects[id];
		auto& list = scene->m_waitForRemoveObjs[id];

		list.ForEach(
			[&](GameObject* obj)
			{
				auto treeId = (AABBQueryID*)Scene::GetObjectAABBQueryId(obj);
				freeIds.AddToSpaceUnsafe(id, treeId);
				queryStructure->Remove(treeId->id[id]);
				objects.Remove(treeId->ulistId[id]);
			}
		);

		//scene->m_queryStructuresSizes[id] -= list.size();
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

void DoubleDynamicLayersScene::BeginAllocateID()
{
	auto freeIdsSize = m_freeIds.size();
	if (freeIdsSize > 0) 
	{
		m_isFreeIdsAvaiable[0] = true;
		m_isFreeIdsAvaiable[1] = true;
	}
	else
	{
		m_isFreeIdsAvaiable[0] = false;
		m_isFreeIdsAvaiable[1] = false;
	}

	m_freeIdsNumChunks = m_freeIds.GetChunksCount();
	for (size_t i = 0; i < m_freeIdsNumChunks; i++)
	{
		m_freeIdsChunkNumElm[i] = m_freeIds.GetChunkSizeAt(i);
		m_freeIdsChunk[i] = m_freeIds.GetChunkAddrAtIndex(i);
	}
}

DoubleDynamicLayersScene::AABBQueryID* DoubleDynamicLayersScene::AllocateID(size_t treeId)
{
	if (!m_isFreeIdsAvaiable[treeId])
	{
		return rheap::New<AABBQueryID>();
	}

	auto start = treeId % m_freeIdsNumChunks;
	size_t count = 0;

	intmax_t index = -1;
	while (true)
	{
		index = --m_freeIdsChunkNumElm[start];
		if (index >= 0)
		{
			break;
		}

		count++;

		if (count == m_freeIdsNumChunks)
		{
			m_isFreeIdsAvaiable[treeId] = false;
			return rheap::New<AABBQueryID>();
		}

		start = (start + 1) % m_freeIdsNumChunks;
	}

	auto ret = m_freeIdsChunk[start][index];
	ret->id[0] = 0;
	ret->id[1] = 0;
	return ret;
}

void DoubleDynamicLayersScene::EndAllocateID()
{
	for (size_t i = 0; i < m_freeIdsNumChunks; i++)
	{
		intmax_t size = std::max((intmax_t)0, (intmax_t)m_freeIdsChunkNumElm[i]);
		m_freeIds.ResizeChunk(i, size);
	}
}

void DoubleDynamicLayersScene::ProcessAddLists()
{
	struct CurrentTaskParam
	{
		DoubleDynamicLayersScene* scene;
		ID treeId;
	};

	size_t addSize = m_waitForAddObj.size();
	//intmax_t deltaSize = m_queryStructuresSizes[0] - m_queryStructuresSizes[1];
	intmax_t deltaSize = m_objects[0].size() - m_objects[1].size();

	intmax_t expectDeltaSize = addSize / ADD_BALANCE_PARAM;

	// num object will be add to 1st tree, remain belong to 2nd tree
	size_t sizeOf1;

	if (deltaSize == 0)
	{
		sizeOf1 = addSize / 2;
	}
	else
	{
		expectDeltaSize = std::min(std::abs(deltaSize), expectDeltaSize);
		sizeOf1 = addSize / 2 + expectDeltaSize / 2;
		if (deltaSize > 0)
		{
			// 1st tree higher than 2nd tree, take the less part
			sizeOf1 = addSize - sizeOf1;
		}
	}

	m_waitForAddObj.Split(sizeOf1, m_splitAddList[0], m_splitAddList[1]);

	void (*taskFn)(void*) = [](void* p)
	{
		auto param = (CurrentTaskParam*)p;
		auto scene = param->scene;
		auto id = param->treeId;

		auto& freeIds = scene->m_freeIds;
		auto queryStructure = scene->m_queryStructures[id];
		auto& objects = scene->m_objects[id];
		auto chunks = scene->m_splitAddList[id];

		size_t i = 0;
		while (chunks[i])
		{
			auto it = chunks[i];
			auto end = chunks[i + 1];

			while (it != end)
			{
				auto obj = *it;
				auto aabbId = scene->AllocateID(id);
				aabbId->id[id] = queryStructure->Add(obj->GetAABB(), obj);
				aabbId->ulistId[id] = objects.Add(obj);
				Scene::SetObjectAABBQueryId(obj, (ID)aabbId);
				it++;
			}

			i += 2;
		}
	};

	CurrentTaskParam params[2] = {
		{ this, 0 },
		{ this, 1 }
	};

	Task tasks[2] = {
		{ taskFn, &params[0] },
		{ taskFn, &params[1] }
	};

	BeginAllocateID();
	TaskSystem::SubmitAndWait(tasks, 2, Task::CRITICAL);
	EndAllocateID();

	m_waitForAddObj.Clear();

	//m_queryStructuresSizes[0] += sizeOf1;
	//m_queryStructuresSizes[1] += addSize - sizeOf1;
}

void DoubleDynamicLayersScene::ProcessRefreshLists()
{
	struct CurrentTaskParam
	{
		DoubleDynamicLayersScene* scene;
		ID treeId;
	};

	size_t tree1RefreshSize = m_waitForRefreshObjs[0].size();
	size_t tree2RefreshSize = m_waitForRefreshObjs[1].size();
	intmax_t deltaSize = tree1RefreshSize - tree2RefreshSize;

	size_t lessSize = std::min(tree1RefreshSize, tree2RefreshSize);

	auto numTransferObjs = std::abs(deltaSize);

	if (numTransferObjs > (lessSize / REFRESH_BALANCE_PARAM))
	{
		m_waitForRefreshObjs[0].GetChunks(m_splitAddList[0]);
		m_waitForRefreshObjs[0].GetChunks(m_splitRemoveList[0]);

		m_waitForRefreshObjs[1].GetChunks(m_splitAddList[1]);
		m_waitForRefreshObjs[1].GetChunks(m_splitRemoveList[1]);
	}
	else
	{
		auto p1 = &m_waitForRefreshObjs[0];
		auto p1Size = tree1RefreshSize;
		auto p1AddList = m_splitAddList[0];
		auto p1RemoveList = m_splitAddList[0];

		auto p2 = &m_waitForRefreshObjs[1];
		auto p2Size = tree2RefreshSize;
		auto p2AddList = m_splitAddList[1];
		auto p2RemoveList = m_splitAddList[1];

		if (deltaSize < 0)
		{
			std::swap(p1, p2);
			std::swap(p1Size, p2Size);
			std::swap(p1AddList, p2AddList);
			std::swap(p1RemoveList, p2RemoveList);
		}

		// p1 -> higher refresh rate tree

		p2->GetChunks(p2RemoveList);
		auto p2AddListEnd = p2AddList + p2->GetChunks(p2AddList);

		auto numRefreshObjectP1 = p1Size - numTransferObjs;
		p1->GetChunks(p1RemoveList);
		p1->Split(numRefreshObjectP1, p1AddList, p2AddListEnd);

		//if (deltaSize < 0)
		//{
		//	// tree1 < tree2
		//	m_queryStructuresSizes[0] += numTransferObjs;
		//	m_queryStructuresSizes[1] -= numTransferObjs;
		//}
		//else
		//{
		//	// tree1 > tree2
		//	m_queryStructuresSizes[0] -= numTransferObjs;
		//	m_queryStructuresSizes[1] += numTransferObjs;
		//}
	}

	void (*taskFn)(void*) = [](void* p)
	{
		auto param = (CurrentTaskParam*)p;
		auto scene = param->scene;
		auto id = param->treeId;

		auto& freeIds = scene->m_freeIds;
		auto queryStructure = scene->m_queryStructures[id];
		auto& objects = scene->m_objects[id];

		auto addChunks = scene->m_splitAddList[id];
		auto removeChunks = scene->m_splitAddList[id];

		size_t i = 0;

		#define LOOP(chunkName, body)								\
		while (addChunks[i])										\
		{															\
			auto it = addChunks[i];									\
			auto end = addChunks[i + 1];							\
			while (it != end)										\
			{														\
				auto obj = *it;										\
				body												\
				it++;												\
			}														\
			i += 2;													\
		}

		LOOP(
			removeChunks,
			auto queryId = (AABBQueryID*)Scene::GetObjectAABBQueryId(obj);
			auto& nodeId = queryId->id[id];
			auto& ulistId = queryId->ulistId[id];

			queryStructure->Remove(nodeId);
			objects.Remove(ulistId);

			nodeId = 0;
			ulistId = 0;
		);

		LOOP(
			addChunks,
			auto queryId = (AABBQueryID*)Scene::GetObjectAABBQueryId(obj);
			auto& nodeId = queryId->id[id];
			auto& ulistId = queryId->ulistId[id];

			nodeId = queryStructure->Add(obj->GetAABB(), obj);
			ulistId = objects.Add(obj);
		);

		#undef LOOP
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

	m_waitForRefreshObjs[0].Clear();
	m_waitForRefreshObjs[1].Clear();
}

void DoubleDynamicLayersScene::IncrementalBalance(size_t remainNS)
{
	auto start = Clock::ns::now();

	auto p1 = m_queryStructures[0];
	auto* objs1 = &m_objects[0];
	size_t treeId1 = 0;

	auto p2 = m_queryStructures[1];
	auto* objs2 = &m_objects[1];
	size_t treeId2 = 1;

	if (objs1->size() == objs2->size()) return;

	if (objs1->size() < objs2->size())
	{
		std::swap(p1, p2);
		std::swap(objs1, objs2);
		std::swap(treeId1, treeId2);
	}

	// p1 is higher tree

	while (true)
	{
		for (size_t i = 0; i < REBALANCE_BATCH_SIZE; i++)
		{
			auto obj = objs1->back();
			auto queryId = (AABBQueryID*)Scene::GetObjectAABBQueryId(obj);

			auto& nodeId1 = queryId->id[treeId1];
			auto& ulistId1 = queryId->ulistId[treeId1];

			auto& nodeId2 = queryId->id[treeId2];
			auto& ulistId2 = queryId->ulistId[treeId2];

			p1->Remove(nodeId1);
			objs1->Remove(ulistId1);

			nodeId1 = 0;
			ulistId1 = 0;

			nodeId2 = p2->Add(obj->GetAABB(), obj);
			ulistId2 = objs2->Add(obj);

			if (objs1->size() == objs2->size())
			{
				return;
			}
		}

		if (Clock::ns::now() - start > remainNS)
		{
			break;
		}
	}
}

void DoubleDynamicLayersScene::ReConstruct()
{
	auto start = Clock::ns::now();

	std::cout << "DoubleDynamicLayersScene::ProcessRemoveLists()\n";
	ProcessRemoveLists();

	std::cout << "DoubleDynamicLayersScene::ProcessRefreshLists()\n";
	ProcessRefreshLists();

	std::cout << "DoubleDynamicLayersScene::ProcessAddLists()\n";
	ProcessAddLists();

	auto now = Clock::ns::now();
	if (now - start < LIMIT_RECONSTRUCT_TIME_NS)
	{
		std::cout << "DoubleDynamicLayersScene::IncrementalBalance()\n";
		IncrementalBalance(now - start);
	}
}

void DoubleDynamicLayersScene::Synchronize()
{
}

NAMESPACE_END