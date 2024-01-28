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
}

RigidBodyDynamic::~RigidBodyDynamic()
{

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
	lastGlobalTransform = myMat;
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

NAMESPACE_END