#pragma once

#include "Collider2D.h"
#include "Collider2DUtils.h"

NAMESPACE_BEGIN

class RectCollider : public Collider2D
{
public:
	Rect m_rect;
	Vec2 m_gap;

	RectCollider(const Rect& rect, const Vec2& gap = { 0,0 }) : m_rect(rect), m_gap(gap)
	{

	}

	virtual AARect GetLocalAABB() override
	{
		return RectToAARect(m_rect.GetLooser(m_gap));
	}

	// this collider is A, and the another is B
	virtual void Collide(
		const Mat3& selfTransform,
		const AARect& aaRect,
		const Mat3& aaRectTransform,
		Collision2DResult& output
	) override {
		auto A = m_rect;
		A.Transform(selfTransform);

		auto B = AARectToRect(aaRect);
		B.Transform(aaRectTransform);

		RectRectCollision(A, B, output);
	}

	virtual void Collide(
		const Mat3& selfTransform,
		const Rect2D& rect,
		const Mat3& rectTransform,
		Collision2DResult& output
	) override {
		auto A = m_rect;
		A.Transform(selfTransform);

		auto B = rect;
		B.Transform(rectTransform);

		RectRectCollision(A, B, output);

		//if (output.penetration > 0)
		//	std::cout << output.penetration << "\n";
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