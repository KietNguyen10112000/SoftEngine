#include "ActionPhysicsQuery.h"

#include "../PhysicsSystem.h"

NAMESPACE_BEGIN

ActionPhysicsQuery::ActionPhysicsQuery(PhysicsSystem* sys, size_t activeIteration) : m_system(sys), m_activeIteration(activeIteration)
{
}

ActionBase::RETURN_CODE ActionPhysicsQuery::Update(float dt)
{
	if (m_system == nullptr)
	{
		return RETURN_CODE::FINISHED;
	}

	if (m_system->GetScene()->GetIterationCount() != m_activeIteration)
	{
		return RETURN_CODE::NONE;
	}

	CallCallback();

	m_system = nullptr;
	return RETURN_CODE::FINISHED;
}

NAMESPACE_END