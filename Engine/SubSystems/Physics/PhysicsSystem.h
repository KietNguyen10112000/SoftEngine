#pragma once

#include "SubSystems/SubSystem.h"

#include "Objects/Physics/Collision/Manifold.h"

NAMESPACE_BEGIN

class Scene;

class PhysicsSystem : public SubSystem
{
private:
	size_t m_iterationCount = 0;

	raw::ConcurrentArrayList<GameObject*> m_entriesBoardPhase[2];

	// after board phase entry object will contact with many new objects outside of m_entriesBoardPhase
	// so record it
	raw::ConcurrentArrayList<GameObject*> m_boardPhaseOutputObjs[2];

	// all collision pairs of board phase, narrow phase will resolve all these manifold
	raw::ConcurrentArrayList<Manifold*> m_boardPhaseManifolds;

	// when multithreaded traserval over scene struct, 2 thread can touch same object
	raw::ConcurrentArrayList<GameObject*> m_boardPhaseDuplicateManifoldObjs;

	std::atomic<ID> m_manifoldIDCount = { 0 };
	raw::ConcurrentArrayList<Manifold*> m_allocatedManifolds;
	raw::ConcurrentArrayList<Manifold*> m_freeManifolds;
	raw::ConcurrentArrayList<Manifold*>::ConsumeHead m_freeManifoldsHead;

	UniquePtr<SceneQuerySession> m_querySessions[ThreadLimit::MAX_THREADS];

	struct BoardPhaseCtx
	{
		std::Vector<Physics*> stack;
	};

	BoardPhaseCtx m_boardPhaseCtxs[ThreadLimit::MAX_THREADS];

public:
	PhysicsSystem(Scene* scene);
	~PhysicsSystem();

public:
	virtual void PrevIteration(float dt) override;

	virtual void Iteration(float dt) override;

	virtual void PostIteration(float dt) override;

	inline virtual bool FilterAddSubSystemComponent(SubSystemComponent* comp) override 
	{ 
		if (m_scene->IsGhost(comp->GetObject()))
		{
			return false;
		}
		
		auto physics = (Physics*)comp;
		if (physics->m_TYPE == Physics::DYNAMIC || physics->m_TYPE == Physics::KINEMATIC)
		{
			AddToBeginBoardPhase(comp->GetObject());
			return true;
		}

		return false;
	};

	inline virtual bool FilterRemoveSubSystemComponent(SubSystemComponent* comp) override
	{
		if (m_scene->IsGhost(comp->GetObject()))
		{
			return false;
		}

		auto physics = (Physics*)comp;
		if (physics->m_TYPE == Physics::DYNAMIC || physics->m_TYPE == Physics::KINEMATIC)
		{
			return true;
		}

		return false;
	};

private:
	inline Manifold* AllocateManifold()
	{
		Manifold* ret;
		if (m_freeManifoldsHead.TryTake(ret))
		{
			ret->refManifold = nullptr;
			return ret;
		}

		ret = rheap::New<Manifold>();
		m_allocatedManifolds.Add(ret);
		ret->m_id = ++m_manifoldIDCount;
		ret->refManifold = nullptr;
		return ret;
	}

	inline void DeallocateManifold(Manifold* m)
	{
		m_freeManifolds.Add(m);
	}

	inline auto& GetCurEntriesBoardPhase()
	{
		return m_entriesBoardPhase[m_iterationCount % 2];
	}

	inline auto& GetNextEntriesBoardPhase()
	{
		return m_entriesBoardPhase[(m_iterationCount + 1) % 2];
	}

	inline auto& GetCurBoardPhaseOutputObjs()
	{
		return m_boardPhaseOutputObjs[m_iterationCount % 2];
	}

	inline auto& GetPrevBoardPhaseOutputObjs()
	{
		return m_boardPhaseOutputObjs[(m_iterationCount + 1) % 2];
	}

	void NewPhysicsIteration();
	void NewPhysicsIterationProcessObject(GameObject* obj, ID dispatchId);
	void BoardPhase();
	void BoardPhaseProcessObject(GameObject* obj, ID dispatchId);
	void NarrowPhase();
	void EndPhysicsIteration();

	void BoardPhaseProcessCtx(BoardPhaseCtx& ctx, SceneQuerySession* querySession);
	void BoardPhaseFilterDuplicateManifolds();
	void BoardPhaseFilterDuplicateManifoldsProcessObject(GameObject* obj, ID dispatchId);

public:
	// add object to begin physics processing (entry objects for MT board phase)
	void AddToBeginBoardPhase(GameObject* obj)
	{
		auto& entries = GetNextEntriesBoardPhase();
		entries.Add(obj);
	}

};

NAMESPACE_END