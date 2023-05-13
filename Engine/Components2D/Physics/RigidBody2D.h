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

	const TYPE m_BODY_TYPE;

	RigidBody2D(TYPE type, const SharedPtr<Collider2D>& collider) : Body2D(collider), m_BODY_TYPE(type)
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
	}

	virtual void ReactCollisionPairs() override
	{
		switch (m_BODY_TYPE)
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