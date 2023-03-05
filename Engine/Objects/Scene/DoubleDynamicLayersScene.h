#pragma once

#include "Scene.h"

NAMESPACE_BEGIN

class DoubleDynamicLayersScene : Traceable<DoubleDynamicLayersScene>, public Scene
{
protected:
	constexpr static size_t ADD_BALANCE_PARAM = 5;

	constexpr static size_t REFRESH_BALANCE_PARAM = 3;

	constexpr static size_t REBALANCE_BATCH_SIZE = 128;

	constexpr static size_t LIMIT_RECONSTRUCT_TIME_NS = 3 * 1000000; // 3 ms

	using Base = Scene;

	struct AABBQueryID
	{
		// id refer to each AABBQueryStructure
		ID id[2] = { 0, 0 };

		ID ulistId[2] = { 0, 0 };
	};

	AABBQueryStructure* m_queryStructures[2] = { nullptr, nullptr };
	//size_t m_queryStructuresSizes[2] = { 0, 0 };

	raw::UnorderedList<GameObject*, 1024> m_objects[2] = {};

	// wait list for each aabb query structure
	raw::ConcurrentList<GameObject*> m_waitForAddObj;
	raw::ConcurrentList<GameObject*> m_waitForRemoveObjs[2];

	raw::ConcurrentList<GameObject*> m_waitForRefreshObjs[2];

	raw::ConcurrentList<AABBQueryID*> m_freeIds;
	AABBQueryID** m_freeIdsChunk[32] = { {}, {} };
	std::atomic<intmax_t> m_freeIdsChunkNumElm[32] = { {}, {} };
	size_t m_isFreeIdsAvaiable[2] = { false, false };
	size_t m_freeIdsNumChunks = 0;

	GameObject** m_splitAddList[2][32] = { {}, {} };
	GameObject** m_splitRemoveList[2][32] = { {}, {} };

protected:
	TRACEABLE_FRIEND();
	void Trace(Tracer* tracer)
	{
		Base::Trace(tracer);
	}

	inline size_t GetAABBQueryStructureIdOf(GameObject* obj)
	{
		auto id = GetObjectAABBQueryId(obj);
		return ((AABBQueryID*)id)->id[0] == 0 ? 1 : 0;
	}

	void BeginAllocateID();
	AABBQueryID* AllocateID(size_t);
	void EndAllocateID();

	void ProcessRemoveLists();
	void ProcessAddLists();
	void ProcessRefreshLists();
	void IncrementalBalance(size_t remainNS);

public:
	// Inherited via Scene
	virtual void AddDynamicObject(GameObject* obj);

	virtual void RemoveDynamicObject(GameObject* obj);

	virtual void RefreshDynamicObject(GameObject* obj);

	virtual void ReConstruct();

	virtual void Synchronize();

};

NAMESPACE_END