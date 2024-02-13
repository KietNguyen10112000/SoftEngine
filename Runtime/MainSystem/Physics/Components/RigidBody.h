#pragma once

#include "PhysicsComponent.h"

NAMESPACE_BEGIN

class PhysicsShape;

class RigidBody : public PhysicsComponent
{
protected:
	Mat4 m_lastGlobalTransform;

	std::vector<SharedPtr<PhysicsShape>> m_shapes;

	inline RigidBody() {};

public:
	void OnTransformChanged() override;

protected:
	virtual void OnDrawDebug() override;

};

NAMESPACE_END