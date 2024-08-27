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
	if (m_body0)
	{
		m_idx0 = m_body0->m_joints.size();
		m_body0->m_joints.Push(this);
	}

	if (m_body1)
	{
		m_idx1 = m_body1->m_joints.size();
		m_body1->m_joints.Push(this);
	}

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
		if (m_body0 && m_body0->GetPhysicsType() == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
		{
			((RigidBodyDynamic*)m_body0.Get())->InternalWake();
		}

		if (m_body1 && m_body1->GetPhysicsType() == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
		{
			((RigidBodyDynamic*)m_body1.Get())->InternalWake();
		}
	}

	{
		if (m_body0)
		{
			m_body0->m_joints.Remove(m_body0->m_joints.begin() + m_idx0);

			for (size_t i = 0; i < m_body0->m_joints.size(); i++)
			{
				auto& j = m_body0->m_joints[i];
				auto& idx = j->m_body0.Get() == m_body0 ? j->m_idx0 : j->m_idx1;
				idx = i;
			}
		}

		if (m_body1)
		{
			m_body1->m_joints.Remove(m_body1->m_joints.begin() + m_idx1);

			for (size_t i = 0; i < m_body1->m_joints.size(); i++)
			{
				auto& j = m_body1->m_joints[i];
				auto& idx = j->m_body1.Get() == m_body1 ? j->m_idx1 : j->m_idx0;
				idx = i;
			}
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

void Joint::ReconstraintBodyForwardToXAxisOfJointGlobalTransform(int bodyIndex)
{
	auto jointGlobalTransform = GetGlobalTransform();
	auto body = bodyIndex == 0 ? GetBody0()->m_pxActor->is<PxRigidBody>() : GetBody1()->m_pxActor->is<PxRigidBody>();
	auto resetJointConstraintOriTransform = body->getGlobalPose();
	auto resetJointConstraintOriJointTransform = jointGlobalTransform.ToTransformMatrix();

	// forward to X
	auto& jointForward = jointGlobalTransform.ToTransformMatrix().Right();
	auto bodyTransform = PhysXUtils::ToTransform(body->getGlobalPose());

	auto& p = bodyTransform.Position();
	auto& o = jointGlobalTransform.Position();
	auto t = o + (p - o).Length() * jointForward.Normal();

	body->setGlobalPose(
		PhysXUtils::ToPxTransform(Transform::FromTransformMatrix(
			Mat4::Rotation(jointGlobalTransform.Rotation()) * Mat4::Translation(t)
		))
	);

	auto a0GlobalTransform = PhysXUtils::ToTransform(GetBody0()->m_pxActor->is<PxRigidBody>()->getGlobalPose());
	auto a1GlobalTransform = PhysXUtils::ToTransform(GetBody1()->m_pxActor->is<PxRigidBody>()->getGlobalPose());

	auto localframe0 = Transform::FromTransformMatrix(resetJointConstraintOriJointTransform * a0GlobalTransform.ToTransformMatrix().GetInverse());
	auto localframe1 = Transform::FromTransformMatrix(resetJointConstraintOriJointTransform * a1GlobalTransform.ToTransformMatrix().GetInverse());

	m_pxJoint->setLocalPose(PxJointActorIndex::eACTOR0, PhysXUtils::ToPxTransform(localframe0));
	m_pxJoint->setLocalPose(PxJointActorIndex::eACTOR1, PhysXUtils::ToPxTransform(localframe1));

	body->setGlobalPose(resetJointConstraintOriTransform);
}

void Joint::InitJoint(void* pxInitFunc, const Handle<RigidBody>& body0, const Transform& localFrame0, const Handle<RigidBody>& body1, const Transform& localFrame1)
{
	assert(body0.Get() != nullptr || body1.Get() != nullptr);

	m_body0 = body0;
	m_body1 = body1;

	if (m_body0)
	{
		m_component = m_body0.Get();
	}

	if (!m_component)
	{
		m_component = m_body1.Get();
	}

	assert(
		body0.Get() == nullptr
		|| body1.Get() == nullptr
		|| (body0.Get() && body0->GetGameObject() == nullptr && body1.Get() && body1->GetGameObject() == nullptr)
		|| (body0->GetGameObject()->GetScene() == body1->GetGameObject()->GetScene() && "2 bodies must be in the same scene!")
	);
	
	auto l0 = PhysXUtils::ToPxTransform(localFrame0);
	auto l1 = PhysXUtils::ToPxTransform(localFrame1);

	if (GetComponent()->GetGameObject() && GetComponent()->GetGameObject()->GetScene())
	{
		auto scene = GetComponent()->GetGameObject()->GetScene();
		m_pxJoint = (PxJoint*)scene->GenericStorage()->Store<Joint>(this);
	}

	MAIN_SYSTEM_TASK_IMPL_COMMON_3(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, l0, l1, pxInitFunc,
		{
			using InitFunc = PxJoint* (*)(PxPhysics&, PxRigidActor*, const PxTransform&, PxRigidActor*, const PxTransform&);
			auto px = PhysX::Get()->GetPxPhysics();
			auto a0 = self->m_body0.Get() ? self->m_body0->m_pxActor : nullptr;
			auto a1 = self->m_body1.Get() ? self->m_body1->m_pxActor : nullptr;

			if (self->m_pxJoint)
			{
				self->GetComponent()->GetGameObject()->GetScene()->GenericStorage()->Remove(ID(self->m_pxJoint));
			}

			if (a0)
			{
				self->m_initBody0Transform = PhysXUtils::ToTransform(a0->is<PxRigidActor>()->getGlobalPose());
			}

			if (a1)
			{
				self->m_initBody1Transform = PhysXUtils::ToTransform(a1->is<PxRigidActor>()->getGlobalPose());
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

	if (j.contains("InitBody0Transform"))
	{
		Transform initTransform0 = j["InitBody0Transform"];
		Transform initTransform1 = j["InitBody1Transform"];

		m_initBody0Transform = initTransform0;
		m_initBody1Transform = initTransform1;

		auto a0 = m_body0.Get() ? m_body0->m_pxActor->is<PxRigidActor>() : nullptr;
		auto a1 = m_body1.Get() ? m_body1->m_pxActor->is<PxRigidActor>() : nullptr;

		PxTransform temp0;
		if (a0)
		{
			temp0 = a0->getGlobalPose();
			a0->setGlobalPose(PhysXUtils::ToPxTransform(initTransform0));
		}

		PxTransform temp1;
		if (a1)
		{
			temp1 = a1->getGlobalPose(); 
			a1->setGlobalPose(PhysXUtils::ToPxTransform(initTransform1));
		}

		m_pxJoint->setLocalPose(PxJointActorIndex::eACTOR0, PhysXUtils::ToPxTransform(l0));
		m_pxJoint->setLocalPose(PxJointActorIndex::eACTOR1, PhysXUtils::ToPxTransform(l1));

		if (a0)
		{
			a0->setGlobalPose(temp0);
		}

		if (a1)
		{
			a1->setGlobalPose(temp1);
		}
	}
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

	j["InitBody0Transform"] = m_initBody0Transform;
	j["InitBody1Transform"] = m_initBody1Transform;
}

void Joint::DeserializeFromJson(Serializer* serializer, const json& j)
{
	assert(0 && "Call void Joint::InitJoint(void* pxInitFunc, Serializer* serializer, const json& j) instead!");
}

void Joint::WakeUpBodies()
{
	if (m_body0)
	{
		auto type = m_body0->GetPhysicsType();
		if (type == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
		{
			auto dynamic = (RigidBodyDynamic*)m_body0.Get();
			dynamic->InternalWake();
		}
	}

	if (m_body1)
	{
		auto type = m_body1->GetPhysicsType();
		if (type == PHYSICS_TYPE_RIGID_BODY_DYNAMIC)
		{
			auto dynamic = (RigidBodyDynamic*)m_body1.Get();
			dynamic->InternalWake();
		}
	}
}

bool Joint::IsBroken() const
{
	return m_pxJoint == nullptr || m_isBroken == true;
}

void Joint::Break()
{
	assert(!IsBroken());

	m_isBroken = true;

	MAIN_SYSTEM_TASK_IMPL_COMMON_0(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST,
		{
			self->RemoveJointFromBodies();
		}
	);
}

void Joint::SetBreakForce(float force, float torque)
{
	assert(!IsBroken());
	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
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

	MAIN_SYSTEM_TASK_IMPL_COMMON_2(GetComponent(),
		PhysicsSystem, AsyncTaskRunnerST, body, transform,
		{
			auto a0 = self->m_body0.Get() ? self->m_body0->m_pxActor : nullptr;
			auto a1 = self->m_body1.Get() ? self->m_body1->m_pxActor : nullptr;

			if (a0)
			{
				self->m_initBody0Transform = PhysXUtils::ToTransform(a0->is<PxRigidActor>()->getGlobalPose());
			}

			if (a1)
			{
				self->m_initBody1Transform = PhysXUtils::ToTransform(a1->is<PxRigidActor>()->getGlobalPose());
			}

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
	auto t0 = m_body0.Get() ? Transform::FromTransformMatrix(m_body0->GetGameObject()->GetCommittedGlobalTransform()) : Transform();
	t0.Scale() = { 1,1,1 };
	auto p0 = Transform::FromTransformMatrix(l0.ToTransformMatrix() * t0.ToTransformMatrix());

	auto l1 = PhysXUtils::ToTransform(m_pxJoint->getLocalPose(PxJointActorIndex::eACTOR1));
	auto t1 = m_body1.Get() ? Transform::FromTransformMatrix(m_body1->GetGameObject()->GetCommittedGlobalTransform()) : Transform();
	t1.Scale() = { 1,1,1 };
	auto p1 = Transform::FromTransformMatrix(l1.ToTransformMatrix() * t1.ToTransformMatrix());

	if (p0.Equals(p1, 0.01f))
	{
		return p0;
	}

	return Transform::FromTransformMatrix(Mat4::Translation((p0.Position() + p1.Position()) / 2.0f));
}

void Joint::BaseLimit::SerializeToJson(json& j) const
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