#include "PhysX.h"

#include "PhysXAllocator.h"
#include "PhysXCpuDispatcher.h"

NAMESPACE_BEGIN

using namespace physx;

void PhysX::DeletionListener::onRelease(const physx::PxBase* observed, void* userData, physx::PxDeletionEventFlag::Enum deletionEvent)
{

}

PhysX::PhysX()
{
	static PhysXAllocator allocator = {};
	static PhysXCpuDispatcher cpuDispatcher = {};

	m_allocator = &allocator;
	m_dispatcher = &cpuDispatcher;

	m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, m_errorCallback);
	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, PxTolerancesScale());

	//m_physics->registerDeletionListener(m_deletionListener, PxDeletionEventFlag::eMEMORY_RELEASE);
}

PhysX::~PhysX()
{
	PX_RELEASE(m_physics);
	PX_RELEASE(m_foundation);

	//assert(m_desrializedBuffersMap.size() == 0);
}

NAMESPACE_END