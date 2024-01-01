#pragma once

#include "PhysicsComponent.h"

NAMESPACE_BEGIN

class RigidBodyDynamic : public PhysicsComponent
{
public:
	COMPONENT_CLASS(RigidBodyDynamic);

	RigidBodyDynamic();
	~RigidBodyDynamic();

public:
	void OnTransformChanged() override;

};

NAMESPACE_END