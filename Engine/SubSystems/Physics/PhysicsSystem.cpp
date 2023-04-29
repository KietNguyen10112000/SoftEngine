#include "PhysicsSystem.h"

#include <iostream>

#include "Components/Physics/Physics.h"

#include "TaskSystem/TaskUtils.h"

NAMESPACE_BEGIN

PhysicsSystem::PhysicsSystem(Scene* scene) : SubSystem(scene, Physics::COMPONENT_ID)
{
	auto count = TaskSystem::GetWorkerCount();
	for (size_t i = 0; i < count; i++)
	{
		m_querySessions[i] = m_scene->NewDynamicQuerySession();
	}
}

PhysicsSystem::~PhysicsSystem()
{
	m_allocatedManifolds.ForEach([](Manifold* m) {
			rheap::Delete(m);
		}
	);
	m_allocatedManifolds.Clear();
}

void PhysicsSystem::PrevIteration(float dt)
{
	m_iterationCount++;
	GetCurBoardPhaseOutputObjs().Clear();
	GetNextEntriesBoardPhase().Clear();
	m_boardPhaseDuplicateManifoldObjs.Clear();
	m_boardPhaseManifolds.Clear();

	auto& entries = GetCurEntriesBoardPhase();
	for (auto& obj : m_rootObjects)
	{
		entries.Add(obj);
	}

	m_freeManifoldsHead = m_freeManifolds.GetComsumeHead();
}

void PhysicsSystem::Iteration(float dt)
{
	NewPhysicsIteration();
	BoardPhase();
	NarrowPhase();
	EndPhysicsIteration();
}

void PhysicsSystem::PostIteration(float dt)
{
}

void PhysicsSystem::NewPhysicsIterationProcessObject(GameObject* obj, ID dispatchId)
{
	auto physics = obj->GetComponentRaw<Physics>();
	auto write = physics->m_manifolds.GetWriteHead();
	write->ForEach([&](Manifold* m)
		{
			DeallocateManifold(m);
		}
	);
	write->Clear();

	auto refreshed = physics->m_isRefreshed;

	if (refreshed)
	{
		return;
	}

	auto read = physics->m_manifolds.GetWriteHead();

	auto aabb = physics->GetBoardPhaseAABB();
	read->ForEach(
		[&](Manifold* manifold) 
		{
			auto another = manifold->m_A == physics ? manifold->m_B : manifold->m_A;
			if (!another->m_isRefreshed)
			{
				// if another object in collision pair still not moving, keep it as cache
				write->Add(manifold);
			}
		}
	);
}

void PhysicsSystem::NewPhysicsIteration()
{
	auto iteration = m_iterationCount;
	TaskUtils::ForEachConcurrentList(
		GetPrevBoardPhaseOutputObjs(),
		[&](GameObject* obj, ID dispatchId)
		{
			auto physics = obj->GetComponentRaw<Physics>();
			bool isNotProcessed =
				physics->m_numBeginSetup.load(std::memory_order_relaxed) != iteration
				&& physics->m_numBeginSetup.exchange(iteration) != iteration;

			if (isNotProcessed)
			{
				NewPhysicsIterationProcessObject(obj, dispatchId);
			}

			return isNotProcessed;
		},
		TaskSystem::GetWorkerCount()
	);

	TaskUtils::ForEachConcurrentList(
		GetPrevBoardPhaseOutputObjs(),
		[&](GameObject* obj, ID dispatchId)
		{
			auto physics = obj->GetComponentRaw<Physics>();
			bool isNotProcessed =
				physics->m_numBeginSetup.load(std::memory_order_relaxed) != 0
				&& physics->m_numBeginSetup.exchange(0) != 0;

			physics->m_isRefreshed = false;

			return isNotProcessed;
		},
		TaskSystem::GetWorkerCount()
	);
}

void PhysicsSystem::BoardPhaseProcessCtx(BoardPhaseCtx& ctx, SceneQuerySession* querySession)
{
	auto& curOutput = GetCurBoardPhaseOutputObjs();
	auto& stack = ctx.stack;
	while (!stack.empty())
	{
		auto physics = stack.back();
		curOutput.Add(physics->GetObject());
		stack.pop_back();

		auto& manifolds = *physics->m_manifolds.GetWriteHead();

		m_scene->AABBDynamicQueryAABox(physics->GetBoardPhaseAABB(), querySession);

		auto it = querySession->begin;
		auto end = querySession->end;
		while (it != end)
		{
			auto gameObject = *it;

			auto anotherPhysics = gameObject->GetComponentRaw<Physics>();
			if (anotherPhysics)
			{
				bool push = true;
				if (physics->m_numAcquiredBoardPhase.load(std::memory_order_relaxed) != m_iterationCount
					&& physics->m_numAcquiredBoardPhase.exchange(m_iterationCount) != m_iterationCount)
				{
					stack.push_back(anotherPhysics);
				}
				else
				{
					if (physics->m_numProcessedBoardPhase.load(std::memory_order_relaxed) != m_iterationCount
						&& physics->m_numProcessedBoardPhase.exchange(m_iterationCount) != m_iterationCount)
					{
						push = false;
					}
					else if (physics->m_numFilterDuplBoardPhase.load(std::memory_order_relaxed) != m_iterationCount
						&& physics->m_numFilterDuplBoardPhase.exchange(m_iterationCount) != m_iterationCount)
					{
						m_boardPhaseDuplicateManifoldObjs.Add(anotherPhysics->GetObject());
					}
				}

				if (push)
				{
					auto m = AllocateManifold();
					m->m_A = physics;
					m->m_B = anotherPhysics;
					manifolds.Add(m);
					anotherPhysics->m_manifolds.GetWriteHead()->Add(m);
					m_boardPhaseManifolds.Add(m);
				}
			}

			it++;
		}

		physics->m_numProcessedBoardPhase.store(m_iterationCount, std::memory_order_relaxed);
	}
	
}

