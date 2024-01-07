#pragma once

#include "PhysicsComponent.h"

NAMESPACE_BEGIN

class RigidBody : public PhysicsComponent
{
protected:
	Mat4 m_lastGlobalTransform;

	inline RigidBody() {};

public:
	void OnTransformChanged() override;

};

NAMESPACE_END