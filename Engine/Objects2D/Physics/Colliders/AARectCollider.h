#pragma once

#include "Collider2D.h"
#include "Collider2DUtils.h"

NAMESPACE_BEGIN

class AARectCollider : public Collider2D
{
public:
	AARect m_rect;
	Vec2 m_gap;

	AARectCollider(const AARect& rect, const Vec2& gap = { 0,0 }) : m_rect(rect), m_gap(gap)
	{

	}

	virtual AARect GetLocalAABB() override
	{
		return m_rect.GetLooser(m_gap);
	}

	// this collider is A, and the another is B
	virtual void Collide(
		const Mat3& selfTransform,
		const AARect& rect,
		const Mat3& rectTransform,
		Collision2DResult& output
	) override {
		//assert(selfTransform.GetRotation() == 0);
		//assert(rectTransform.GetRotation() == 0);

		auto aRect = m_rect;
		aRect.Transform(selfTransform);

		auto bRect = rect;
		bRect.Transform(rectTransform);

		auto n = bRect.GetCenter() - aRect.GetCenter();
		Vec2 absN = { std::abs(n.x), std::abs(n.y) };

		// Calculate half extents along x axis for each object 
		auto aExtent = aRect.GetHalfDimensions();
		auto bExtent = bRect.GetHalfDimensions();

		// Calculate overlap on x axis 
		float xOverlap = aExtent.x + bExtent.x - absN.x;

		if (xOverlap > 0.001f)
		{
			// Calculate overlap on y axis 
			float yOverlap = aExtent.y + bExtent.y - absN.y;

			if (yOverlap > 0.001f)
			{
				if (std::abs(xOverlap - yOverlap) < 0.001f)
				{
					output.normal = { 0, n.y / std::abs(n.y) };
					output.penetration = 0.1f;
					return;
				}

				if (xOverlap > yOverlap)
				{
					output.normal = { 0, n.y / std::abs(n.y) };
					output.penetration = yOverlap;
				}
				else
				{
					output.normal = { n.x / std::abs(n.x), 0 };
					output.penetration = xOverlap;
				}
			}
		}
	}

	virtual void Collide(
		const Mat3& selfTransform,
		const Rect2D& rect,
		const Mat3& rectTransform,
		Collision2DResult& output
	) override {
		auto A = AARectToRect(m_rect);
		A.Transform(selfTransform);

		auto B = rect;
		B.Transform(rectTransform);

		RectRectCollision(A, B, output);
	}


	// another will be A
	virtual void Collide(
		const Mat3& selfTransform,
		Collider2D* another,
		const Mat3& anotherTransform,
		Collision2DResult& output
	) override {
		another->Collide(anotherTransform, m_rect, selfTransform, output);
	}

	virtual void AdjustSelf(
		Transform2D& selfTransform,
		const Mat3& selfTransformMat,
		Collider2D* another,
		const Mat3& anotherTransform
	) override {
		Collision2DResult collision = {};
		another->Collide(anotherTransform, m_rect, selfTransformMat, collision);

		if (collision.HasCollision())
		{
			selfTransform.Translation() += collision.AB * collision.penetration;
		}
	}

	virtual bool RayQuery(const Mat3& selfTransform, Ray2D& ray, Ray2DQueryResult& output) override
	{
		return RectRayQuery(m_rect, selfTransform, ray, output);
	}

};

NAMESPACE_END