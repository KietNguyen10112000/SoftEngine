#include "DoubleBVH.h"

#include "Core/Time/Clock.h"

#include "TaskSystem/TaskSystem.h"

NAMESPACE_BEGIN

void DoubleBVH::RecordAddComponent(MainComponent* comp)
{
	assert(comp->m_doubleBVHId[0].bvhId == INVALID_ID && comp->m_doubleBVHId[1].bvhId == INVALID_ID);

	m_addList.push_back(comp);
}

void DoubleBVH::RecordRemoveComponent(MainComponent* comp)
{
	assert(comp->m_doubleBVHId[0].bvhId != INVALID_ID || comp->m_doubleBVHId[1].bvhId != INVALID_ID);

	auto treeId = GetAABBQueryStructureIdOf(comp);
	m_removeList[treeId].push_back(comp);
}

void DoubleBVH::RecordRefreshComponent(MainComponent* comp)
{
	if (comp->m_doubleBVHId[0].bvhId == INVALID_ID && comp->m_doubleBVHId[1].bvhId == INVALID_ID)
	{
		return;
	}

	auto treeId = GetAABBQueryStructureIdOf(comp);
	m_refreshList[treeId].push_back(comp);
}

void DoubleBVH::ProcessAddList()
{
	size_t addSize = m_addList.size();

	if (addSize == 0)
	{
		return;
	}

	//std::cout << "Add size " << addSize << "\n";

	//intmax_t deltaSize = m_queryStructuresSizes[0] - m_queryStructuresSizes[1];
	intmax_t deltaSize = m_components[0].size() - m_components[1].size();

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

	//std::cout << "Add sizeOf1 " << sizeOf1 << "\n";
	//m_addList.Split(sizeOf1, m_splitAddList[0], m_splitAddList[1]);

	auto& list1 = m_splitAddList[0][0];
	list1.begin = m_addList.data();
	list1.end = m_addList.data() + sizeOf1;

	auto& list2 = m_splitAddList[1][0];
	list2.begin = list1.end;
	list2.end = &(*m_addList.rbegin()) + 1;

	void (*taskFn)(void*) = [](void* p)
	{
		auto param = (TaskParam*)p;
		auto doubleBVT = param->doubleBVH;
		auto id = param->treeId;

		//auto& freeIds = scene->m_freeIds;
		auto& queryStructure = doubleBVT->m_bvt[id];
		auto& objects = doubleBVT->m_components[id];
		//auto chunks = scene->m_splitAddList[id];

		auto it = doubleBVT->m_splitAddList[id][0].begin;
		auto end = doubleBVT->m_splitAddList[id][0].end;

		while (it != end)
		{
			auto comp = *it;
			auto& doubleBVTId = comp->m_doubleBVHId[id];
			doubleBVTId.bvhId = queryStructure.Add(comp->GetGlobalAABB(), comp);
			doubleBVTId.ulistId = objects.Add(comp);
			it++;
		}
	};

	TaskParam params[2] = {
		{ this, 0 },
		{ this, 1 }
	};

	Task tasks[2] = {
		{ taskFn, &params[0] },
		{ taskFn, &params[1] }
	};

	TaskSystem::SubmitAndWait(tasks, 2, Task::CRITICAL);

	m_addList.clear();
}

void DoubleBVH::RefreshAll()
{
	void (*taskFn)(void*) = [](void* p)
	{
		auto param = (TaskParam*)p;
		auto layer = param->doubleBVH;
		auto id = param->treeId;

		auto& queryStructure = layer->m_bvt[id];
		auto& objects = layer->m_components[id];

		queryStructure.Clear();

		objects.ForEach(
			[&](MainComponent* comp)
			{
				auto& queryId = comp->m_doubleBVHId[id];
				auto& nodeId = queryId.bvhId;
				nodeId = queryStructure.Add(comp->GetGlobalAABB(), comp);
			}
		);
	};

	TaskParam params[2] = {
		{ this, 0 },
		{ this, 1 }
	};

	Task tasks[2] = {
		{ taskFn, &params[0] },
		{ taskFn, &params[1] }
	};

	TaskSystem::SubmitAndWait(tasks, 2, Task::CRITICAL);
}

