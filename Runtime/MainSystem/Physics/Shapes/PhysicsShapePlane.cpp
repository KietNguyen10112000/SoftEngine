#include "PhysicsShapePlane.h"

#include "PhysX/PhysX.h"

#include "../Materials/PhysicsMaterial.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShapePlane::PhysicsShapePlane(const SharedPtr<PhysicsMaterial>& material)
{
	auto physics = PhysX::Get()->GetPxPhysics();
	auto& m = *(material->m_pxMaterial);
	m_pxShape = physics->createShape(PxPlaneGeometry(), m, false);

	m_pxShape->userData = this;
	m_meterial = material;
}

void PhysicsShapePlane::CloneFrom(Serializer* serializer, Serializable* another)
{
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
	this->~PhysicsShapePlane();
	new (this) PhysicsShapePlane(GetDeserializedMaterial(serializer, j));
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