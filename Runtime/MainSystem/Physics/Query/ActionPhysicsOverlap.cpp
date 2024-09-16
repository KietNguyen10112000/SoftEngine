#include "ActionPhysicsOverlap.h"

#include "MainSystem/Physics/PhysicsSystem.h"

NAMESPACE_BEGIN

ActionPhysicsOverlap::ActionPhysicsOverlap(PhysicsSystem* sys, size_t activeIteration) : ActionPhysicsQuery(sys, activeIteration)
{

}

void ActionPhysicsOverlap::CallCallback()
{
	if (m_executedQuery)
	{
		m_callback(this, m_result);
	}
}

void ActionPhysicsOverlap::ExecuteQuery()
{
	{
		m_queryStatus = m_system->OverlapImpl(m_result, m_shape.get(), GetTransform(), m_filter.get());
		m_executedQuery = true;
	}
}

NAMESPACE_END