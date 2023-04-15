#pragma once
#include "Core/TypeDef.h"

NAMESPACE_BEGIN

struct StartupConfig
{
	static StartupConfig s_instance;

	inline static auto& Get()
	{
		return s_instance;
	}

	bool isEnableRendering	= true;
	bool isEnablePhysics	= true;
	bool isEnableScript		= true;
	bool isEnableNetwork	= true;

	uint32_t maxThreads		= -1;
	uint32_t reservedThread = 4;

};

NAMESPACE_END