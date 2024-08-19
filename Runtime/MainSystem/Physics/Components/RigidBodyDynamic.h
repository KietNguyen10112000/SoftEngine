#pragma once

#include "Core/Memory/SmartPointers.h"

#include "PhysicsComponent.h"
#include "RigidBody.h"

NAMESPACE_BEGIN

class PhysicsShape;

class API RigidBodyDynamic : public RigidBody
{
protected:
	COMPONENT_CLASS(RigidBodyDynamic);

	friend class RigidBody;
	friend class Joint;
	friend class AnimatorSkeletalArray;

	byte m_isKinematic = 0;
	float m_density = 1.0f;

public:
	RigidBodyDynamic();
	RigidBodyDynamic(const SharedPtr<PhysicsShape>& shape);
	~RigidBodyDynamic();

private:
	static void TransformContributor(GameObject* object, Transform& local, Mat4& global, void* self);

protected:
	void InternalWake();
	virtual void Wake() override;
	virtual void OnPhysicsTransformChanged() override; 

	// Inherited via RigidBody
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;
	Handle<ClassMetadata> GetMetadata(size_t sign) override;
	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	void RunAnimatorMotionMatchingCallback(void (*callback)(AnimatorSkeletalArray*, ID), AnimatorSkeletalArray* animator, ID param);

public:
	inline virtual PHYSICS_TYPE GetPhysicsType() const 
	{
		return PHYSICS_TYPE_RIGID_BODY_DYNAMIC;
	};

public:

	void OnComponentAdded() override;

	void OnComponentRemoved() override;

	void OnTransformChanged() override;

	AABox GetGlobalAABB() override;

public:
	void SetDensity(float density);
	float GetDensity() const;
	float GetMass() const;

	void SetKinematic(bool enable);
	bool IsKinematic() const;

	void AddForce(const Vec3& f);
	void AddForceAtPos(const Vec3& f, const Vec3& pos);

	void AddImpulse(const Vec3& impulse);
	void AddImpulseAtPos(const Vec3& impulse, const Vec3& pos);

};

NAMESPACE_END