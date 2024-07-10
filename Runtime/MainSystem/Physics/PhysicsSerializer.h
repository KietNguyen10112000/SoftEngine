#pragma once

#include "Core/TypeDef.h"
#include <cassert>

namespace physx
{
	class PxCollection;
	class PxBase;
}

NAMESPACE_BEGIN

class PhysicsSerializer
{
private:
	enum STAGE
	{
		SERIALIZE,
		DESRIALIZE
	};

	friend class Serializer;
	physx::PxCollection* m_pxSerializeCollection = nullptr;
	physx::PxCollection* m_pxDeserializeCollection = nullptr;

	ID m_idCounter = 1;

	STAGE m_lastStage = STAGE::SERIALIZE;

public:
	PhysicsSerializer();
	~PhysicsSerializer();

private:
	void InitializeSerializeCollection();
	void InitializeDeserializeCollectionFromSerializeCollection();

	void ReadFromFile(const char* path);

public:
	ID Serialize(physx::PxBase* object);
	physx::PxBase* Deserialize(ID id);

	/*inline auto GetSerializePxCollection()
	{
		if (m_pxSerializeCollection == nullptr)
		{
			InitializeSerializeCollection();
		}

		assert(m_lastStage == STAGE::SERIALIZE);
		m_lastStage = STAGE::SERIALIZE;
		return m_pxSerializeCollection;
	}

	inline auto GetDeserializePxCollection()
	{
		if (m_pxDeserializeCollection == nullptr)
		{
			InitializeDeserializeCollectionFromSerializeCollection();
		}

		assert(m_lastStage == STAGE::SERIALIZE || m_lastStage == STAGE::DESRIALIZE);
		m_lastStage = STAGE::DESRIALIZE;
		return m_pxDeserializeCollection;
	}*/
};

NAMESPACE_END