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
}

RigidBodyDynamic::~RigidBodyDynamic()
{

}

void RigidBodyDynamic::TransformContributor(GameObject* object, Transform& local, Mat4& global, void* self)
{
	auto rigidBody = (RigidBodyDynamic*)self;
	auto pxRigidBody = (PxRigidActor*)rigidBody->m_pxActor;
	auto pxTransform = pxRigidBody->getGlobalPose();

	PxMat44 shapePose(pxTransform);
	Mat4& myMat = reinterpret_cast<Mat4&>(shapePose);

	global = myMat;

	rigidBody->m_lastGlobalTransform = myMat;
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