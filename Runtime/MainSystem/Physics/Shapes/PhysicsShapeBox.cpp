#include "PhysicsShapeBox.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Materials/PhysicsMaterial.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Physics/Components/RigidBody.h"
#include "MainSystem/Physics/PhysicsSystem.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "PhysicsShapeUtils.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShapeBox::PhysicsShapeBox()
{
}

PhysicsShapeBox::PhysicsShapeBox(const Vec3& dimensions, const SharedPtr<PhysicsMaterial>& material)
{
	PhysicsShapeUtils::InitializeShape<PxBoxGeometry>(this, material, false, dimensions.x / 2.0f, dimensions.y / 2.0f, dimensions.z / 2.0f);
}

void PhysicsShapeBox::CloneFrom(Serializer* serializer, Serializable* another)
{
	auto src = (PhysicsShapeBox*)another;
	assert(m_pxShape == nullptr);

	auto material = serializer->Clone(src->m_meterial);

	auto dimensions = src->GetDimensions();
	PhysicsShapeUtils::InitializeShape<PxBoxGeometry>(
		this, material, false, dimensions.x / 2.0f, dimensions.y / 2.0f, dimensions.z / 2.0f);

	PhysicsShape::CloneFrom(serializer, another);
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

	Vec3 dimensions = j["Dimensions"];
	PhysicsShapeUtils::InitializeShape<PxBoxGeometry>(
		this, GetDeserializedMaterial(serializer, j), false, dimensions.x / 2.0f, dimensions.y / 2.0f, dimensions.z / 2.0f);
	PhysicsShape::DeserializeFromJson(serializer, j);
}

Handle<ClassMetadata> PhysicsShapeBox::GetMetadata(size_t sign)
{
	auto metadata = PhysicsShape::GetMetadata(sign + 1);
	metadata->SetName(GetClassName());

	metadata->AddProperty(Accessor(
		"Dimensions",
		this,
		[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
		{
			auto* self = (PhysicsShapeBox*)instance;
			auto& dimensions = input.As<Vec3>();
			self->SetDimensions(dimensions);
		},
		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto* self = (PhysicsShapeBox*)instance;
			return Variant::Of(self->GetDimensions());
		},
		this
	));

	return metadata;
}

void PhysicsShapeBox::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void PhysicsShapeBox::SetDimensions(const Vec3& dimensions)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(m_attachedRigidBody, PhysicsSystem, AsyncTaskRunnerST, dimensions, 
		{
			auto box = (PxBoxGeometry*)&self->m_pxShape->getGeometry();
			box->halfExtents = PhysXUtils::ToPxVec3(dimensions / 2.0f);
		}
	);
}

Vec3 PhysicsShapeBox::GetDimensions() const
{
	auto box = (PxBoxGeometry*)&m_pxShape->getGeometry();
	return PhysXUtils::ToVec3(box->halfExtents * 2.0f);
}

NAMESPACE_END