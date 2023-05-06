#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

NAMESPACE_BEGIN

struct Collision2DResult;
class Collider2D;
class Physics2D;

class Body2D
{
public:
	virtual void ReactFor(Physics2D* physics) = 0;

};

NAMESPACE_END