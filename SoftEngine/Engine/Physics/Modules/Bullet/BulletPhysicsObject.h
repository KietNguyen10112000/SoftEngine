#pragma once

#include "Engine/Physics/PhysicsObject.h"

#include "Math/Math.h"

#include "btBulletDynamicsCommon.h"

#include <memory>

class BulletPhysicsObject : public PhysicsObject
{
public:
	std::shared_ptr<btCollisionShape> m_shape;
	btCollisionObject* m_object = 0;

public:
	~BulletPhysicsObject();

	// Inherited via PhysicsObject
	virtual void SetTransform(const Mat4x4& transform) override;


	virtual Mat4x4 GetTransform() override;


	virtual void ApplyForce(const Vec3& position, const Vec3& force) override;


};