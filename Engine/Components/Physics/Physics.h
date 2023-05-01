#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/DeferredBuffer.h"

#include "Components/SubSystemComponent.h"
#include "Components/SubSystemComponentId.h"

#include "Objects/Scene/Scene.h"
#include "Objects/Physics/Colliders/Collider.h"

#include "SubSystems/Physics/PhysicsSystem.h"

NAMESPACE_BEGIN

class Manifold;

class Physics : public SubSystemComponent
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId::PHYSICS_SUBSYSTEM_COMPONENT_ID;

	enum TYPE
	{
		STATIC,
		DYNAMIC,
		KINEMATIC
	};

protected:
	friend class PhysicsSystem;
	const TYPE m_TYPE = STATIC;
	ID m_dynamicId = INVALID_ID;

	// stored aabb of object for dynamic branch
	AABox m_aabb;
	std::atomic<size_t> m_numBoardPhase = { 0 };
	std::atomic<size_t> m_numProcessedBoardPhase = { 0 };
	std::atomic<size_t> m_numAcquiredBoardPhase = { 0 };
	std::atomic<size_t> m_numFilterDuplBoardPhase = { 0 };
	std::atomic<size_t> m_numBeginSetup = { 0 };
	size_t m_numClearManifold = 0;
	size_t m_processedBoardPhaseDispatchId = INVALID_ID;

	Spinlock m_clearManifoldLock;
	bool m_isRefreshed = false;
	bool m_padd[2];

	DeferredBuffer<raw::ConcurrentArrayList<Manifold*>> m_manifolds;

	SharedPtr<Collider> m_collider;

public:
	Vec3 m_debugColor = { 0,1,0 };
	size_t m_debugIteration = 0;

public:
	inline Physics(TYPE type, const SharedPtr<Collider>& collider) : m_TYPE(type), m_collider(collider) {};
	inline virtual ~Physics() {};

public:
	virtual void OnComponentAdded() {};
	virtual void OnComponentRemoved() {};
	virtual void OnComponentAddedToScene() 
	{
		OnObjectRefresh();
	};

	virtual void OnComponentRemovedFromScene() {};
	virtual void SetAsMain() {};
	virtual void SetAsExtra() {};

	virtual void ResolveBranch() {};
	virtual bool IsNewBranch()
	{
		return false;
	}

	virtual void OnObjectRefresh() override
	{
		//m_aabb = m_collider->GetLocalAABB();
		//m_aabb.Transform(GetObject()->GetTransformMat4());
		m_isRefreshed = true;
		m_aabb = GetObject()->GetAABB();
		GetObject()->GetScene()->GetPhysicsSystem()->AddToBeginBoardPhase(GetObject());
	}

	virtual math::AABox GetLocalAABB() override
	{
		return m_collider->GetLocalAABB();
	}

public:
	inline auto Type() const
	{
		return m_TYPE;
	}

	inline auto GetBoardPhaseAABB()
	{
		return m_aabb;
	}



};

NAMESPACE_END