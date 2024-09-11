#include "ActionPhysicsSweep.h"

#include "MainSystem/Physics/PhysicsSystem.h"

NAMESPACE_BEGIN

ActionPhysicsSweep::ActionPhysicsSweep(PhysicsSystem* sys, size_t activeIteration) : m_system(sys), m_activeIteration(activeIteration)
{

}

ActionBase::RETURN_CODE ActionPhysicsSweep::Update(float dt)
{
	if (m_system->GetScene()->GetIterationCount() != m_activeIteration)
	{
		return RETURN_CODE::NONE;
	}

	m_callback(this, m_result);

	return RETURN_CODE::FINISHED;
}

NAMESPACE_END