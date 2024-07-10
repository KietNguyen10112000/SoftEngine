#include "PhysicsShapeBox.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Materials/PhysicsMaterial.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShapeBox::PhysicsShapeBox()
{
}

PhysicsShapeBox::PhysicsShapeBox(const Vec3& dimensions, const SharedPtr<PhysicsMaterial>& material)
{
	auto physics = PhysX::Get()->GetPxPhysics();
	auto& m = *(material->m_pxMaterial);
	m_pxShape = physics->createShape(PxBoxGeometry(dimensions.x / 2.0f, dimensions.y / 2.0f, dimensions.z / 2.0f), m, false);

	m_pxShape->userData = this;
	m_meterial = material;
}

void PhysicsShapeBox::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void PhysicsShapeBox::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void PhysicsShapeBox::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void PhysicsShapeBox::SerializeToJson(Serializer* serializer, json& j) const
{
	auto pxBox = (PxBoxGeometry*)&m_pxShape->getGeometry();
	j["Dimensions"] = PhysXUtils::ToVec3(pxBox->halfExtents) * 2.0f;
	PhysicsShape::SerializeToJson(serializer, j);
}

void PhysicsShapeBox::DeserializeFromJson(Serializer* serializer, const json& j)
{
	assert(m_pxShape == nullptr);
	new (this) PhysicsShapeBox(j["Dimensions"], GetDeserializedMaterial(serializer, j));
	PhysicsShape::DeserializeFromJson(serializer, j);
}

Handle<ClassMetadata> PhysicsShapeBox::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void PhysicsShapeBox::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END