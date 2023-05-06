#pragma once

#include "Body2D.h"

#include "Components2D/Physics/Physics2D.h"
#include "Objects2D/Physics/Collision/Collision2DPair.h"

NAMESPACE_BEGIN

class RigidBody2D : public Body2D
{
public:
	enum TYPE
	{
		DYNAMIC,
		KINEMATIC
	};

	const TYPE m_TYPE;

	RigidBody2D(TYPE type) : m_TYPE(type)
	{

	}

	inline void ReactKinematic(Physics2D* physics)
	{
		auto obj = physics->GetObject();

		auto& cachedTransform = obj->GetCachedTransform();
		auto& prevPosition = cachedTransform.GetTranslation();
		if (prevPosition == obj->Position())
		{
			// object doesn't move, nothing happend
			return;
		}

		//Vec2 impulse = { 0,0 };
		auto& collisionPairs = physics->CollisionPairs();

		/*std::sort(collisionPairs.begin(), collisionPairs.end(),
			[&](Collision2DPair* p1, Collision2DPair* p2)
			{
				auto n1 = p1->GetNormal(physics);
				auto n2 = p2->GetNormal(physics);

				if (!p1->result.HasCollision())
				{
					n1.x = FLT_MAX;
					n1.y = FLT_MAX;
				}

				if (!p2->result.HasCollision())
				{
					n2.x = FLT_MAX;
					n2.y = FLT_MAX;
				}
				
				if (n1.x == n2.x)
				{
					return n1.y < n2.y;
				}
				return n1.x < n2.x;
			}
		);*/

		for (auto& pair : collisionPairs)
		{
			auto another = pair->GetAnotherOf(physics);
			if (another->Body() != nullptr)
			{
				// kinematic object only reacts to static object
				// if not static object, doesn't react
				continue;
			}

			physics->Collider()->AdjustSelf(
				obj->Transform(), 
				another->Collider().get(), 
				another->GetObject()->Transform()
			);
		}
	}

	virtual void ReactFor(Physics2D* physics) override
	{
		switch (m_TYPE)
		{
		case DYNAMIC:
			assert(0); // =)))
			break;
		case KINEMATIC:
			ReactKinematic(physics);
			break;
		}
	}

};

NAMESPACE_END