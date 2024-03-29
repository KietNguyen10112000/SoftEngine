#pragma once

#include "Core/Structures/Raw/UnorderedList.h"

#include "../Base/MainComponent.h"
#include "../QueryStructures/BVT.h"

NAMESPACE_BEGIN

class DoubleBVH
{
private:
	constexpr static size_t ADD_BALANCE_PARAM					= 5;
	constexpr static size_t REFRESH_BALANCE_PARAM				= 3;
	constexpr static size_t REBALANCE_BATCH_SIZE				= 128;
	constexpr static size_t LIMIT_RECONSTRUCT_TIME_NS			= 3 * 1000000; // 3 ms

	struct TaskParam
	{
		DoubleBVH* doubleBVH;
		ID treeId;
	};

	struct ComponentChunk
	{
		MainComponent** begin;
		MainComponent** end;
	};

	std::vector<MainComponent*> m_addList;
	std::vector<MainComponent*> m_removeList[2];
	std::vector<MainComponent*> m_refreshList[2];

	// component list of each BVT
	raw::UnorderedList<MainComponent*, 1024> m_components[2] = {};

	//ComponentChunk m_splitAddList[2] = {};
	//ComponentChunk m_splitRefreshList[2] = {};
	//ComponentChunk m_splitRemoveList[2] = {};

	ComponentChunk m_splitAddList[2][32] = {};
	ComponentChunk m_splitRemoveList[2][32] = {};

	BVT m_bvt[2] = {};

private:
	inline static size_t GetAABBQueryStructureIdOf(MainComponent* comp)
	{
		auto& id = comp->m_doubleBVHId[0];
		return id.bvhId == INVALID_ID ? 1 : 0;
	}

	void RefreshAll();

	void ProcessAddList();
	void ProcessRemoveList();
	void ProcessRefreshList();
	void IncrementalBalance(size_t remainNS);

public:
	void Reconstruct(size_t limitTimeNS);

	void RecordAddComponent(MainComponent* comp);
	void RecordRemoveComponent(MainComponent* comp);
	void RecordRefreshComponent(MainComponent* comp);

	inline auto NewQuerySession()
	{
		return m_bvt[0].NewSession();
	}

	inline void DeleteQuerySession(AABBQuerySession* querySession)
	{
		m_bvt[0].DeleteSession(querySession);
	}

	inline void QueryAABox(const AABox& aabox, AABBQuerySession* session)
	{
		m_bvt[0].QueryAABox(aabox, session);
		m_bvt[1].QueryAABox(aabox, session);
	}

	inline void QuerySphere(const Sphere& sphere, AABBQuerySession* session)
	{
		m_bvt[0].QuerySphere(sphere, session);
		m_bvt[1].QuerySphere(sphere, session);
	}

	inline void QueryBox(const Box& box, AABBQuerySession* session)
	{
		m_bvt[0].QueryBox(box, session);
		m_bvt[1].QueryBox(box, session);
	}

	inline void QueryFrustum(const Frustum& frustum, AABBQuerySession* session)
	{
		m_bvt[0].QueryFrustum(frustum, session);
		m_bvt[1].QueryFrustum(frustum, session);
	}

	inline void Query(AABBQueryTester* tester, AABBQuerySession* session)
	{
		m_bvt[0].Query(tester, session);
		m_bvt[1].Query(tester, session);
	}
};

NAMESPACE_END