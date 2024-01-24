#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

enum PHYSICS_FLAG
{
	// default is 0
	PHYSICS_FLAG_CORRECT_LOCAL_TRANSFORM	= (1 << 0),

	PHYSICS_FLAG_ENABLE_COLLISION			= (1 << 1),
};

NAMESPACE_END