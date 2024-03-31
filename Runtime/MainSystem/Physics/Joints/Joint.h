#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"

#include "Math/Math.h"

namespace physx
{
	class PxJoint;
	class PxActor;
}

NAMESPACE_BEGIN

class PhysicsComponent;
class RigidBody;

class Joint
{
protected:
	physx::PxJoint* m_pxJoint = nullptr;
	Handle<PhysicsComponent> m_actor0 = nullptr;
	Handle<PhysicsComponent> m_actor1 = nullptr;

	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		tracer->Trace(m_actor0);
		tracer->Trace(m_actor1);
	}

protected:
	void InitJoint(physx::PxJoint* pxJoint, const Handle<PhysicsComponent>& actor0, const Handle<PhysicsComponent>& actor1);

public:
	virtual ~Joint();

};

NAMESPACE_END