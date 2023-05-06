#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

#include "../Collision/Collision2DResult.h"

NAMESPACE_BEGIN

struct Collision2DResult;

class Collider2D
{
public:
	virtual AARect GetLocalAABB() = 0;

	// this collider is A, and the another is B
	virtual void Collide(
		const Transform2D& selfTransform, 
		const AARect& rect, 
		const Transform2D& rectTransform, 
		Collision2DResult& output
	) = 0;

	virtual void Collide(
		const Transform2D& selfTransform,
		const Rect2D& rect, 
		const Transform2D& rectTransform, 
		Collision2DResult& output
	) = 0;


	// another will be A
	virtual void Collide(
		const Transform2D& selfTransform, 
		Collider2D* another, 
		const Transform2D& anotherTransform,
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
		Collider2D* another,
		const Transform2D& anotherTransform
	) = 0;

};

NAMESPACE_END