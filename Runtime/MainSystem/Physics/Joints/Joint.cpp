#include "Joint.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "../Components/RigidBody.h"
#include "../Components/RigidBodyDynamic.h"

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
		m_pxJoint->userData = nullptr;
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

	m_pxJoint->userData = this;
}

void Joint::RemoveJointFromBodies()
{
	// break the joint
	if (m_pxJoint == nullptr)
	{
		return;
	}

	{
		if (m_body0->GetPhysicsType() == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
		{
			((RigidBodyDynamic*)m_body0.Get())->InternalWake();
		}

		if (m_body1->GetPhysicsType() == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
		{
			((RigidBodyDynamic*)m_body1.Get())->InternalWake();
		}
	}

	{
		m_body0->m_joints.Remove(m_body0->m_joints.begin() + m_idx0);
		m_body1->m_joints.Remove(m_body1->m_joints.begin() + m_idx1);

		for (size_t i = 0; i < m_body0->m_joints.size(); i++)
		{
			auto& j = m_body0->m_joints[i];
			auto& idx = j->m_body0.Get() == m_body0 ? j->m_idx0 : j->m_idx1;
			idx = i;
		}

		for (size_t i = 0; i < m_body1->m_joints.size(); i++)
		{
			auto& j = m_body1->m_joints[i];
			auto& idx = j->m_body1.Get() == m_body1 ? j->m_idx1 : j->m_idx0;
			idx = i;
		}
	}

	m_body0 = nullptr;
	m_body1 = nullptr;

	m_idx0 = uint32_t(INVALID_ID);
	m_idx1 = uint32_t(INVALID_ID);

	if (m_pxJoint)
	{
		m_pxJoint->userData = nullptr;
		m_pxJoint->release();
		m_pxJoint = nullptr;
	}
}

void Joint::InitJoint(void* pxInitFunc, const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	m_body0 = body0;
	m_body1 = body1;

	assert((body0->GetGameObject() == nullptr && body1->GetGameObject() == nullptr)
		|| (body0->GetGameObject()->GetScene() == body1->GetGameObject()->GetScene() && "2 bodies must be in the same scene!"));
	
	auto l0 = PhysXUtils::ToPxTransform(localFrame0);
	auto l1 = PhysXUtils::ToPxTransform(localFrame1);

	if (body0->GetGameObject() && body0->GetGameObject()->GetScene())
	{
		auto scene = body0->GetGameObject()->GetScene();
		m_pxJoint = (PxJoint*)scene->GenericStorage()->Store<Joint>(this);
	}

	MAIN_SYSTEM_TASK_IMPL_COMMON_3(body0.Get(),
		PhysicsSystem, AsyncTaskRunnerST, l0, l1, pxInitFunc,
		{
			using InitFunc = PxJoint* (*)(PxPhysics&, PxRigidActor*, const PxTransform&, PxRigidActor*, const PxTransform&);
			auto px = PhysX::Get()->GetPxPhysics();
			auto a0 = self->m_body0->m_pxActor;
			auto a1 = self->m_body1->m_pxActor;

			if (self->m_pxJoint)
			{
				self->m_body0->GetGameObject()->GetScene()->GenericStorage()->Remove(ID(self->m_pxJoint));
			}

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

bool Joint::IsBroken() const
{
	return m_pxJoint == nullptr;
}

void Joint::Break()
{
	assert(!IsBroken());

	MAIN_SYSTEM_TASK_IMPL_COMMON_0(m_body0.Get(),
		PhysicsSystem, AsyncTaskRunnerST,
		{
			self->RemoveJointFromBodies();
		}
	);
}

void Joint::SetBreakForce(float force, float torque)
{
	assert(!IsBroken());
	MAIN_SYSTEM_TASK_IMPL_COMMON_2(m_body0.Get(),
		PhysicsSystem, AsyncTaskRunnerST, force, torque,
		{
			self->m_pxJoint->setBreakForce(force, torque);
		}
	);
}

float Joint::GetBreakForce() const
{
	float f, t;
	m_pxJoint->getBreakForce(f, t);
	return f;
}

float Joint::GetBreakTorque() const
{
	float f, t;
	m_pxJoint->getBreakForce(f, t);
	return t;
}

Transform Joint::GetLocalFrame(RigidBody* body) const
{
	if (body == m_body0)
	{
		return PhysXUtils::ToTransform(m_pxJoint->getLocalPose(PxJointActorIndex::eACTOR0));
	}

	if (body == m_body1)
	{
		return PhysXUtils::ToTransform(m_pxJoint->getLocalPose(PxJointActorIndex::eACTOR1));
	}

	return {};
}

void Joint::SetLocalFrame(RigidBody* body, const Transform& transform)
{
	assert(!IsBroken());

	MAIN_SYSTEM_TASK_IMPL_COMMON_2(m_body0.Get(),
		PhysicsSystem, AsyncTaskRunnerST, body, transform,
		{
			if (body == self->m_body0)
			{
				self->m_pxJoint->setLocalPose(PxJointActorIndex::eACTOR0, PhysXUtils::ToPxTransform(transform));
			}

			if (body == self->m_body1)
			{
				self->m_pxJoint->setLocalPose(PxJointActorIndex::eACTOR1, PhysXUtils::ToPxTransform(transform));
			}
		}
	);
}

Transform Joint::GetGlobalTransform() const
{
	auto l0 = PhysXUtils::ToTransform(m_pxJoint->getLocalPose(PxJointActorIndex::eACTOR0));
	auto t0 = Transform::FromTransformMatrix(m_body0->GetGameObject()->GetCommittedGlobalTransform());
	t0.Scale() = { 1,1,1 };
	auto p0 = Transform::FromTransformMatrix(t0.ToTransformMatrix() * l0.ToTransformMatrix());

	auto l1 = PhysXUtils::ToTransform(m_pxJoint->getLocalPose(PxJointActorIndex::eACTOR1));
	auto t1 = Transform::FromTransformMatrix(m_body1->GetGameObject()->GetCommittedGlobalTransform());
	t1.Scale() = { 1,1,1 };
	auto p1 = Transform::FromTransformMatrix(t1.ToTransformMatrix() * l1.ToTransformMatrix());

	if (p0.Equals(p1, 0.01f))
	{
		return p0;
	}

	return Transform::FromTransformMatrix(Mat4::Translation((p0.Position() + p1.Position()) / 2.0f));
}

void Joint::BaseLimit::SerializeToJson(json& j)
{
	j["Restitution"]		= restitution;
	j["BounceThreshold"]	= bounceThreshold;
	j["Stiffness"]			= stiffness;
	j["Damping"]			= damping;
}

void Joint::BaseLimit::DeserializeFromJson(const json& j)
{
	restitution			= j["Restitution"];
	bounceThreshold		= j["BounceThreshold"];
	stiffness			= j["Stiffness"];
	damping				= j["Damping"];
}

NAMESPACE_END