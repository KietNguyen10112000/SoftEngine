#pragma once

#include "PhysicsShape.h"

#include "Math/Math.h"

#include "Core/Memory/SmartPointers.h"

NAMESPACE_BEGIN

class PhysicsMaterial;

class API PhysicsShapePlane : public PhysicsShape
{
	SERIALIZABLE_CLASS(PhysicsShapePlane, SERIALIZABLE_MEM_SHARED);
public:
	inline PhysicsShapePlane() {};
	PhysicsShapePlane(const SharedPtr<PhysicsMaterial>& material);

	inline virtual PHYSICS_SHAPE_TYPE GetType() const
	{
		return PHYSICS_SHAPE_TYPE_PLANE;
	};

	// Inherited via PhysicsShape
	void CloneFrom(Serializer* serializer, Serializable* another) override;
	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;
	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;
	void SerializeToJson(Serializer* serializer, json& j) const override;
	void DeserializeFromJson(Serializer* serializer, const json& j) override;
	Handle<ClassMetadata> GetMetadata(size_t sign) override;
	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

	virtual physx::PxGeometry* NewQueryGeometry(PxQueryGeometryDtor& dtor) const override;
	virtual void UpdateQueryGeometry(physx::PxGeometry*) const override;

	virtual void ScaleBy(float scale) override;
};

NAMESPACE_END