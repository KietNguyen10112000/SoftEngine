#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"

#include "Math/Math.h"

#include "Common/Base/Serializable.h"

namespace physx
{
	class PxJoint;
	class PxActor;
}

NAMESPACE_BEGIN

class PhysicsComponent;
class RigidBody;

class Joint : public Serializable
{
protected:
	physx::PxJoint* m_pxJoint = nullptr;
	Handle<RigidBody> m_body0 = nullptr;
	Handle<RigidBody> m_body1 = nullptr;
	uint32_t m_idx0 = uint32_t(INVALID_ID);
	uint32_t m_idx1 = uint32_t(INVALID_ID);

	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_body0);
		tracer->Trace(m_body1);
	}

protected:
	void InitJoint(physx::PxJoint* pxJoint, const Handle<RigidBody>& body0, const Handle<RigidBody>& body1);

public:
	virtual ~Joint();

};

NAMESPACE_END