void PhysicsSystem::BoardPhaseProcessObject(GameObject* obj, ID dispatchId)
{
	auto physics = obj->GetComponentRaw<Physics>();
	if (physics->m_numAcquiredBoardPhase.load(std::memory_order_relaxed) == m_iterationCount
		|| physics->m_numAcquiredBoardPhase.exchange(m_iterationCount) == m_iterationCount)
	{
		return;
	}

	auto& ctx = m_boardPhaseCtxs[dispatchId];
	auto querySession = m_querySessions[dispatchId].get();
	ctx.stack.push_back(physics);
	BoardPhaseProcessCtx(ctx, querySession);
}

void PhysicsSystem::BoardPhaseFilterDuplicateManifoldsProcessObject(GameObject* obj, ID dispatchId)
{
	auto physics = obj->GetComponentRaw<Physics>();
	auto& manifold = *physics->m_manifolds.GetWriteHead();
	
	auto begin = manifold.begin();
	auto end = manifold.end();
	std::sort(begin, end,
		[](Manifold* m1, Manifold* m2)
		{
			if (m1->m_id > m2->m_id && m1->m_id < m2->m_id)
			{
				int x = 3;
			}
			return m1->m_id > m2->m_id;
		}
	);

	Manifold* prev = nullptr;
	auto it = begin;
	while (it != end)
	{
		auto cur = *it;

		assert(cur != nullptr);

		if (cur == prev)
		{
			cur->refManifold = prev;
			it++;
			continue;
		}
		
		it++;
		prev = cur;
	}
}

void PhysicsSystem::BoardPhaseFilterDuplicateManifolds()
{
	auto iteration = m_iterationCount;
	TaskUtils::ForEachConcurrentList(
		m_boardPhaseDuplicateManifoldObjs, 
		[&](GameObject* obj, ID dispatchId) 
		{
			auto physics = obj->GetComponentRaw<Physics>();
			bool isNotProcessed =
				physics->m_numFilterDuplBoardPhase.load(std::memory_order_relaxed) != iteration
				&& physics->m_numFilterDuplBoardPhase.exchange(iteration) != iteration;

			if (isNotProcessed)
			{
				BoardPhaseFilterDuplicateManifoldsProcessObject(obj, dispatchId);
			}

			return isNotProcessed;
		},
		TaskSystem::GetWorkerCount()
	);
}

void PhysicsSystem::BoardPhase()
{
	auto iteration = m_iterationCount;
	auto& entries = GetCurEntriesBoardPhase();
	TaskUtils::ForEachConcurrentList(
		entries, 
		[&](GameObject* obj, ID dispatchId) 
		{
			auto physics = obj->GetComponentRaw<Physics>();
			bool isNotProcessed =
				physics->m_numBoardPhase.load(std::memory_order_relaxed) != iteration
				&& physics->m_numBoardPhase.exchange(iteration) != iteration;

			if (isNotProcessed)
			{
				BoardPhaseProcessObject(obj, dispatchId);
			}

			return isNotProcessed;
		},
		TaskSystem::GetWorkerCount()
	);

	BoardPhaseFilterDuplicateManifolds();
}

void PhysicsSystem::NarrowPhase()
{

}

void PhysicsSystem::EndPhysicsIteration()
{
	m_freeManifolds.UpdateFromConsumeHead(m_freeManifoldsHead);
	TaskUtils::ForEachConcurrentList(
		GetPrevBoardPhaseOutputObjs(),
		[&](GameObject* obj, ID dispatchId)
		{
			auto physics = obj->GetComponentRaw<Physics>();
			bool isNotProcessed =
				physics->m_numBeginSetup.load(std::memory_order_relaxed) != -1
				&& physics->m_numBeginSetup.exchange(-1) != -1;

			if (isNotProcessed)
			{
				physics->m_manifolds.UpdateReadWriteHead(m_scene->GetIterationCount());
			}

			return isNotProcessed;
		},
		TaskSystem::GetWorkerCount()
	);

	TaskUtils::ForEachConcurrentList(
		GetCurBoardPhaseOutputObjs(),
		[&](GameObject* obj, ID dispatchId)
		{
			auto physics = obj->GetComponentRaw<Physics>();
			bool isNotProcessed =
				physics->m_numBeginSetup.load(std::memory_order_relaxed) != -1
				&& physics->m_numBeginSetup.exchange(-1) != -1;

			if (isNotProcessed)
			{
				physics->m_manifolds.UpdateReadWriteHead(m_scene->GetIterationCount());
			}

			return isNotProcessed;
		},
		TaskSystem::GetWorkerCount()
	);
}

NAMESPACE_END