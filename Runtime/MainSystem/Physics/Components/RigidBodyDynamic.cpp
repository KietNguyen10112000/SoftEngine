#include "RigidBodyDynamic.h"

#include "Scene/GameObject.h"

#include "PxPhysicsAPI.h"

#include "PhysX/PhysX.h"
#include "PhysX/Utils.h"

#include "MainSystem/MainSystemTaskPacking.h"

#include "../Shapes/PhysicsShape.h"

#include "../PhysicsSystem.h"

using namespace physx;

NAMESPACE_BEGIN

RigidBodyDynamic::RigidBodyDynamic()
{
	auto physics = PhysX::Get()->GetPxPhysics();

	auto body = physics->createRigidDynamic(PxTransform(PxIdentity));
	m_pxActor = body;
	m_pxActor->userData = this;
}

RigidBodyDynamic::RigidBodyDynamic(const SharedPtr<PhysicsShape>& shape) : RigidBodyDynamic()
{
	AddShape(shape);
}

RigidBodyDynamic::~RigidBodyDynamic()
{

}

void RigidBodyDynamic::OnTransformChanged()
{
	auto gameObject = GetGameObject();

	auto& globalTransform = gameObject->GetCommittedGlobalTransform();

	//auto pxRigidBody = m_pxActor->is<PxRigidBody>();

	//assert(pxRigidBody && "something wrong here!");

	PxRigidDynamic* pxRigidBody = m_pxActor->is<PxRigidDynamic>();

	//auto pxTransform = pxRigidBody->getGlobalPose();

	//if (::memcmp(&m_lastGlobalTransform, &globalTransform, sizeof(Mat4)) != 0)
	{
		Vec3 scale;
		Vec3 pos;
		Quaternion rot;
		globalTransform.Decompose(scale, rot, pos);

		PxTransform pxTransform;
		pxTransform.p = reinterpret_cast<PxVec3&>(pos);
		pxTransform.q.x = rot.x;
		pxTransform.q.y = rot.y;
		pxTransform.q.z = rot.z;
		pxTransform.q.w = rot.w;

		if (m_isKinematic)
		{
			if (m_isKinematic == 1)
			{
				pxRigidBody->setGlobalPose(pxTransform);
				m_isKinematic = 2;
			}

			if (pxRigidBody->getScene())
			{
				pxRigidBody->setKinematicTarget(pxTransform);
			}
		}
		else
		{
			pxRigidBody->setGlobalPose(pxTransform);
		}

		//m_lastGlobalTransform = globalTransform;
	}
}

void RigidBodyDynamic::InternalWake()
{
	if (!m_pxActor)
	{
		return;
	}
	auto dynamic = ((PxRigidDynamic*)m_pxActor);
	if (!m_isKinematic && dynamic->getScene() && dynamic->isSleeping())
	{
		dynamic->wakeUp();
	}
}

void RigidBodyDynamic::Wake()
{
	if (!m_pxActor || !(((PxRigidDynamic*)m_pxActor)->isSleeping()))
	{
		return;
	}
	OnPhysicsTransformChanged();
}

void RigidBodyDynamic::OnPhysicsTransformChanged()
{
	auto obj = GetGameObject();
	//obj->ContributeTransform(this, RigidBodyDynamic::TransformContributor);
	//obj->GetScene()->OnObjectTransformChanged(obj);

	auto pxRigidBody = (PxRigidActor*)m_pxActor;
	auto pxTransform = pxRigidBody->getGlobalPose();

	PxMat44 shapePose(pxTransform);
	Mat4& myMat = reinterpret_cast<Mat4&>(shapePose);

	obj->SetGlobalTransform(Mat4::Scaling(obj->GetCommittedLocalTransform().GetScale()) * myMat, COMPONENT_ID);
}

