#pragma once

#include "Core/Memory/SmartPointers.h"

#include "PhysicsComponent.h"
#include "RigidBody.h"

NAMESPACE_BEGIN

class PhysicsShape;

class API RigidBodyStatic : public RigidBody
{
public:
	COMPONENT_CLASS(RigidBodyStatic);

	inline RigidBodyStatic() {};
	RigidBodyStatic(const SharedPtr<PhysicsShape>& shape);
	~RigidBodyStatic();

protected:
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
		return PHYSICS_TYPE_RIGID_BODY_STATIC;
	};

	void OnComponentAdded() override;

	void OnComponentRemoved() override;

	AABox GetGlobalAABB() override;

};

NAMESPACE_END