#pragma once

#include "Collider2D.h"
#include "Collider2DUtils.h"

NAMESPACE_BEGIN

class CircleCollider : public Collider2D
{
public:
	Circle m_circle;

	CircleCollider(const Vec2& center, float radius) : m_circle(center, radius)
	{
		
	}

	virtual AARect GetLocalAABB()
	{
		return AARect(m_circle.m_center - Vec2(m_circle.m_radius), Vec2(m_circle.m_radius * 2.0f));
	}

	// this collider is A, and the another is B
	virtual void Collide(
		const Mat3& selfTransform,
		const Circle& circle,
		const Mat3& circleTransform,
		Collision2DResult& output
	) override {
		auto A = m_circle;
		A.Transform(selfTransform);

		auto B = circle;
		B.Transform(circleTransform);

		auto AB = (B.m_center - A.m_center);
		auto len = AB.Length();
		AB /= len;

		if (A.m_radius + B.m_radius > len)
		{
			output.penetration = 0;
			return;
		}

		output.penetration = A.m_radius + B.m_radius - len;
		output.AB = AB;
	};

	virtual void Collide(
		const Mat3& selfTransform,
		const AARect& rect,
		const Mat3& rectTransform,
		Collision2DResult& output
	) override {
		auto A = m_circle;
		A.Transform(selfTransform);

		auto B = rect;
		B.Transform(rectTransform);

		RectCircleCollision(B, A, output);
		output.AB = -output.AB;
	};

	virtual void Collide(
		const Mat3& selfTransform,
		const Rect2D& rect,
		const Mat3& rectTransform,
		Collision2DResult& output
	) override {
		auto A = m_circle;
		A.Transform(selfTransform);

		auto B = rect;
		B.Transform(rectTransform);

		RectCircleCollision(B, A, output);
		output.AB = -output.AB;
	};


	// another will be A
	virtual void Collide(
		const Mat3& selfTransform,
		Collider2D* another,
		const Mat3& anotherTransform,
		Collision2DResult& output
	) override {
		another->Collide(anotherTransform, m_circle, selfTransform, output);
	};

	// make another is colliding to this collider become non-collide by moving yourself
	// modify selfTransform
	virtual void AdjustSelf(
		Transform2D& selfTransform,
		const Mat3& selfTransformMat,
		Collider2D* another,
		const Mat3& anotherTransform
	) override {

	};

	virtual void RayQuery(const Mat3& selfTransform, Ray2D& ray, Ray2DQueryResult& output) override 
	{
		auto circle = m_circle;
		circle.Transform(selfTransform);

		auto line = Line2D::FromPointAndDirection(ray.begin, ray.direction);

		auto side = line.ValueOf(circle.m_center);
		auto dist = std::abs(side);

		if (dist > circle.m_radius)
		{
			return;
		}

		auto n = line.normal;
		if (side > 0)
		{
			n = -n;
		}

		if (std::abs(dist - circle.m_radius) < 0.00001f)
		{
			auto p = circle.m_center + n * circle.m_radius;
			output.StoreIfSameSide(ray, p);
		}

		auto dist2 = dist * dist;
		auto r2 = circle.m_radius * circle.m_radius;
		auto offset = std::sqrt(r2 - dist2);
		auto p = circle.m_center + n * circle.m_radius;

		auto tangen = Vec2(-ray.direction.y, ray.direction.x);
		auto p1 = p + offset * tangen;
		auto p2 = p - offset * tangen;

		output.StoreIfSameSide(ray, p1);
		output.StoreIfSameSide(ray, p2);
	};

};

NAMESPACE_END