//void RigidBodyDynamic::TransformContributor(GameObject* object, Transform& local, Mat4& global, void* self)
//{
//	auto rigidBody = (RigidBodyDynamic*)self;
//	auto gameObject = rigidBody->GetGameObject();
//	auto scene = gameObject->GetScene();
//	auto pxRigidBody = (PxRigidActor*)rigidBody->m_pxActor;
//
//	auto& lastGlobalTransform = rigidBody->m_lastGlobalTransform;
//	if (gameObject->m_lastWriteLocalTransformIterationCount == scene->GetIterationCount())
//	{
//		return;
//	}
//
//	auto pxTransform = pxRigidBody->getGlobalPose();
//
//	PxMat44 shapePose(pxTransform);
//	Mat4& myMat = reinterpret_cast<Mat4&>(shapePose);
//
//	global = Mat4::Scaling(local.Scale()) * myMat;
//	lastGlobalTransform = global;
//	rigidBody->GetGameObject()->m_isNeedRecalculateLocalTransform = true;
//}

void RigidBodyDynamic::OnComponentAdded()
{
}

void RigidBodyDynamic::OnComponentRemoved()
{
}

AABox RigidBodyDynamic::GetGlobalAABB()
{
	return AABox();
}

void RigidBodyDynamic::SetDensity(float density)
{
	m_density = density;
	auto pxRigidBody = (PxRigidDynamic*)m_pxActor;
	//pxRigidBody->setMass(mass);
	//pxRigidBody->setMassSpaceInertiaTensor(PxVec3(1.f));
	/*pxRigidBody->setLinearVelocity({ 0,0,0 });
	pxRigidBody->setAngularVelocity({ 0,0,0 });
	pxRigidBody->setForceAndTorque({ 0,0,0 }, { 0,0,0 });*/
	PxRigidBodyExt::updateMassAndInertia(*pxRigidBody, density);
}

float RigidBodyDynamic::GetDensity() const
{
	return m_density;
}

float RigidBodyDynamic::GetMass() const
{
	auto pxRigidBody = (PxRigidDynamic*)m_pxActor;
	return pxRigidBody->getMass();
}

void RigidBodyDynamic::SetKinematic(bool enable, bool wakeUp)
{
	MAIN_SYSTEM_TASK_COMMON_2(
		PhysicsSystem, AsyncTaskRunnerST, enable, wakeUp,
		{
			auto pxRigidBody = (PxRigidDynamic*)self->m_pxActor;
			pxRigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, enable);
			if (wakeUp && !enable && pxRigidBody->getScene() && pxRigidBody->isSleeping())
			{
				pxRigidBody->wakeUp();
			}

			self->m_isKinematic = (byte)enable;
		}
	);
}

bool RigidBodyDynamic::IsKinematic() const
{
	return m_isKinematic != 0;
}

void RigidBodyDynamic::AddForce(const Vec3& f)
{
	MAIN_SYSTEM_TASK_COMMON_1(
		PhysicsSystem, AsyncTaskRunnerST, f, 
		{
			auto pxRigidBody = (PxRigidDynamic*)self->m_pxActor;
			pxRigidBody->addForce(reinterpret_cast<const PxVec3&>(f));
		}
	);
}

void RigidBodyDynamic::AddForceAtPos(const Vec3& f, const Vec3& pos)
{
	MAIN_SYSTEM_TASK_COMMON_2(
		PhysicsSystem, AsyncTaskRunnerST, f, pos,
		{
			auto pxRigidBody = (PxRigidDynamic*)self->m_pxActor;
			PxRigidBodyExt::addForceAtPos(*pxRigidBody, reinterpret_cast<const PxVec3&>(f), reinterpret_cast<const PxVec3&>(pos));
		}
	);
}

void RigidBodyDynamic::AddImpulse(const Vec3& impulse)
{
	auto scene = GetGameObject() ? GetGameObject()->GetScene() : nullptr;
	if (scene)
	{
		AddForce(impulse / scene->Dt());
		return;
	}
	
	AddForce(impulse / 0.016f); // :D
}

