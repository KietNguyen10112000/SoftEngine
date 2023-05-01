#include "PhysicsSystem.h"

#include <iostream>

#include "Components/Physics/Physics.h"

#include "TaskSystem/TaskUtils.h"

#include "Engine/DebugVar.h"

#define Acquire(lock, value) lock.load(std::memory_order_relaxed) != value && lock.exchange(value) != value

NAMESPACE_BEGIN

PhysicsSystem::PhysicsSystem(Scene* scene) : SubSystem(scene, Physics::COMPONENT_ID)
{
	TaskSystem::PrepareHandle(&m_filterAliveManifoldsHandle);
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

	/*auto& entries = GetCurEntriesBoardPhase();
	for (auto& obj : m_rootObjects)
	{
		entries.Add(obj);
	}*/
}

void PhysicsSystem::Iteration(float dt)
{
	TaskSystem::WaitForHandle(&m_filterAliveManifoldsHandle);
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
	physics->m_numClearManifold = m_iterationCount;

	auto refreshed = physics->m_isRefreshed;

	if (refreshed)
	{
		return;
	}

	auto read = physics->m_manifolds.GetReadHead();

	auto aabb = physics->GetBoardPhaseAABB();
	read->ForEach(
		[&](Manifold* manifold) 
		{
			auto another = manifold->m_A == physics ? manifold->m_B : manifold->m_A;
			if (!another->m_isRefreshed)
			{
				// if another object in collision pair still not moving, keep it as cache
				++(manifold->m_refCount);
				write->Add(manifold);
			}
		}
	);
}

