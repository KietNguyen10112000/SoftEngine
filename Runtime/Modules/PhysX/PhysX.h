#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Pattern/Singleton.h"

#include "PxPhysicsAPI.h"

#include <map>

NAMESPACE_BEGIN

class PhysX : public Singleton<PhysX>
{
public:
	struct SharedDeserializedBuffer
	{
		void* mem = nullptr;
		size_t count = 0;
	};

private:
	class DeletionListener : public physx::PxDeletionListener
	{
	public:
		PhysX* m_physX = nullptr;

		inline DeletionListener(PhysX* physX) : m_physX(physX) {};

		virtual void onRelease(const physx::PxBase* observed, void* userData, physx::PxDeletionEventFlag::Enum deletionEvent) override;
	};

	physx::PxAllocatorCallback*	m_allocator;
	physx::PxDefaultErrorCallback	m_errorCallback;
	physx::PxFoundation* m_foundation = NULL;
	physx::PxPhysics* m_physics = NULL;
	physx::PxCpuDispatcher* m_dispatcher = NULL;

	//std::map<physx::PxBase*, SharedDeserializedBuffer*> m_desrializedBuffersMap;
	//DeletionListener m_deletionListener = this;

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

	/*inline auto* GetDesrializedBuffersMap()
	{
		return &m_desrializedBuffersMap;
	}*/
};

NAMESPACE_END