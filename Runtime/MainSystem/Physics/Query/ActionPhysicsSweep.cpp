#include "ActionPhysicsSweep.h"

#include "MainSystem/Physics/PhysicsSystem.h"

NAMESPACE_BEGIN

ActionPhysicsSweep::ActionPhysicsSweep(PhysicsSystem* sys, size_t activeIteration) : ActionPhysicsQuery(sys, activeIteration)
{

}

void ActionPhysicsSweep::CallCallback()
{
	if (m_executedQuery)
	{
		m_callback(this, m_result);
	}
}

void ActionPhysicsSweep::ExecuteQuery()
{
	{
		m_queryStatus = m_system->SweepImpl(m_result, m_shape.get(), GetStartTransform(), m_sweepDistance, m_filter.get());
		m_executedQuery = true;
	}
}

NAMESPACE_END