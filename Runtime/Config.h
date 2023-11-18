#pragma once

#include "Core/TypeDef.h"

NAMESPACE_BEGIN

class Config
{
public:
	constexpr static size_t NUM_DEFER_BUFFER = 2;

	constexpr static bool ORDERED_CHILD = true;

	constexpr static bool ENABLE_DEBUG_GRAPHICS = true;

};

NAMESPACE_END