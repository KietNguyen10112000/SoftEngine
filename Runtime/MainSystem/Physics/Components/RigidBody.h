#pragma once

#include "PhysicsComponent.h"

NAMESPACE_BEGIN

class RigidBody : public PhysicsComponent
{
protected:
	inline RigidBody() {};

public:
	void OnTransformChanged() override;

};

NAMESPACE_END