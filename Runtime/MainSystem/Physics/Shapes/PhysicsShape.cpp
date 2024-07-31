#include "PhysicsShape.h"

#include "PxPhysicsAPI.h"

#include "PhysX/Utils.h"

#include "../Materials/PhysicsMaterial.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Physics/Components/RigidBody.h"
#include "MainSystem/Physics/PhysicsSystem.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsShape::~PhysicsShape()
{
	if (m_pxShape)
		m_pxShape->release();

	m_pxShape = nullptr;
}

void PhysicsShape::SetTransform(const Transform& transform)
{
	PxTransform pxTransform;
	pxTransform.p = reinterpret_cast<PxVec3&>(transform.GetPosition());
	pxTransform.q = PxQuat(transform.GetRotation().x, transform.GetRotation().y, transform.GetRotation().z, transform.GetRotation().w);
	m_pxShape->setLocalPose(pxTransform);
}

SharedPtr<PhysicsMaterial> PhysicsShape::GetDeserializedMaterial(Serializer* serializer, const json& j)
{
	SharedPtr<PhysicsMaterial> material;
	serializer->Deserialize(j["Meterial"], material);
	return material;
}

void PhysicsShape::CloneFrom(Serializer* serializer, Serializable* another)
{
	auto src = (PhysicsShape*)another;
	m_pxShape->setLocalPose(src->m_pxShape->getLocalPose());
	m_meterial = serializer->Clone(src->m_meterial);
}

void PhysicsShape::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void PhysicsShape::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void PhysicsShape::SerializeToJson(Serializer* serializer, json& j) const
{
	j["Transform"] = PhysXUtils::ToTransform(m_pxShape->getLocalPose());
	j["Meterial"] = serializer->Serialize(m_meterial);
}

void PhysicsShape::DeserializeFromJson(Serializer* serializer, const json& j)
{
	serializer->Deserialize(j["Meterial"], m_meterial);
	m_pxShape->setLocalPose(PhysXUtils::ToPxTransform(j["Transform"]));
}

Handle<ClassMetadata> PhysicsShape::GetMetadata(size_t sign)
{
	auto metadata = mheap::New<ClassMetadata>("PhysicsShape", shared_from_this());

	metadata->AddProperty(Accessor(
		"LocalTransform",
		this,
		[](const Variant& input, UnknownAddress& var, Serializable* instance) -> void
		{
			auto* self = (PhysicsShape*)instance;
			return self->SetLocalTransform(input.As<Transform>());
		},
		[](UnknownAddress& var, Serializable* instance) -> Variant
		{
			auto* self = (PhysicsShape*)instance;
			return Variant::Of(self->GetLocalTransform());
		},
		this
	));

	return metadata;
}

void PhysicsShape::SetLocalTransform(const Transform& transform)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(m_attachedRigidBody, PhysicsSystem, AsyncTaskRunnerST, transform,
		{
			self->m_pxShape->setLocalPose(PhysXUtils::ToPxTransform(transform));
		}
	);
}

Transform PhysicsShape::GetLocalTransform() const
{
	return PhysXUtils::ToTransform(m_pxShape->getLocalPose());
}

NAMESPACE_END