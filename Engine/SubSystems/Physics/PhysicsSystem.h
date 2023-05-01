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
	raw::ConcurrentArrayList<Manifold*> m_aliveManifolds;

	TaskWaitingHandle m_filterAliveManifoldsHandle = { 0,0 };

	// when multithreaded traserval over scene struct, 2 thread can touch same object
	raw::ConcurrentArrayList<GameObject*> m_boardPhaseDuplicateManifoldObjs;

	std::atomic<ID> m_manifoldIDCount = { 0 };
	raw::ConcurrentArrayList<Manifold*> m_allocatedManifolds;
	ConcurrentQueue<Manifold*> m_freeManifolds;

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

	virtual bool FilterAddSubSystemComponent(SubSystemComponent* comp) override;

	virtual bool FilterRemoveSubSystemComponent(SubSystemComponent* comp) override;

private:
	inline Manifold* AllocateManifold(Physics* A, Physics* B)
	{
		Manifold* ret;
		if (m_freeManifolds.try_dequeue(ret))
		{
			goto Return;
		}

		ret = rheap::New<Manifold>();
		m_allocatedManifolds.Add(ret);
		ret->m_id = ++m_manifoldIDCount;

	Return:
		ret->m_A = A;
		ret->m_B = B;
		ret->m_refCount.store(2, std::memory_order_relaxed);
		ret->refManifold = nullptr;

		m_aliveManifolds.Add(ret);
		return ret;
	}

	inline void DeallocateManifold(Manifold* m)
	{
		assert(m->m_refCount.load(std::memory_order_relaxed) >= 1);
		if ((--(m->m_refCount)) == 0)
		{
			m_freeManifolds.enqueue(m);
		}
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

	void BoardPhaseFilterAliveManifolds();

	void BoardPhaseProcessCtx(ID dispatchId, BoardPhaseCtx& ctx, SceneQuerySession* querySession);
	void BoardPhaseFilterDuplicateManifolds();
	void BoardPhaseFilterDuplicateManifoldsProcessObject(GameObject* obj, ID dispatchId);

public:
	// add object to begin physics processing (entry objects for MT board phase)
	void AddToBeginBoardPhase(GameObject* obj)
	{
		auto& entries = GetNextEntriesBoardPhase();
		entries.Add(obj);

		/*if (entries.size() > 101)
		{
			int x = 3;
		}*/
	}

};

NAMESPACE_END