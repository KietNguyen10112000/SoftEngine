#include "PhysicsShapeSphere.h"

#include "PhysX/PhysX.h"

#include "../Materials/PhysicsMaterial.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Physics/Components/RigidBody.h"
#include "MainSystem/Physics/PhysicsSystem.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "PhysicsShapeUtils.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShapeSphere::PhysicsShapeSphere(float radius, const SharedPtr<PhysicsMaterial>& material)
{
	PhysicsShapeUtils::InitializeShape<PxSphereGeometry>(this, material, false, radius);
}

void PhysicsShapeSphere::CloneFrom(Serializer* serializer, Serializable* another)
{
	auto src = (PhysicsShapeSphere*)another;
	assert(m_pxShape == nullptr);

	auto material = serializer->Clone(src->m_meterial);
	PhysicsShapeUtils::InitializeShape<PxSphereGeometry>(this, material, false, src->GetRadius());
	PhysicsShape::CloneFrom(serializer, another);
}

void PhysicsShapeSphere::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void PhysicsShapeSphere::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void PhysicsShapeSphere::SerializeToJson(Serializer* serializer, json& j) const
{
	auto geo = (PxSphereGeometry*)&m_pxShape->getGeometry();
	j["Redius"] = geo->radius;
	PhysicsShape::SerializeToJson(serializer, j);
}

void PhysicsShapeSphere::DeserializeFromJson(Serializer* serializer, const json& j)
{
	assert(m_pxShape == nullptr);
	PhysicsShapeUtils::InitializeShape<PxSphereGeometry>(this, GetDeserializedMaterial(serializer, j), false, float(j["Radius"]));
	PhysicsShape::DeserializeFromJson(serializer, j);
}

Handle<ClassMetadata> PhysicsShapeSphere::GetMetadata(size_t sign)
{
	auto metadata = PhysicsShape::GetMetadata(sign + 1);
	metadata->SetName(GetClassName());

	metadata->AddProperty(Accessor(
		"Radius",
		this,
		[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
		{
			auto* self = (PhysicsShapeSphere*)instance;
			self->SetRadius(input.As<float>());
		},
		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto* self = (PhysicsShapeSphere*)instance;
			return Variant::Of(self->GetRadius());
		},
		this
	));

	return metadata;
}

void PhysicsShapeSphere::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void PhysicsShapeSphere::ScaleBy(float scale)
{
	auto r = GetRadius();
	SetRadius(r * scale);
}

void PhysicsShapeSphere::SetRadius(float r)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(m_attachedRigidBody, PhysicsSystem, AsyncTaskRunnerST, r,
		{
			auto capsule = (PxSphereGeometry*)&self->m_pxShape->getGeometry();
			capsule->radius = r;
		}
	);
}

float PhysicsShapeSphere::GetRadius() const
{
	auto capsule = (PxSphereGeometry*)&m_pxShape->getGeometry();
	return capsule->radius;
}

NAMESPACE_END