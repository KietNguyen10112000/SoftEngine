#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/SmartPointers.h"

#include "Components2D/SubSystemComponent2D.h"
#include "Components2D/SubSystemComponentId2D.h"

//#include "Objects2D/Scene2D/Scene2D.h"
#include "Objects2D/Physics/Colliders/Collider2D.h"

//#include "SubSystems/Physics/PhysicsSystem.h"

NAMESPACE_BEGIN

class Body2D;
class Collision2DPair;

class Physics2D : public SubSystemComponent2D
{
public:
	constexpr static size_t COMPONENT_ID = SubSystemComponentId2D::PHYSICS_SUBSYSTEM_COMPONENT_ID;

	/*enum TYPE
	{
		STATIC,
		DYNAMIC,
		KINEMATIC
	};*/

protected:
	friend class PhysicsSystem2D;

	ID m_id = INVALID_ID;

	SharedPtr<Collider2D> m_collider;
	SharedPtr<Body2D> m_body;

	size_t m_lastBoardPhaseIterationCount = 0;
	std::Vector<Collision2DPair*> m_collisionPairs;

public:
	inline Physics2D(
		const SharedPtr<Collider2D>& collider, 
		const SharedPtr<Body2D>& body
	) : m_collider(collider), m_body(body) {};

	inline virtual ~Physics2D() {};

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

	virtual math::AARect GetLocalAABB() override
	{
		return m_collider->GetLocalAABB();
	}

public:
	inline auto& CollisionPairs()
	{
		return m_collisionPairs;
	}

	inline auto& Collider()
	{
		return m_collider;
	}

	inline auto& Body()
	{
		return m_body;
	}

};

NAMESPACE_END