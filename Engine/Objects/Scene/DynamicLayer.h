#pragma once

#include "Core/TypeDef.h"

#include "Scene.h"

NAMESPACE_BEGIN

class AABBQueryStructure;
class GameObject;

class DynamicLayer
{
protected:
	constexpr static size_t ADD_BALANCE_PARAM = 5;

	constexpr static size_t REFRESH_BALANCE_PARAM = 3;

	constexpr static size_t REBALANCE_BATCH_SIZE = 128;

	constexpr static size_t LIMIT_RECONSTRUCT_TIME_NS = 3 * 1000000; // 3 ms

	struct AABBQueryID
	{
		// id refer to each AABBQueryStructure
		/*ID id[2] = { 0, 0 };

		ID ulistId[2] = { 0, 0 };*/

		struct
		{
			ID id = INVALID_ID;
			ID ulistId = INVALID_ID;
		} m[2] = {};
	};

	AABBQueryStructure* m_queryStructures[2] = { nullptr, nullptr };
	//size_t m_queryStructuresSizes[2] = { 0, 0 };

	raw::UnorderedList<GameObject*, 1024> m_objects[2] = {};

	// wait list for each aabb query structure
	raw::ConcurrentList<GameObject*> m_waitForAddObj;
	raw::ConcurrentList<GameObject*> m_waitForRemoveObjs[2];

	raw::ConcurrentList<GameObject*> m_waitForRefreshObjs[2];

	raw::ConcurrentList<AABBQueryID*> m_freeIds;
	raw::ConcurrentList<AABBQueryID*> m_allocatedIds;

	AABBQueryID** m_freeIdsChunk[32] = { {}, {} };
	std::atomic<intmax_t> m_freeIdsChunkNumElm[32] = { {}, {} };
	size_t m_isFreeIdsAvaiable[2] = { false, false };
	size_t m_freeIdsNumChunks = 0;

	GameObject** m_splitAddList[2][32] = { {}, {} };
	GameObject** m_splitRemoveList[2][32] = { {}, {} };

protected:
	inline size_t GetAABBQueryStructureIdOf(GameObject* obj)
	{
		auto id = Scene::GetObjectAABBQueryId(obj);
		return ((AABBQueryID*)id)->m[0].id == INVALID_ID ? 1 : 0;
	}

	void BeginAllocateID();
	AABBQueryID* AllocateID(size_t);
	void EndAllocateID();

	void ProcessRemoveLists();
	void ProcessAddLists();
	void ProcessRefreshLists();
	void IncrementalBalance(size_t remainNS);

public:
	DynamicLayer();
	~DynamicLayer();

public:
	// Inherited via Scene
	void AddDynamicObject(GameObject* obj);

	void RemoveDynamicObject(GameObject* obj);

	void RefreshDynamicObject(GameObject* obj);

	void ReConstruct();

};

NAMESPACE_END