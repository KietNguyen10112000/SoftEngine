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
	assert(0);
}

void PhysicsSystem2D::BoardPhase()
{
	// free all things
	for (auto& physics : m_boardPhaseEntries)
	{
		FreeCollisionPairs(physics->m_collisionPairs);
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
		while (!m_boardPhaseStack.empty())
		{
			auto top = m_boardPhaseStack.back();

			assert(top->m_lastBoardPhaseIterationCount != m_iterationCount);
			top->m_lastBoardPhaseIterationCount = m_iterationCount;

			m_boardPhaseStack.pop_back();

			auto& pairs = top->m_collisionPairs;

			m_querySession->Clear();
			m_scene->AABBStaticQueryAARect(top->GetObject()->m_globalAABB, m_querySession);
			for (auto& obj : *m_querySession)
			{
				auto another = obj->GetComponentRaw<Physics2D>();
				pairs.push_back(AllocateCollisionPair(top, another));
			}

			m_querySession->Clear();
			m_scene->AABBDynamicQueryAARect(top->GetObject()->m_globalAABB, m_querySession);
			for (auto& obj : *m_querySession)
			{
				auto another = obj->GetComponentRaw<Physics2D>();

				if (another->m_lastBoardPhaseIterationCount == m_iterationCount)
				{
					continue;
				}

				auto pair = AllocateCollisionPair(top, another);
				pairs.push_back(pair);
				another->m_collisionPairs.push_back(pair);
				m_boardPhaseStack.push_back(another);
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
			B->GetObject()->GlobalTransform(),
			A->m_collider.get(),
			A->GetObject()->GlobalTransform(), 
			pair->result
		);
	}
}

void PhysicsSystem2D::NarrowPhase()
{
	SolveAllCollisionPairs();

	for (auto& physics : m_boardPhaseEntries)
	{
		if (!physics->CollisionPairs().empty())
		{
			physics->ReactCollisionPairs();
		}
	}
}

NAMESPACE_END
