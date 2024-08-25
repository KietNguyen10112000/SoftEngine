#include "PhysicsShape.h"

#include "PxPhysicsAPI.h"

#include "PhysX/Utils.h"

#include "../Materials/PhysicsMaterial.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Physics/Components/RigidBody.h"
#include "MainSystem/Physics/PhysicsSystem.h"
#include "MainSystem/Physics/Components/RigidBodyDynamic.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "../FILTER_FLAG.h"

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

void PhysicsShape::RecalculateMass()
{
	auto actor = m_pxShape->getActor();
	if (actor)
	{
		auto dynamic = actor->is<PxRigidDynamic>();
		if (dynamic)
		{
			auto comp = ((RigidBodyDynamic*)dynamic->userData);
			auto pxRigidBody = (PxRigidDynamic*)comp->m_pxActor;
			PxRigidBodyExt::updateMassAndInertia(*pxRigidBody, comp->m_density);
			comp->InternalWake();
		}
	}
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

	auto filterData = m_pxShape->getSimulationFilterData();
	j["FilterFlags"] = filterData.word0 & (~PHYSICS_FILTER_FLAG::CALLBACK);
	j["CollisionMask"] = filterData.word1;
}

void PhysicsShape::DeserializeFromJson(Serializer* serializer, const json& j)
{
	serializer->Deserialize(j["Meterial"], m_meterial);
	m_pxShape->setLocalPose(PhysXUtils::ToPxTransform(j["Transform"]));

	if (j.contains("FilterFlags"))
	{
		PxFilterData filterData = {};
		filterData.word0 = j["FilterFlags"];
		filterData.word1 = j["CollisionMask"];
		m_pxShape->setSimulationFilterData(filterData);
	}
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

			auto actor = self->m_pxShape->getActor();
			if (actor)
			{
				auto dynamic = actor->is<PxRigidDynamic>();
				if (dynamic)
				{
					auto comp = ((RigidBodyDynamic*)dynamic->userData);
					PxRigidBodyExt::updateMassAndInertia(*dynamic, comp->GetDensity());
					comp->InternalWake();
				}
			}
		}
	);
}

Transform PhysicsShape::GetLocalTransform() const
{
	return PhysXUtils::ToTransform(m_pxShape->getLocalPose());
}

void PhysicsShape::SetCollisionMask(uint32_t mask)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(
		m_attachedRigidBody, PhysicsSystem, AsyncTaskRunnerST, mask,
		{
			PxFilterData data = self->m_pxShape->getSimulationFilterData();
			data.word1 = mask;
			self->m_pxShape->setSimulationFilterData(data);
		}
	);
}

uint32_t PhysicsShape::GetCollisionMask() const
{
	return m_pxShape->getSimulationFilterData().word1;
}

bool PhysicsShape::IsEnableFamilyNoCollide()
{
	return m_pxShape->getSimulationFilterData().word0 & PHYSICS_FILTER_FLAG::FAMILY_NO_COLLIDE;
}

void PhysicsShape::SetFamilyNoCollide(bool enable)
{
	MAIN_SYSTEM_TASK_IMPL_COMMON_1(
		m_attachedRigidBody, PhysicsSystem, AsyncTaskRunnerST, enable,
		{
			PxFilterData data = self->m_pxShape->getSimulationFilterData();
			if (enable)
			{
				data.word0 |= PHYSICS_FILTER_FLAG::FAMILY_NO_COLLIDE;
			}
			else
			{
				data.word0 &= ~PHYSICS_FILTER_FLAG::FAMILY_NO_COLLIDE;
			}
			self->m_pxShape->setSimulationFilterData(data);
		}
	);
}

NAMESPACE_END