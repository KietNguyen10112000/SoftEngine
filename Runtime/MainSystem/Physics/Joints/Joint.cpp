#include "Joint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"

#include "Scene/GameObject.h"
#include "Scene/Scene.h"

#include "MainSystem/MainSystemTaskPacking.h"
#include "MainSystem/Physics/PhysicsSystem.h"

using namespace physx;

NAMESPACE_BEGIN

Joint::~Joint()
{
	if (m_pxJoint)
	{
		m_pxJoint->release();
		m_pxJoint = nullptr;
	}
}

void Joint::CommitJointToBodies()
{
	m_idx0 = m_body0->m_joints.size();
	m_body0->m_joints.Push(this);

	m_idx1 = m_body1->m_joints.size();
	m_body1->m_joints.Push(this);
}

void Joint::InitJoint(void* pxInitFunc, const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	m_body0 = body0;
	m_body1 = body1;

	assert((body0->GetGameObject() == nullptr && body1->GetGameObject() == nullptr)
		|| (body0->GetGameObject()->GetScene() == body1->GetGameObject()->GetScene() && "2 bodies must be in the same scene!"));
	
	auto l0 = PhysXUtils::ToPxTransform(localFrame0);
	auto l1 = PhysXUtils::ToPxTransform(localFrame1);

	MAIN_SYSTEM_TASK_IMPL_COMMON_3(body0.Get(),
		PhysicsSystem, AsyncTaskRunnerST, l0, l1, pxInitFunc,
		{
			using InitFunc = PxJoint* (*)(PxPhysics&, PxRigidActor*, const PxTransform&, PxRigidActor*, const PxTransform&);
			auto px = PhysX::Get()->GetPxPhysics();
			auto a0 = self->m_body0->m_pxActor;
			auto a1 = self->m_body1->m_pxActor;
			self->m_pxJoint = ((InitFunc)pxInitFunc)(*px,
				a0->is<PxRigidActor>(),
				l0,
				a1->is<PxRigidActor>(),
				l1
			);
			self->CommitJointToBodies();
		}
	);
}

void Joint::InitJoint(void* pxInitFunc, Serializer* serializer, const json& j)
{
	serializer->Deserialize(j["Body0"], m_body0);
	serializer->Deserialize(j["Body1"], m_body1);

	Transform l0 = j["Localframe0"];
	Transform l1 = j["Localframe1"];

	InitJoint(pxInitFunc, m_body0, l0, m_body1, l1);
}

void Joint::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void Joint::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void Joint::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void Joint::SerializeToJson(Serializer* serializer, json& j) const
{
	j["Body0"] = serializer->Serialize(m_body0);
	j["Body1"] = serializer->Serialize(m_body1);

	j["Localframe0"] = PhysXUtils::ToTransform(m_pxJoint->getLocalPose(PxJointActorIndex::eACTOR0));
	j["Localframe1"] = PhysXUtils::ToTransform(m_pxJoint->getLocalPose(PxJointActorIndex::eACTOR1));
}

void Joint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	assert(0 && "Call void Joint::InitJoint(void* pxInitFunc, Serializer* serializer, const json& j) instead!");
}

NAMESPACE_END