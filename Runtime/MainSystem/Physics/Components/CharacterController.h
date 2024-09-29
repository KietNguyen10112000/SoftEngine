#pragma once

#include "PhysicsComponent.h"
#include "RigidBody.h"

namespace physx
{
class PxController;
class PxQueryFilterCallback;
class PxShape;
}

NAMESPACE_BEGIN

class PhysicsQueryFilterCallback;

struct CharacterControllerDesc
{
	SharedPtr<PhysicsMaterial> material;

	float slopeLimit = 0.707f;
	float stepOffset = 0.5f;
	float contactOffset = 0.1f;

private:
	friend class CharacterControllerCapsule;
	void ToPxDesc(void* pxDesc);

	void ToJson(Serializer* s, json& j);
	void FromJson(Serializer* s, const json& j);

};

class API CharacterController : public RigidBody
{
private:
	friend class PhysXSimulationFilterCallback;
	friend class PhysXSimulationCallback;
	friend class PhysicsSystem;
	friend class AnimatorSkeletalArray;
	friend class CharacterControllerHitCallback;
	friend class CCTDefaultFilterCallBack;
	
public:
	struct CollisionPlane
	{
		Vec3 position;
		Vec3 normal;

		PhysicsShape* shape = nullptr;
		GameObject* object = nullptr;

		bool isGround = false;

		inline bool TestGround(const Vec3& dir) const
		{
			if (dir.Length2() == 0)
			{
				return false;
			}

			return dir.Normal().Dot(normal) < -0.0001f;
		}
	};

	struct CollisionPlanes
	{
		std::vector<CollisionPlane> planes;
		size_t groundCount = 0;
	};

protected:
	//Mat4 m_lastGlobalTransform;

	CharacterControllerDesc* m_pDerivedDesc = nullptr;

	physx::PxController* m_pxCharacterController = nullptr;
	Quaternion m_rotation = {};
	Quaternion m_additionRotation = {};
	Quaternion m_lastRotation = {};

	Vec3 m_gravity = Vec3::ZERO;
	Vec3 m_velocity = Vec3::ZERO;
	Vec3 m_committedVelocity = Vec3::ZERO;

	float m_overrideGravityStaticFriction = 0.0f;
	float m_overrideGravityDynamicFriction = 0.0f;
	float m_overrideVelocityStaticFriction = 0.0f;
	float m_overrideVelocityDynamicFriction = 0.0f;

	Vec3 m_sumDisp = Vec3::ZERO;
	size_t m_lastMoveIterationCount = 0;

	DeferredBuffer<CollisionPlanes> m_collisionPlanesBuffer;

	float m_mass = 1;

	bool m_isOnGround = false;
	bool m_isEnableGravity = false;
	bool m_isEnableAdditionRotation = false;

	mutable spinlock m_lock;

	//size_t m_contributeVelocityToPositionIterationCount = 0;

	int m_countScheduleUpdate = 0; 

	physx::PxQueryFilterCallback* m_defaultCCTFilterCallback = nullptr;

	SharedPtr<PhysicsQueryFilterCallback> m_filterCallback = nullptr;

	CharacterController();
	~CharacterController();

private:
	static void TransformContributor(GameObject* object, Transform& local, Mat4& global, void* self);

	void ReduceVelocityByCollisionPlanes(float dt);
	void ApplyGravity(float dt);
	void ApplyAditionRotation(float dt);

	void CCTSetRotationImpl(const Quaternion& rotation);

protected:
	virtual void Wake() override;
	virtual void OnPhysicsTransformChanged() override;

	virtual void OnUpdate(float dt);
	virtual void OnPrevUpdate(float dt);

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

public:
	void OnTransformChanged() override;

	inline virtual PHYSICS_TYPE GetPhysicsType() const
	{
		return PHYSICS_TYPE_CHARACTER_CONTROLLER;
	}

public:
	void Move(const Vec3& disp);

	void SetGravity(const Vec3& g);
	void SetGravityEnabled(bool enable);

	void CCTSetVelocity(const Vec3& velocity);
	Vec3 CCTGetVelocity() const;
	void CCTApplyVelocity(const Vec3& velocity);

	void CCTApplyImpulse(const Vec3& impulse);
	bool CCTIsOnGround();
	//void CCTSetContactFilterCallback(RigidBody::ContactReportFilterCallback callback);

	void CCTSetRotation(const Quaternion& rotation);

	void CCTSetAdditionRotationEnabled(bool enable);
	void CCTSetAdditionRotation(const Quaternion& rotation);

	// return the collision planes that the CCT is currenly in contact
	const CollisionPlanes& CCTGetCollisionPlanes();

	inline const auto& CCTGetRotation() const
	{
		return m_lastRotation;
	}

	Vec3 GetGravity() const;
	Vec3 GetVelocity() const;

public:
	void CCTSetSlopeLimit(float cosAngle);
	float CCTGetSlopeLimit() const;

	void CCTSetStepOffset(float stepOffset);
	float CCTGetStepOffset() const;

	void CCTSetContactOffset(float contactOffset);
	float CCTGetContactOffset() const;

	void CCTSetFilterCallback(const SharedPtr<PhysicsQueryFilterCallback>& filter);

};

NAMESPACE_END