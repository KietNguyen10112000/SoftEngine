#include "RigidBodyStatic.h"

#include "Scene/GameObject.h"

#include "PxPhysicsAPI.h"

#include "PhysX/PhysX.h"

#include "../Shapes/PhysicsShape.h"

using namespace physx;

NAMESPACE_BEGIN

RigidBodyStatic::RigidBodyStatic(const SharedPtr<PhysicsShape>& shape)
{
	auto physics = PhysX::Get()->GetPxPhysics();

	auto body = physics->createRigidStatic(PxTransform(PxIdentity));
	body->attachShape(*shape->m_pxShape);

	m_pxActor = body;
	m_pxActor->userData = this;

	m_shapes.push_back(shape);
}

RigidBodyStatic::~RigidBodyStatic()
{

}

void RigidBodyStatic::OnPhysicsTransformChanged()
{
	// can not be moved
	assert(0);
}

void RigidBodyStatic::Serialize(Serializer* serializer)
{
}

void RigidBodyStatic::Deserialize(Serializer* serializer)
{
}

void RigidBodyStatic::CleanUp()
{
}

Handle<ClassMetadata> RigidBodyStatic::GetMetadata(size_t sign)
{
	return Handle<ClassMetadata>();
}

void RigidBodyStatic::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

void RigidBodyStatic::OnComponentAdded()
{
}

void RigidBodyStatic::OnComponentRemoved()
{
}

AABox RigidBodyStatic::GetGlobalAABB()
{
	return AABox();
}

NAMESPACE_END