#include "PhysicsShapePlane.h"

#include "PhysX/PhysX.h"

#include "../Materials/PhysicsMaterial.h"

#include "PhysicsShapeUtils.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShapePlane::PhysicsShapePlane(const SharedPtr<PhysicsMaterial>& material)
{
	PhysicsShapeUtils::InitializeShape<PxPlaneGeometry>(this, material, false);
}

void PhysicsShapePlane::CloneFrom(Serializer* serializer, Serializable* another)
{
	auto src = (PhysicsShapePlane*)another;
	assert(m_pxShape == nullptr);

	auto material = serializer->Clone(src->m_meterial);
	PhysicsShapeUtils::InitializeShape<PxPlaneGeometry>(this, material, false);
	PhysicsShape::CloneFrom(serializer, another);
}

void PhysicsShapePlane::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void PhysicsShapePlane::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void PhysicsShapePlane::SerializeToJson(Serializer* serializer, json& j) const
{
	PhysicsShape::SerializeToJson(serializer, j);
}

void PhysicsShapePlane::DeserializeFromJson(Serializer* serializer, const json& j)
{
	assert(m_pxShape == nullptr);
	PhysicsShapeUtils::InitializeShape<PxPlaneGeometry>(this, GetDeserializedMaterial(serializer, j), false);
	PhysicsShape::DeserializeFromJson(serializer, j);
}

Handle<ClassMetadata> PhysicsShapePlane::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void PhysicsShapePlane::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END