void DoubleBVH::ProcessRefreshList()
{
	size_t tree1RefreshSize = m_refreshList[0].size();
	size_t tree2RefreshSize = m_refreshList[1].size();

	if (tree1RefreshSize + tree2RefreshSize > 1 + (m_components[0].size() + m_components[1].size()) / 2)
	{
		/// 
		/// each Add or Remove operator on DVBTQueryTree cost O(log(n)) (n is num objs on tree)
		/// => if we Remove then Add num objects that greater than n / 2, we cost n*O(log(n)) same as time to RefreshAll tree
		/// 

		RefreshAll();
		goto Return;
	}

	intmax_t deltaSize = tree1RefreshSize - tree2RefreshSize;

	size_t lessSize = std::min(tree1RefreshSize, tree2RefreshSize);

	auto numTransferObjs = std::abs(deltaSize);

	if (numTransferObjs < (lessSize / REFRESH_BALANCE_PARAM) || lessSize <= 5)
		//if (numTransferObjs == 0)
		//if (numTransferObjs != -1)
	{
		m_splitAddList[0][0].begin = m_refreshList[0].data();
		m_splitAddList[0][0].end = m_refreshList[0].data() + tree1RefreshSize;
		m_splitAddList[0][1].begin = 0;
		m_splitAddList[0][1].end = 0;

		m_splitRemoveList[0][0].begin = m_refreshList[0].data();
		m_splitRemoveList[0][0].end = m_refreshList[0].data() + tree1RefreshSize;
		m_splitRemoveList[0][1].begin = 0;
		m_splitRemoveList[0][1].end = 0;


		m_splitAddList[1][0].begin = m_refreshList[1].data();
		m_splitAddList[1][0].end = m_refreshList[1].data() + tree2RefreshSize;
		m_splitAddList[1][1].begin = 0;
		m_splitAddList[1][1].end = 0;

		m_splitRemoveList[1][0].begin = m_refreshList[1].data();
		m_splitRemoveList[1][0].end = m_refreshList[1].data() + tree2RefreshSize;
		m_splitRemoveList[1][1].begin = 0;
		m_splitRemoveList[1][1].end = 0;
	}
	else
	{
		auto p1 = &m_refreshList[0];
		auto p1Size = tree1RefreshSize;
		auto p1AddList = m_splitAddList[0];
		auto p1RemoveList = m_splitRemoveList[0];

		auto p2 = &m_refreshList[1];
		auto p2Size = tree2RefreshSize;
		auto p2AddList = m_splitAddList[1];
		auto p2RemoveList = m_splitRemoveList[1];

		if (deltaSize < 0)
		{
			std::swap(p1, p2);
			std::swap(p1Size, p2Size);
			std::swap(p1AddList, p2AddList);
			std::swap(p1RemoveList, p2RemoveList);
		}

		// p1 -> higher refresh rate tree
		auto numRefreshObjectP1 = p1Size - numTransferObjs;

		p1RemoveList[0].begin = p1->data();
		p1RemoveList[0].end = p1->data() + p1->size();
		p1RemoveList[1].begin = 0;
		p1RemoveList[1].end = 0;

		p1AddList[0].begin = p1->data();
		p1AddList[0].end = p1->data() + numRefreshObjectP1;
		p1AddList[1].begin = 0;
		p1AddList[1].end = 0;


		p2RemoveList[0].begin = p2->data();
		p2RemoveList[0].end = p2->data() + p2->size();
		p2RemoveList[1].begin = 0;
		p2RemoveList[1].end = 0;

		p2AddList[0].begin = p2->data();
		p2AddList[0].end = p2->data() + p2->size();
		p2AddList[1].begin = p1AddList[0].end;
		p2AddList[1].end = &(*p1->rbegin()) + 1;
		p2AddList[2].begin = 0;
		p2AddList[2].end = 0;
	}

	void (*taskFn)(void*) = [](void* p)
	{
		auto param = (TaskParam*)p;
		auto doubleBVT = param->doubleBVH;
		auto id = param->treeId;

		auto& queryStructure = doubleBVT->m_bvt[id];
		auto& objects = doubleBVT->m_components[id];

		auto addChunks = doubleBVT->m_splitAddList[id];
		auto removeChunks = doubleBVT->m_splitRemoveList[id];

		size_t i = 0;

#define LOOP(chunkName, body)										\
		while (chunkName[i].begin)									\
		{															\
			auto it = chunkName[i].begin;							\
			auto end = chunkName[i].end;							\
			while (it != end)										\
			{														\
				auto obj = *it;										\
				body												\
				it++;												\
			}														\
			i++;													\
		}

		LOOP(
			removeChunks,
			auto& queryId = obj->m_doubleBVHId[id];
			auto& nodeId = queryId.bvhId;
			auto& ulistId = queryId.ulistId;

			queryStructure.Remove(nodeId);
			objects.Remove(ulistId);

			nodeId = INVALID_ID;
			ulistId = INVALID_ID;
		);

		i = 0;
		LOOP(
			addChunks,
			auto& queryId = obj->m_doubleBVHId[id];
			auto& nodeId = queryId.bvhId;
			auto& ulistId = queryId.ulistId;

			nodeId = queryStructure.Add(obj->GetGlobalAABB(), obj);
			ulistId = objects.Add(obj);
		);

#undef LOOP
	};

	TaskParam params[2] = {
		{ this, 0 },
		{ this, 1 }
	};

	Task tasks[2] = {
		{ taskFn, &params[0] },
		{ taskFn, &params[1] }
	};

	//auto start = Clock::ns::now();
	TaskSystem::SubmitAndWait(tasks, 2, Task::CRITICAL);
	//auto dt = Clock::ns::now() - start;
	//std::cout << "Dt: " << dt / 1000000 << " ms\n";

	//auto start = Clock::ns::now();
	//tasks[0].Entry()(tasks[0].Params());
	//tasks[1].Entry()(tasks[1].Params());
	//auto dt = Clock::ns::now() - start;
	//std::cout << "Dt: " << dt / 1000000 << " ms\n";
	////tasks[1].Entry()(tasks[1].Params());

Return:
	m_refreshList[0].clear();
	m_refreshList[1].clear();
}

