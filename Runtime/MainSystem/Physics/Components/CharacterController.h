#pragma once

#include "PhysicsComponent.h"

namespace physx
{
class PxController;
}

NAMESPACE_BEGIN

class CharacterController : public PhysicsComponent
{
protected:
	Mat4 m_lastGlobalTransform;

	physx::PxController* m_pxCharacterController = nullptr;

	inline CharacterController() {};
	~CharacterController();

private:
	static void TransformContributor(GameObject* object, Transform& local, Mat4& global, void* self);

protected:
	virtual void OnPhysicsTransformChanged() override;

public:
	void OnTransformChanged() override;

	inline virtual PHYSICS_TYPE GetPhysicsType() const
	{
		return PHYSICS_TYPE_CHARACTER_CONTROLLER;
	}

public:
	void Move(const Vec3& disp, float minDist);

};

NAMESPACE_END