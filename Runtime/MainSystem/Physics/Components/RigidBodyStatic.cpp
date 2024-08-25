#include "RigidBodyStatic.h"

#include "Scene/GameObject.h"

#include "PxPhysicsAPI.h"

#include "PhysX/PhysX.h"

#include "../Shapes/PhysicsShape.h"

using namespace physx;

NAMESPACE_BEGIN

RigidBodyStatic::RigidBodyStatic()
{
	auto physics = PhysX::Get()->GetPxPhysics();

	auto body = physics->createRigidStatic(PxTransform(PxIdentity));
	m_pxActor = body;
	m_pxActor->userData = this;
}

RigidBodyStatic::RigidBodyStatic(const SharedPtr<PhysicsShape>& shape) : RigidBodyStatic()
{
	AddShape(shape);
}

RigidBodyStatic::~RigidBodyStatic()
{

}

void RigidBodyStatic::OnPhysicsTransformChanged()
{
	// can not be moved
	assert(0);
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

void RigidBodyStatic::CloneFrom(Serializer* serializer, Serializable* another)
{
}

void RigidBodyStatic::SerializeToBinary(Serializer* serializer, ByteStream& stream) const
{
}

void RigidBodyStatic::DeserializeFromBinary(Serializer* serializer, const ByteStream& stream)
{
}

void RigidBodyStatic::SerializeToJson(Serializer* serializer, json& j) const
{
	RigidBody::SerializeToJson(serializer, j);
}

void RigidBodyStatic::DeserializeFromJson(Serializer* serializer, const json& j)
{
	//assert(m_pxActor == nullptr);

	RigidBody::DeserializeFromJson(serializer, j);

	/*PxRigidStatic* body;
	if (m_pxActor == nullptr)
	{
		auto physics = PhysX::Get()->GetPxPhysics();
		body = physics->createRigidStatic(PxTransform(PxIdentity));
		m_pxActor = body;
		m_pxActor->userData = this;
	}
	else
	{
		body = m_pxActor->is<PxRigidStatic>();
	}

	for (auto& shape : m_shapes)
	{
		body->attachShape(*shape->m_pxShape);
	}

	RigidBody::DeserializeFromJson(serializer, j);*/
}

Handle<ClassMetadata> RigidBodyStatic::GetMetadata(size_t sign)
{
	auto metadata = RigidBody::GetMetadata(sign + 1);
	metadata->SetName(GetClassName());

	return metadata;
}

void RigidBodyStatic::OnPropertyChanged(const UnknownAddress& var, const Variant& newValue)
{
}

NAMESPACE_END