#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

struct PHYSICS_FILTER_FLAG
{
	enum FLAG
	{
		CCT						= 1 << 0,
		CALLBACK				= 1 << 1,
		FAMILY_NO_COLLIDE		= 1 << 2
	};
};

NAMESPACE_END