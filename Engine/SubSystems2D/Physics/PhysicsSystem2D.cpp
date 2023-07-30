#include "PhysicsSystem2D.h"

#include "Core/TemplateUtils/TemplateUtils.h"

#include "Components2D/Script/Script2D.h"

#include "Components2D/Physics/Physics2D.h"

#include <iostream>

NAMESPACE_BEGIN

PhysicsSystem2D::PhysicsSystem2D(Scene2D* scene) : SubSystem2D(scene, Script2D::COMPONENT_ID)
{
	m_querySession = scene->NewQuerySession();
}

PhysicsSystem2D::~PhysicsSystem2D()
{
	m_scene->DeleteQuerySession(m_querySession);

	for (auto& pair : m_allocatedPairs)
	{
		rheap::Delete(pair);
	}
}

void PhysicsSystem2D::PrevIteration(float dt)
{
}

void PhysicsSystem2D::Iteration(float dt)
{
	BoardPhase();
	NarrowPhase();
}

void PhysicsSystem2D::PostIteration(float dt)
{
}

void PhysicsSystem2D::AddSubSystemComponent(SubSystemComponent2D* comp)
{
	auto physics = (Physics2D*)comp;

	assert(physics->m_collider != nullptr);

	// no body means static object
	if (physics->m_TYPE != Physics2D::STATIC)
	{
		assert(physics->GetObject()->m_type != GameObject2D::STATIC);

		physics->m_id = m_boardPhaseEntries.size();
		m_boardPhaseEntries.push_back(physics);
	}
}

void PhysicsSystem2D::RemoveSubSystemComponent(SubSystemComponent2D* comp)
{
	auto physics = (Physics2D*)comp;
	for (auto& pairs : physics->m_collisionPairs)
	{
		for (auto& p : pairs)
		{
			if (p->A == physics)
			{
				p->cacheA = 0;
			}
			else
			{
				p->cacheB = 0;
			}
			//p->result.penetration = 0;
		}
		FreeCollisionPairs(pairs);
	}
	
	if (physics->m_TYPE != Physics2D::STATIC)
	{
		STD_VECTOR_ROLL_TO_FILL_BLANK(m_boardPhaseEntries, physics->m_id, back->m_id);
	}
}

void PhysicsSystem2D::BoardPhase()
{
	// free all things
	for (auto& physics : m_boardPhaseEntries)
	{
		physics->m_collisionPairsId = (physics->m_collisionPairsId + 1) % 2;
		FreeCollisionPairs(physics->CollisionPairs());
	}
	m_narrowPhaseEntries.clear();
	m_iterationCount++;


	for (auto& physics : m_boardPhaseEntries)
	{
		if (physics->m_lastBoardPhaseIterationCount == m_iterationCount)
		{
			continue;
		}

		m_boardPhaseStack.push_back(physics);
		//physics->m_lastBoardPhaseIterationCount = m_iterationCount;
		physics->m_isInStackCount = m_iterationCount;
		while (!m_boardPhaseStack.empty())
		{
			auto top = m_boardPhaseStack.back();

			//assert(top->m_lastBoardPhaseIterationCount == m_iterationCount);
			top->m_lastBoardPhaseIterationCount = m_iterationCount;

			m_boardPhaseStack.pop_back();

			auto& pairs = top->CollisionPairs();

			m_querySession->Clear();
			m_scene->AABBStaticQueryAARect(top->GetObject()->m_globalAABB, m_querySession);
			for (auto& obj : *m_querySession)
			{
				auto another = obj->GetComponentRaw<Physics2D>();

				if (!another) continue;

				if (another->CollisionMask() & top->CollisionMask())
				{
					pairs.push_back(AllocateCollisionPair(top, another, 1));
				}
			}

			m_querySession->Clear();
			m_scene->AABBDynamicQueryAARect(top->GetObject()->m_globalAABB, m_querySession);
			for (auto& obj : *m_querySession)
			{
				auto another = obj->GetComponentRaw<Physics2D>();

				//if (another == top) continue;
				if (!another) continue;

				if (another->m_lastBoardPhaseIterationCount == m_iterationCount)
				{
					continue;
				}
				//another->m_lastBoardPhaseIterationCount = m_iterationCount;

				if (another->CollisionMask() & top->CollisionMask())
				{
					auto pair = AllocateCollisionPair(top, another, 2);
					pairs.push_back(pair);
					another->CollisionPairs().push_back(pair);

					if (another->m_isInStackCount != m_iterationCount) 
					{
						m_boardPhaseStack.push_back(another);
						another->m_isInStackCount = m_iterationCount;
					}
				}
			}
		}
	}
}

void PhysicsSystem2D::SolveAllCollisionPairs()
{
	for (auto& pair : m_narrowPhaseEntries)
	{
		auto A = pair->A;
		auto B = pair->B;
		B->m_collider->Collide(
			B->GetObject()->GlobalTransformMatrix(),
			A->m_collider.get(),
			A->GetObject()->GlobalTransformMatrix(),
			pair->result
		);
	}
}

void PhysicsSystem2D::NarrowPhase()
{
	auto dt = m_scene->Dt();
	SolveAllCollisionPairs();

	for (auto& physics : m_boardPhaseEntries)
	{
		if (!physics->IsDisabled() && !physics->CollisionPairs().empty())
		{
			physics->ReactCollisionPairs();
		}
	}

	for (auto& physics : m_boardPhaseEntries)
	{
		if (!physics->IsDisabled())
		{
			physics->NarrowPhase(dt);
		}
	}
}

NAMESPACE_END
