#include "PhysicsComponent.h"

#include "PxPhysicsAPI.h"

NAMESPACE_BEGIN

PhysicsComponent::~PhysicsComponent() 
{
	if (m_pxActor)
	{
		m_pxActor->release();
		m_pxActor = nullptr;
	}
};

NAMESPACE_END