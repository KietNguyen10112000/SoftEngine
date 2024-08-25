#pragma once

#include "PhysicsComponent.h"
#include "RigidBody.h"

namespace physx
{
class PxController;
class PxQueryFilterCallback;
}

NAMESPACE_BEGIN

class API CharacterController : public RigidBody
{
private:
	friend class PhysXSimulationFilterCallback;
	friend class PhysXSimulationCallback;
	friend class PhysicsSystem;
	friend class AnimatorSkeletalArray;
	//RigidBody::ContactReportFilterCallback m_contactFilterCallback = nullptr;

protected:

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

	//Mat4 m_lastGlobalTransform;

	physx::PxController* m_pxCharacterController = nullptr;
	Quaternion m_rotation = {};
	Quaternion m_lastRotation = {};

	Vec3 m_gravity = Vec3::ZERO;
	Vec3 m_velocity = Vec3::ZERO;

	Vec3 m_sumF = Vec3::ZERO;

	Vec3 m_sumDisp[2] = { Vec3::ZERO, Vec3::ZERO };
	Vec3 m_lastDisp = Vec3::ZERO;
	size_t m_lastMoveIterationCount = 0;

	std::vector<CollisionPlane> m_collisionPlanes;

	float m_mass = 1;

	bool m_isOnGround = false;

	//size_t m_contributeVelocityToPositionIterationCount = 0;

	int m_countScheduleUpdate = 0;

	physx::PxQueryFilterCallback* m_defaultCCTFilterCallback = nullptr;

	void (*m_animationMotionMatchingCallback)(AnimatorSkeletalArray*, ID) = nullptr;
	AnimatorSkeletalArray* m_animationMotionMatchingCallbackAnimator = nullptr;
	ID m_animationMotionMatchingCallbackParam = INVALID_ID;

	CharacterController();
	~CharacterController();

private:
	static void TransformContributor(GameObject* object, Transform& local, Mat4& global, void* self);

	void RunAnimatorMotionMatchingCallback(void(*callback)(AnimatorSkeletalArray*, ID), AnimatorSkeletalArray* animator, ID _param);

	//bool IsHasNextMove();

protected:
	virtual void Wake() override;
	virtual void OnPhysicsTransformChanged() override;

	virtual void OnUpdate(float dt);
	virtual void OnPrevUpdate(float dt);

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

	void CCTApplyVelocity(const Vec3& velocity);
	void CCTApplyImpulse(const Vec3& impulse);
	bool CCTIsOnGround();
	//void CCTSetContactFilterCallback(RigidBody::ContactReportFilterCallback callback);

	void CCTSetRotation(const Quaternion& rotation);

	inline const auto& CCTGetRotation() const
	{
		return m_lastRotation;
	}

};

NAMESPACE_END