#include "BulletPhysicsObject.h"

BulletPhysicsObject::~BulletPhysicsObject()
{
	delete m_object;
}

void BulletPhysicsObject::SetTransform(const Mat4x4& transform)
{
	btTransform trans;

	// see magic here
	auto* pos = (btVector3*)&transform.m[3][0];
	trans.setOrigin(*pos);

	auto* rotation = (btMatrix3x3*)&transform.m[0][0];
	trans.setBasis(*rotation);

	m_object->setWorldTransform(trans);
}

Mat4x4 BulletPhysicsObject::GetTransform()
{
	Mat4x4 transform;

	auto& trans = m_object->getWorldTransform();

	auto* rotation = (btMatrix3x3*)&transform.m[0][0];
	*rotation = trans.getBasis();

	auto* pos = (Vec3*)&trans.getOrigin();
	transform.SetPosition(*pos);

	transform.MulScaleComponent(m_scale);

	return transform;
}

void BulletPhysicsObject::ApplyForce(const Vec3& position, const Vec3& force)
{
}
