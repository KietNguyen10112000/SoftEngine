#pragma once
#include "Core/TypeDef.h"

NAMESPACE_BEGIN

struct DebugVar
{
	static DebugVar s_instance;

	inline static auto& Get()
	{
		return s_instance;
	}

	size_t refreshedAABBCount = 0;
	size_t debugVar1 = 0;
	size_t fps = 0;
	bool movingObject = false;

};

NAMESPACE_END