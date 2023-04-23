#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class ColliderId
{
public:
	constexpr static size_t MAX_COLLIDERS			= 128;

	constexpr static size_t SPHERE_COLLIDER_ID		= 0;
	constexpr static size_t BOX_COLLIDER_ID			= 1;

};

NAMESPACE_END