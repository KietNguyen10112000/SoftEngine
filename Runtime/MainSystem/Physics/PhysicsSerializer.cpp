#include "PhysicsSerializer.h"

#include "PxPhysicsAPI.h"

using namespace physx;

NAMESPACE_BEGIN

PhysicsSerializer::PhysicsSerializer()
{
}

PhysicsSerializer::~PhysicsSerializer()
{
	if (m_pxSerializeCollection)
	{
		m_pxSerializeCollection->release();
		m_pxSerializeCollection = nullptr;
	}

	if (m_pxDeserializeCollection)
	{
		m_pxDeserializeCollection->release();
		m_pxDeserializeCollection = nullptr;
	}
}

void PhysicsSerializer::InitializeSerializeCollection()
{
	m_pxSerializeCollection = PxCreateCollection();
}

void PhysicsSerializer::InitializeDeserializeCollectionFromSerializeCollection()
{

}

void PhysicsSerializer::ReadFromFile(const char* path)
{
}

ID PhysicsSerializer::Serialize(physx::PxBase* object)
{
	assert(m_lastStage == STAGE::SERIALIZE);
	m_lastStage = STAGE::SERIALIZE;
	if (m_pxSerializeCollection == nullptr)
	{
		InitializeSerializeCollection();
	}

	m_pxSerializeCollection->add(*object, m_idCounter);

	return m_idCounter++;
}

physx::PxBase* PhysicsSerializer::Deserialize(ID id)
{
	if (m_pxDeserializeCollection == nullptr)
	{
		InitializeDeserializeCollectionFromSerializeCollection();
	}

	assert(m_lastStage == STAGE::SERIALIZE || m_lastStage == STAGE::DESRIALIZE);
	m_lastStage = STAGE::DESRIALIZE;

	return m_pxSerializeCollection->find(id);
}

NAMESPACE_END