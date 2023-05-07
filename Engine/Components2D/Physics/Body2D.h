#pragma once

#include "Physics2D.h"

#include "Math/Math.h"

#include "Objects2D/Physics/Collision/Collision2DPair.h"

NAMESPACE_BEGIN

struct Collision2DResult;
class Collider2D;
class Physics2D;

class Body2D : public Physics2D
{
protected:
	std::Vector<Collision2DPair*> m_collisionPlanes;

public:
	Body2D(const SharedPtr<Collider2D>& collider) : Physics2D(Physics2D::DYNAMIC_BODY, collider) {};

public:
	// just take single plane of collision
	inline void FilterDuplicateCollision2DResult()
	{
		auto comparator = [&](Collision2DPair* p1, Collision2DPair* p2)
		{
			auto n1 = p1->GetNormal(this);
			auto n2 = p2->GetNormal(this);
			if (n1.x == n2.x)
			{
				return n1.y < n2.y;
			}
			return n1.x < n2.x;
		};

		auto equals = [&](Collision2DPair* p1, Collision2DPair* p2)
		{
			auto n1 = p1->GetNormal(this);
			auto n2 = p2->GetNormal(this);
			return n1 == n2;
		};

		m_collisionPlanes.clear();
		for (auto& pair : CollisionPairs())
		{
			if (!pair->result.HasCollision()) continue;

			//auto end = m_collisionPlanes.data() + m_collisionPlanes.size();
			auto it = std::lower_bound(
				m_collisionPlanes.begin(),
				m_collisionPlanes.end(),
				pair,
				comparator
			);

			bool equalNormal = it == m_collisionPlanes.end() ? false : equals(*it, pair);
			if (it == m_collisionPlanes.end() || !equalNormal)
			{
				m_collisionPlanes.insert(it, pair);
				continue;
			}

			if (equalNormal)
			{
				if (pair->result.penetration > (*it)->result.penetration)
				{
					// store max penetration
					*it = pair;
				}
			}
		}
	}

};

NAMESPACE_END