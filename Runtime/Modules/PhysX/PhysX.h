#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Pattern/Singleton.h"

#include "PxPhysicsAPI.h"

NAMESPACE_BEGIN

class PhysX : public Singleton<PhysX>
{
private:
	physx::PxAllocatorCallback*	m_allocator;
	physx::PxDefaultErrorCallback	m_errorCallback;
	physx::PxFoundation* m_foundation = NULL;
	physx::PxPhysics* m_physics = NULL;
	physx::PxCpuDispatcher* m_dispatcher = NULL;

public:
	PhysX();
	~PhysX();

public:
	inline auto GetPxFoundation()
	{
		return m_foundation;
	}

	inline auto GetPxPhysics()
	{
		return m_physics;
	}

	inline auto GetCpuDispatcher()
	{
		return m_dispatcher;
	}

	inline auto GetAllocator()
	{
		return m_allocator;
	}

};

NAMESPACE_END