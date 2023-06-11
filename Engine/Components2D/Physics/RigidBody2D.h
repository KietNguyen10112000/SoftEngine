#pragma once

#include "Body2D.h"

NAMESPACE_BEGIN

class RigidBody2D : public Body2D
{
public:
	enum TYPE
	{
		DYNAMIC,
		KINEMATIC
	};

	struct BODY_DESC
	{
		size_t	m = 1;
		Vec2	v = {};
	};

	struct KINEMATIC_DESC : public BODY_DESC
	{
		int priority = 0;
	};

	struct DYNAMIC_DESC : public BODY_DESC
	{
		int padd = 0;
	};

	struct DESC
	{
		TYPE type;
		union
		{
			KINEMATIC_DESC kinematic = {};
			DYNAMIC_DESC dynamic;
		};
	};

	DESC m_desc = {};

public:
	RigidBody2D(const TYPE type, const SharedPtr<Collider2D>& collider) : Body2D(collider)
	{
		m_desc.type = type;
	}

	RigidBody2D(DESC desc, const SharedPtr<Collider2D>& collider) : Body2D(collider), m_desc(desc)
	{
	}

	inline void ReactKinematic()
	{
		auto obj = GetObject();

		auto& cachedTransform = obj->GetCachedTransform();
		auto& prevPosition = cachedTransform.GetTranslation();
		if (prevPosition == obj->Position())
		{
			// object doesn't move, nothing happend
			return;
		}

		
		auto& collisionPairs = CollisionPairs();
		//auto transMat = obj->Transform().ToTransformMatrix();
		for (auto& pair : collisionPairs)
		{
			auto another = pair->GetAnotherOf(this);
			if (another->Type() != Physics2D::STATIC)
			{
				// kinematic object only reacts to static object
				// if not static object, doesn't react
				continue;
			}

			Collider()->AdjustSelf(
				obj->Transform(),
				obj->Transform().ToTransformMatrix(),
				another->Collider().get(), 
				another->GetObject()->GlobalTransformMatrix()
			);
		}
	}

	inline void ReactDynamic()
	{
		auto obj = GetObject();

		auto& cachedTransform = obj->GetCachedTransform();
		auto& prevPosition = cachedTransform.GetTranslation();
		if (prevPosition == obj->Position())
		{
			// object doesn't move, nothing happend
			return;
		}

		auto& collisionPairs = CollisionPairs();
		for (auto& pair : collisionPairs)
		{
			if (!pair->result.HasCollision()) continue;

			auto another = pair->GetAnotherOf(this);

			Collider()->AdjustSelf(
				obj->Transform(),
				obj->Transform().ToTransformMatrix(),
				another->Collider().get(),
				another->GetObject()->GlobalTransformMatrix()
			);
		}
	}

	virtual void ReactCollisionPairs() override
	{
		switch (m_desc.type)
		{
		case DYNAMIC:
			ReactDynamic();
			break;
		case KINEMATIC:
			ReactKinematic();
			break;
		}
	}

};

NAMESPACE_END