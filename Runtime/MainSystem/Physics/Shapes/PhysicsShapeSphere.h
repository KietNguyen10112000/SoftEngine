#pragma once

#include "PhysicsShape.h"

#include "Math/Math.h"

#include "Core/Memory/SmartPointers.h"

NAMESPACE_BEGIN

class PhysicsMaterial;

class PhysicsShapeSphere : public PhysicsShape
{
	SERIALIZABLE_CLASS(PhysicsShapeSphere, SERIALIZABLE_MEM_SHARED);
public:
	inline PhysicsShapeSphere() {};
	PhysicsShapeSphere(float radius, const SharedPtr<PhysicsMaterial>& material);

	inline virtual PHYSICS_SHAPE_TYPE GetType() const 
	{
		return PHYSICS_SHAPE_TYPE_SPHERE;
	};

	// Inherited via PhysicsShape
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;
	Handle<ClassMetadata> GetMetadata(size_t sign) override;
	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;
};

NAMESPACE_END