#pragma once

#include "Core/Memory/SmartPointers.h"

#include "PhysicsComponent.h"
#include "RigidBody.h"

NAMESPACE_BEGIN

class PhysicsShape;

class API RigidBodyDynamic : public RigidBody
{
public:
	byte m_isKinematic = 0;

	COMPONENT_CLASS(RigidBodyDynamic);

	inline RigidBodyDynamic() {};
	RigidBodyDynamic(const SharedPtr<PhysicsShape>& shape);
	~RigidBodyDynamic();

private:
	static void TransformContributor(GameObject* object, Transform& local, Mat4& global, void* self);

protected:
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
	float GetMass();

	void SetKinematic(bool enable);

	void AddForce(const Vec3& f);
	void AddForceAtLocalPos(const Vec3& f, const Vec3& pos);

};

NAMESPACE_END