void DoubleBVH::ProcessRemoveList()
{
	void (*taskFn)(void*) = [](void* p)
	{
		auto param = (TaskParam*)p;
		auto scene = param->doubleBVH;
		auto id = param->treeId;

		auto& queryStructure = scene->m_bvt[id];
		auto& objects = scene->m_components[id];
		auto& list = scene->m_removeList[id];

		for (auto& comp : list)
		{
			auto& queryId = comp->m_doubleBVHId[id];
			queryStructure.Remove(queryId.bvhId);
			objects.Remove(queryId.ulistId);

			queryId.bvhId = INVALID_ID;
			queryId.ulistId = INVALID_ID;
		}
		
		list.clear();
	};

	TaskParam params[2] = {
		{ this, 0 },
		{ this, 1 }
	};

	Task tasks[2] = {
		{ taskFn, &params[0] },
		{ taskFn, &params[1] }
	};

	TaskSystem::SubmitAndWait(tasks, 2, Task::CRITICAL);
}

void DoubleBVH::IncrementalBalance(size_t remainNS)
{
	auto start = Clock::ns::now();

	auto p1 = &m_bvt[0];
	auto* objs1 = &m_components[0];
	size_t treeId1 = 0;

	auto p2 = &m_bvt[1];
	auto* objs2 = &m_components[1];
	size_t treeId2 = 1;

	if (std::abs((intmax_t)objs1->size() - (intmax_t)objs2->size()) < 2) return;

	if (objs1->size() < objs2->size())
	{
		std::swap(p1, p2);
		std::swap(objs1, objs2);
		std::swap(treeId1, treeId2);
	}

	// p1 is higher tree

	//std::cout << objs1->size() << ", " << objs2->size() << '\n';
	while (true)
	{
		for (size_t i = 0; i < REBALANCE_BATCH_SIZE; i++)
		{
			//std::cout << objs1->size() << ", " << objs2->size() << '\n';

			auto comp = objs1->back();
			auto& queryId = comp->m_doubleBVHId;

			auto& nodeId1 = queryId[treeId1].bvhId;
			auto& ulistId1 = queryId[treeId1].ulistId;

			auto& nodeId2 = queryId[treeId2].bvhId;
			auto& ulistId2 = queryId[treeId2].ulistId;

			p1->Remove(nodeId1);
			objs1->Remove(ulistId1);

			nodeId1 = INVALID_ID;
			ulistId1 = INVALID_ID;

			nodeId2 = p2->Add(comp->GetGlobalAABB(), comp);
			ulistId2 = objs2->Add(comp);

			if (objs1->size() <= objs2->size())
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

void DoubleBVH::Reconstruct(size_t limitTimeNS)
{
	auto start = Clock::ns::now();

	ProcessAddList();
	ProcessRefreshList();
	ProcessRemoveList();

	auto dt = Clock::ns::now() - start;
	if (dt < LIMIT_RECONSTRUCT_TIME_NS)
	{
		//std::cout << "DynamicLayer::IncrementalBalance()\n";
		IncrementalBalance(LIMIT_RECONSTRUCT_TIME_NS - dt);
	}
}

NAMESPACE_END