void PhysicsSystem::NewPhysicsIteration()
{
	auto iteration = m_iterationCount;
	TaskUtils::ForEachConcurrentListAsRingBuffer(
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

	TaskUtils::ForEachConcurrentListAsRingBuffer(
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

#define ClearManifoldFirstTime(physics, value)								\
if (physics->m_numClearManifold != value)									\
{																			\
	physics->m_clearManifoldLock.lock();									\
	if (physics->m_numClearManifold != value)								\
	{																		\
		auto head = physics->m_manifolds.GetWriteHead();					\
		head->ForEach([&](Manifold* m) { DeallocateManifold(m); });			\
		head->Clear();														\
		physics->m_numClearManifold = value;								\
	}																		\
	physics->m_clearManifoldLock.unlock();									\
}

void PhysicsSystem::BoardPhaseProcessCtx(ID dispatchId, BoardPhaseCtx& ctx, SceneQuerySession* querySession)
{
	auto& curOutput = GetCurBoardPhaseOutputObjs();
	auto& stack = ctx.stack;
	while (!stack.empty())
	{
		auto physics = stack.back();
		curOutput.Add(physics->GetObject());
		stack.pop_back();

		ClearManifoldFirstTime(physics, m_iterationCount);

		auto& manifolds = *physics->m_manifolds.GetWriteHead();

		querySession->Clear();
		m_scene->AABBDynamicQueryAABox(physics->GetBoardPhaseAABB(), querySession);

		auto it = querySession->begin;
		auto end = querySession->end;
		while (it != end)
		{
			auto gameObject = *it;

			auto anotherPhysics = gameObject->GetComponentRaw<Physics>();
			if (anotherPhysics && anotherPhysics != physics)
			{
				bool push = true;
				if (Acquire(anotherPhysics->m_numAcquiredBoardPhase, m_iterationCount))
				{
					anotherPhysics->m_processedBoardPhaseDispatchId = dispatchId;
					stack.push_back(anotherPhysics);
				}
				else
				{
					if (anotherPhysics->m_numProcessedBoardPhase.load(std::memory_order_relaxed) == m_iterationCount)
					{
						push = false;
					}
					else if (dispatchId != anotherPhysics->m_processedBoardPhaseDispatchId 
						&& Acquire(anotherPhysics->m_numFilterDuplBoardPhase, m_iterationCount))
					{
						m_boardPhaseDuplicateManifoldObjs.Add(anotherPhysics->GetObject());
					}
				}

				if (push)
				{
					auto m = AllocateManifold(physics, anotherPhysics);
					manifolds.Add(m);

					ClearManifoldFirstTime(anotherPhysics, m_iterationCount);
					anotherPhysics->m_manifolds.GetWriteHead()->Add(m);

					//m_boardPhaseManifolds.Add(m);
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
	physics->m_processedBoardPhaseDispatchId = m_iterationCount + dispatchId;
	BoardPhaseProcessCtx(m_iterationCount + dispatchId, ctx, querySession);
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
	TaskUtils::ForEachConcurrentListAsRingBuffer(
		m_boardPhaseDuplicateManifoldObjs, 
		[&](GameObject* obj, ID dispatchId) 
		{
			auto physics = obj->GetComponentRaw<Physics>();
			bool isNotProcessed = Acquire(physics->m_numFilterDuplBoardPhase, iteration);

			if (isNotProcessed)
			{
				BoardPhaseFilterDuplicateManifoldsProcessObject(obj, dispatchId);
			}

			return isNotProcessed;
		},
		TaskSystem::GetWorkerCount()
	);
}

void PhysicsSystem::BoardPhaseFilterAliveManifolds()
{
	TaskUtils::ForEachConcurrentList(
		m_aliveManifolds, 
		[&](Manifold* m, ID) 
		{
			if (m->m_refCount.load(std::memory_order_relaxed) != 0)
			{
				m_boardPhaseManifolds.Add(m);
			}
		}, 
		TaskSystem::GetWorkerCount()
	);

	DebugVar::Get().debugVar1 = m_aliveManifolds.size();

	// filter all alive manifolds
	Task task;
	task.Params() = this;
	task.Entry() = [](void* p)
	{
		auto physicsSystem = (PhysicsSystem*)p;
		auto& list = physicsSystem->m_aliveManifolds;
		auto& buffer = list.m_buffer;

		auto size = list.m_size.load(std::memory_order_relaxed);
		for (size_t i = 0; i < size; i++)
		{
			auto& m = buffer[i];
			if (m->m_refCount.load(std::memory_order_relaxed) == 0)
			{
				if (size == 1)
				{
					size = 0;
					break;
				}
				size--;
				m = buffer[size];
				i--;
				continue;
			}
		}

		list.m_size = size;
	};

	TaskSystem::PrepareHandle(&m_filterAliveManifoldsHandle);
	TaskSystem::Submit(&m_filterAliveManifoldsHandle, task, Task::CRITICAL);
}

void PhysicsSystem::BoardPhase()
{
	auto iteration = m_iterationCount;
	auto& entries = GetCurEntriesBoardPhase();
	TaskUtils::ForEachConcurrentListAsRingBuffer(
		entries, 
		[&](GameObject* obj, ID dispatchId) 
		{
			auto physics = obj->GetComponentRaw<Physics>();
			bool isNotProcessed = Acquire(physics->m_numBoardPhase, iteration);

			if (isNotProcessed)
			{
				BoardPhaseProcessObject(obj, dispatchId);
			}

			return isNotProcessed;
		},
		TaskSystem::GetWorkerCount()
	);

	//int x = 3;
	BoardPhaseFilterDuplicateManifolds();
	BoardPhaseFilterAliveManifolds();
}

void PhysicsSystem::NarrowPhase()
{
	m_boardPhaseManifolds.ForEach(
		[&](Manifold* manifold) 
		{
			if (manifold->refManifold)
			{
				return;
			}

			manifold->m_A->m_debugColor = Vec3(1, 0, 0);
			manifold->m_A->m_debugIteration = m_iterationCount;
			manifold->m_B->m_debugColor = Vec3(1, 0, 0);
			manifold->m_B->m_debugIteration = m_iterationCount;
		}
	);

	for (auto& obj : m_rootObjects)
	{
		auto physics = obj->GetComponentRaw<Physics>();
		if (physics->m_debugIteration != m_iterationCount)
			physics->m_debugColor = { 0,1,0 };
	}
}

void PhysicsSystem::EndPhysicsIteration()
{
	TaskUtils::ForEachConcurrentListAsRingBuffer(
		GetPrevBoardPhaseOutputObjs(),
		[&](GameObject* obj, ID dispatchId)
		{
			auto physics = obj->GetComponentRaw<Physics>();
			bool isNotProcessed = Acquire(physics->m_numBeginSetup, -1);

			if (isNotProcessed)
			{
				physics->m_manifolds.UpdateReadWriteHead(m_scene->GetIterationCount());
			}

			return isNotProcessed;
		},
		TaskSystem::GetWorkerCount()
	);

	TaskUtils::ForEachConcurrentListAsRingBuffer(
		GetCurBoardPhaseOutputObjs(),
		[&](GameObject* obj, ID dispatchId)
		{
			auto physics = obj->GetComponentRaw<Physics>();
			bool isNotProcessed = Acquire(physics->m_numBeginSetup, -1);

			if (isNotProcessed)
			{
				physics->m_manifolds.UpdateReadWriteHead(m_scene->GetIterationCount());
			}

			return isNotProcessed;
		},
		TaskSystem::GetWorkerCount()
	);
}

bool PhysicsSystem::FilterAddSubSystemComponent(SubSystemComponent* comp)
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

bool PhysicsSystem::FilterRemoveSubSystemComponent(SubSystemComponent* comp)
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

NAMESPACE_END