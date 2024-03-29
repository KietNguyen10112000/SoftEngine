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
	friend class PhysXSimulationCallback;

	struct CollisionPlane
	{
		Vec3 position;
		Vec3 normal;
		
		float staticFriction;
		float dynamicFriction;

		bool isGround;
		bool isApplyedDynamicFriction;
		bool isGroundForMotion = false;
	};

	Mat4 m_lastGlobalTransform;

	physx::PxController* m_pxCharacterController = nullptr;
	Vec3 m_gravity = Vec3::ZERO;
	Vec3 m_velocity = Vec3::ZERO;

	Vec3 m_sumF = Vec3::ZERO;

	Vec3 m_sumDisp[2] = { Vec3::ZERO, Vec3::ZERO };
	size_t m_lastMoveIterationCount = 0;

	std::vector<CollisionPlane> m_collisionPlanes;

	float m_mass = 1;

	bool m_isOnGround = false;

	//size_t m_contributeVelocityToPositionIterationCount = 0;

	inline CharacterController() {};
	~CharacterController();

private:
	static void TransformContributor(GameObject* object, Transform& local, Mat4& global, void* self);

	bool IsHasNextMove();

protected:
	virtual void OnPhysicsTransformChanged() override;

	virtual void OnUpdate(float dt);
	virtual void OnPostUpdate(float dt);

public:
	void OnTransformChanged() override;

	inline virtual PHYSICS_TYPE GetPhysicsType() const
	{
		return PHYSICS_TYPE_CHARACTER_CONTROLLER;
	}

public:
	void Move(const Vec3& disp);

	// to unset gravity, let g = { 0,0,0 }
	void SetGravity(const Vec3& g);

	void CCTApplyImpulse(const Vec3& impulse);
	bool CCTIsOnGround();

};

NAMESPACE_END