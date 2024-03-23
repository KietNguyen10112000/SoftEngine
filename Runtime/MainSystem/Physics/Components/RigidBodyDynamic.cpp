#include "RigidBodyDynamic.h"

#include "Scene/GameObject.h"

#include "PxPhysicsAPI.h"

#include "PhysX/PhysX.h"

#include "../Shapes/PhysicsShape.h"

using namespace physx;

NAMESPACE_BEGIN

RigidBodyDynamic::RigidBodyDynamic(const SharedPtr<PhysicsShape>& shape)
{
	auto physics = PhysX::Get()->GetPxPhysics();

	auto body = physics->createRigidDynamic(PxTransform(PxIdentity));
	body->attachShape(*shape->m_pxShape);

	m_pxActor = body;
	m_pxActor->userData = this;
	//body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

	m_shapes.push_back(shape);
}

RigidBodyDynamic::~RigidBodyDynamic()
{

}

void RigidBodyDynamic::OnTransformChanged()
{
	auto gameObject = GetGameObject();

	auto& globalTransform = gameObject->ReadGlobalTransformMat();

	//auto pxRigidBody = m_pxActor->is<PxRigidBody>();

	//assert(pxRigidBody && "something wrong here!");

	PxRigidDynamic* pxRigidBody = m_pxActor->is<PxRigidDynamic>();

	//auto pxTransform = pxRigidBody->getGlobalPose();

	if (::memcmp(&m_lastGlobalTransform, &globalTransform, sizeof(Mat4)) != 0)
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
			pxRigidBody->setKinematicTarget(pxTransform);
		}
		else
		{
			pxRigidBody->setGlobalPose(pxTransform);
		}

		m_lastGlobalTransform = globalTransform;
	}
}

void RigidBodyDynamic::OnPhysicsTransformChanged()
{
	auto obj = GetGameObject();
	obj->ContributeTransform(this, RigidBodyDynamic::TransformContributor);
	obj->GetScene()->OnObjectTransformChanged(obj);
}

void RigidBodyDynamic::TransformContributor(GameObject* object, Transform& local, Mat4& global, void* self)
{
	auto rigidBody = (RigidBodyDynamic*)self;
	auto gameObject = rigidBody->GetGameObject();
	auto scene = gameObject->GetScene();
	auto pxRigidBody = (PxRigidActor*)rigidBody->m_pxActor;

	auto& lastGlobalTransform = rigidBody->m_lastGlobalTransform;
	if (gameObject->m_lastWriteLocalTransformIterationCount == scene->GetIterationCount())
	{
		return;
	}

	auto pxTransform = pxRigidBody->getGlobalPose();

	PxMat44 shapePose(pxTransform);
	Mat4& myMat = reinterpret_cast<Mat4&>(shapePose);

	global = Mat4::Scaling(local.Scale()) * myMat;
	lastGlobalTransform = global;
	rigidBody->GetGameObject()->m_isNeedRecalculateLocalTransform = true;
}

void RigidBodyDynamic::Serialize(Serializer* serializer)
{
}

void RigidBodyDynamic::Deserialize(Serializer* serializer)
{
}

void RigidBodyDynamic::CleanUp()
{
}

Handle<ClassMetadata> RigidBodyDynamic::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void RigidBodyDynamic::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

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

void RigidBodyDynamic::SetMass(float mass)
{
	auto pxRigidBody = (PxRigidDynamic*)m_pxActor;
	pxRigidBody->setMass(mass);
	pxRigidBody->setMassSpaceInertiaTensor(PxVec3(0.f));
}

void RigidBodyDynamic::SetKinematic(bool enable)
{
	auto pxRigidBody = (PxRigidDynamic*)m_pxActor;
	pxRigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, enable);

	m_isKinematic = (byte)enable;
}

NAMESPACE_END