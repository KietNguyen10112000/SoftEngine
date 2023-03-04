#pragma once

#include "Scene.h"

NAMESPACE_BEGIN

class DoubleDynamicLayersScene : Traceable<DoubleDynamicLayersScene>, public Scene
{
protected:
	using Base = Scene;

	struct AABBQueryID
	{
		// id refer to each AABBQueryStructure
		ID id[2] = { 0, 0 };
	};

	AABBQueryStructure* m_queryStructures[2] = { nullptr, nullptr };

	// wait list for each aabb query structure
	raw::ConcurrentList<GameObject*> m_waitForAddObjs[2];
	raw::ConcurrentList<GameObject*> m_waitForRemoveObjs[2];

	std::atomic<ID> m_addCounter = { 0 };

	raw::ConcurrentList<AABBQueryID*> m_freeIds;

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

	void ProcessRemoveLists();
	void ProcessAddLists();

public:
	// Inherited via Scene
	virtual void AddDynamicObject(GameObject* obj);

	virtual void RemoveDynamicObject(GameObject* obj);

	virtual void ReConstruct();

	virtual void Synchronize();

};

NAMESPACE_END