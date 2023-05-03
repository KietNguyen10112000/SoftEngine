#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/DeferredBuffer.h"

#include "Components2D/SubSystemComponent2D.h"
#include "Components2D/SubSystemComponentId2D.h"

#include "Objects2D/Scene2D/Scene2D.h"
//#include "Objects/Physics/Colliders/Collider.h"

//#include "SubSystems/Physics/PhysicsSystem.h"

NAMESPACE_BEGIN

class Manifold;

class Physics2D : public SubSystemComponent2D
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