void RigidBodyDynamic::AddImpulseAtPos(const Vec3& impulse, const Vec3& pos)
{
	auto scene = GetGameObject() ? GetGameObject()->GetScene() : nullptr;
	if (scene)
	{
		AddForceAtPos(impulse / scene->Dt(), pos);
		return;
	}

	AddForceAtPos(impulse / 0.016f, pos); // :D
}

void RigidBodyDynamic::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void RigidBodyDynamic::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void RigidBodyDynamic::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void RigidBodyDynamic::SerializeToJson(Serializer* serializer, json& j) const
{
	RigidBody::SerializeToJson(serializer, j);

	auto body = m_pxActor->is<PxRigidDynamic>();
	j["IsKinematic"]				= m_isKinematic;
	j["LinearVelocity"]				= PhysXUtils::ToVec3(body->getLinearVelocity());
	j["AngularVelocity"]			= PhysXUtils::ToVec3(body->getAngularVelocity());
	j["MassSpaceInertiaTensor"]		= PhysXUtils::ToVec3(body->getMassSpaceInertiaTensor());
	j["Mass"]						= body->getMass();
	j["CMassLocal"]					= PhysXUtils::ToTransform(body->getCMassLocalPose());
	j["Density"]					= m_density;
	//j["ContactReportThreshold"]		= body->getContactReportThreshold();
	//j["ContactSlopCoefficient"]		= body->getContactSlopCoefficient();
	//j["DominanceGroup"]				= body->getDominanceGroup();
	//j["LinearDamping"]				= body->getLinearDamping();
}

void RigidBodyDynamic::DeserializeFromJson(Serializer* serializer, const json& j)
{
	RigidBody::DeserializeFromJson(serializer, j);

	PxRigidDynamic* body;
	if (m_pxActor == nullptr)
	{
		auto physics = PhysX::Get()->GetPxPhysics();
		body = physics->createRigidDynamic(PxTransform(PxIdentity));
		m_pxActor = body;
		m_pxActor->userData = this;
	}
	else
	{
		body = m_pxActor->is<PxRigidDynamic>();
	}

	for (auto& shape : m_shapes)
	{
		body->attachShape(*shape->m_pxShape);
		shape->m_attachedRigidBody = this;
	}

	{
		body->setLinearVelocity(PhysXUtils::ToPxVec3(j["LinearVelocity"]));
		body->setAngularVelocity(PhysXUtils::ToPxVec3(j["AngularVelocity"]));
		body->setMassSpaceInertiaTensor(PhysXUtils::ToPxVec3(j["MassSpaceInertiaTensor"]));
		body->setMass(j["Mass"]);


		if (j.contains("CMassLocal"))
		{
			body->setCMassLocalPose(PhysXUtils::ToPxTransform(j["CMassLocal"]));
		}

		m_isKinematic = j["IsKinematic"];
		if (m_isKinematic)
		{
			body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
		}

		if (j.contains("Density"))
		{
			m_density = j["Density"];
			//PxRigidBodyExt::updateMassAndInertia(*body, m_density);
		}
	}

	//RigidBody::DeserializeFromJson(serializer, j);
}

Handle<ClassMetadata> RigidBodyDynamic::GetMetadata(size_t sign)
{
	auto metadata = RigidBody::GetMetadata(sign + 1);
	metadata->SetName(GetClassName());

	return metadata;
}

void RigidBodyDynamic::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void RigidBodyDynamic::RunAnimatorMotionMatchingCallback(void(*callback)(AnimatorSkeletalArray*, ID), AnimatorSkeletalArray* animator, ID _param)
{
	MAIN_SYSTEM_TASK_COMMON_3(
		PhysicsSystem, AsyncTaskRunnerST, callback, animator, _param,
		{
			callback(animator, _param);
		}
	);
}

NAMESPACE_END