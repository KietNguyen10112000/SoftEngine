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

	enum TYPE
	{
		STATIC,
		DYNAMIC_BODY,
		SENSOR,
		FIELD
	};


	friend class PhysicsSystem2D;

private:
	ID m_id = INVALID_ID;
	const TYPE m_TYPE;

protected:
	SharedPtr<Collider2D> m_collider;

	size_t m_lastBoardPhaseIterationCount = 0;
	std::Vector<Collision2DPair*> m_collisionPairs;

protected:
	inline Physics2D(TYPE type,
		const SharedPtr<Collider2D>& collider
	) : m_TYPE(type), m_collider(collider) {};

public:
	inline Physics2D(
		const SharedPtr<Collider2D>& collider
	) : m_TYPE(TYPE::STATIC), m_collider(collider) {};

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
	virtual void ReactCollisionPairs() {};
	virtual void ContributeTo(Body2D* body) {};

public:
	inline auto& CollisionPairs()
	{
		return m_collisionPairs;
	}

	inline auto& Collider()
	{
		return m_collider;
	}

	inline auto Type() const
	{
		return m_TYPE;
	}
};

NAMESPACE_END