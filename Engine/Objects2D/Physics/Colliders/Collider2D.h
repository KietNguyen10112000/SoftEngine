#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

#include "../Collision/Collision2DResult.h"

#include "Objects2D/Scene2D/Ray/Ray2D.h"
#include "Objects2D/Scene2D/Ray/Ray2DQueryResult.h"

NAMESPACE_BEGIN

struct Collision2DResult;

class Collider2D
{
public:
	virtual AARect GetLocalAABB() = 0;

	// this collider is A, and the another is B
	virtual void Collide(
		const Mat3& selfTransform, 
		const AARect& rect, 
		const Mat3& rectTransform, 
		Collision2DResult& output
	) = 0;

	virtual void Collide(
		const Mat3& selfTransform,
		const Rect2D& rect, 
		const Mat3& rectTransform,
		Collision2DResult& output
	) = 0;


	// another will be A
	virtual void Collide(
		const Mat3& selfTransform,
		Collider2D* another, 
		const Mat3& anotherTransform,
		Collision2DResult& output
	) = 0;


	
	/*virtual void Adjust(
		const Transform2D& selfTransform,
		const AARect& rect,
		const Transform2D& anotherTransform,
		Transform2D& output
	) = 0;

	virtual void Adjust(
		const Transform2D& selfTransform,
		const Rect2D& rect,
		const Transform2D& anotherTransform,
		Transform2D& output
	) = 0;*/

	// make another is colliding to this collider become non-collide by moving yourself
	// modify selfTransform
	virtual void AdjustSelf(
		Transform2D& selfTransform,
		const Mat3& selfTransformMat,
		Collider2D* another,
		const Mat3& anotherTransform
	) = 0;

	virtual bool RayQuery(const Mat3& selfTransform, Ray2D& ray, Ray2DQueryResult& output) = 0;

};

NAMESPACE_END