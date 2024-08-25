#include "PhysicsShapeCapsule.h"

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

PhysicsShapeCapsule::PhysicsShapeCapsule(void* pxShape, float h, float r, const SharedPtr<PhysicsMaterial>& material)
{
	m_pxShape = (PxShape*)pxShape;
	m_pxShape->acquireReference();
	m_pxShape->userData = this;
	m_meterial = material;
}

PhysicsShapeCapsule::PhysicsShapeCapsule(float h, float r, const SharedPtr<PhysicsMaterial>& material)
{
	PhysicsShapeUtils::InitializeShape<PxCapsuleGeometry>(this, material, false, r, h / 2.0f);
}

void PhysicsShapeCapsule::CloneFrom(Serializer* serializer, Serializable* another)
{
	auto src = (PhysicsShapeCapsule*)another;
	assert(m_pxShape == nullptr);

	auto material = serializer->Clone(src->m_meterial);
	PhysicsShapeUtils::InitializeShape<PxCapsuleGeometry>(this, material, false, src->GetRadius(), src->GetHeight() / 2.0f);
	PhysicsShape::CloneFrom(serializer, another);
}

void PhysicsShapeCapsule::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void PhysicsShapeCapsule::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void PhysicsShapeCapsule::SerializeToJson(Serializer* serializer, json& j) const
{
	auto geo = (PxCapsuleGeometry*)&m_pxShape->getGeometry();
	j["Height"] = geo->halfHeight * 2.0f;
	j["Radius"] = geo->radius;
	PhysicsShape::SerializeToJson(serializer, j);
}

void PhysicsShapeCapsule::DeserializeFromJson(Serializer* serializer, const json& j)
{
	assert(m_pxShape == nullptr);
	PhysicsShapeUtils::InitializeShape<PxCapsuleGeometry>(this, GetDeserializedMaterial(serializer, j), false, float(j["Radius"]), float(j["Height"]) / 2.0f);
	PhysicsShape::DeserializeFromJson(serializer, j);
}

Handle<ClassMetadata> PhysicsShapeCapsule::GetMetadata(size_t sign)
{
	auto metadata = PhysicsShape::GetMetadata(sign + 1);
	metadata->SetName(GetClassName());

	metadata->AddProperty(Accessor(
		"Height",
		this,
		[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
		{
			auto* self = (PhysicsShapeCapsule*)instance;
			self->SetHeight(input.As<float>());
		},
		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto* self = (PhysicsShapeCapsule*)instance;
			return Variant::Of(self->GetHeight());
		},
		this
	));

	metadata->AddProperty(Accessor(
		"Radius",
		this,
		[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
		{
			auto* self = (PhysicsShapeCapsule*)instance;
			self->SetRadius(input.As<float>());
		},
		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto* self = (PhysicsShapeCapsule*)instance;
			return Variant::Of(self->GetRadius());
		},
		this
	));

	return metadata;
}

void PhysicsShapeCapsule::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void PhysicsShapeCapsule::ScaleBy(float scale)
{
	auto r = GetRadius();
	auto h = GetHeight();
	SetRadius(r * scale);
	SetHeight(h * scale);
}

void PhysicsShapeCapsule::SetHeight(float h)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(m_attachedRigidBody, PhysicsSystem, AsyncTaskRunnerST, h,
		{
			auto capsule = (PxCapsuleGeometry*)&self->m_pxShape->getGeometry();
			capsule->halfHeight = h / 2.0f;
			self->RecalculateMass();
		}
	);
}

float PhysicsShapeCapsule::GetHeight() const
{
	auto capsule = (PxCapsuleGeometry*)&m_pxShape->getGeometry();
	return capsule->halfHeight * 2.0f;
}

void PhysicsShapeCapsule::SetRadius(float r)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(m_attachedRigidBody, PhysicsSystem, AsyncTaskRunnerST, r,
		{
			auto capsule = (PxCapsuleGeometry*)&self->m_pxShape->getGeometry();
			capsule->radius = r;
			self->RecalculateMass();
		}
	);
}

float PhysicsShapeCapsule::GetRadius() const
{
	auto capsule = (PxCapsuleGeometry*)&m_pxShape->getGeometry();
	return capsule->radius;
}

NAMESPACE_END