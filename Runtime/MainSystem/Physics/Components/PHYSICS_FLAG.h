#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

enum PHYSICS_FLAG
{
	// default is 0
	PHYSICS_FLAG_CORRECT_LOCAL_TRANSFORM	= (1 << 0),

	// default 0, should this component receive collision result from physics system
	PHYSICS_FLAG_COLLISION_RESULT			= (1 << 1),
};

NAMESPACE_END