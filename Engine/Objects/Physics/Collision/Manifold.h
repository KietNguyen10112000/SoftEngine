#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class Physics;

class Manifold
{
public:
	ID m_id = INVALID_ID;

	Physics* m_A;
	Physics* m_B;

	// if this manifold is duplicated, this field will not be null
	Manifold* refManifold = nullptr;

};

NAMESPACE_END