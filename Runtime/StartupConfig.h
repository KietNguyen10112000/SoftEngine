#pragma once
#include "Core/TypeDef.h"

NAMESPACE_BEGIN

struct API StartupConfig
{
	static StartupConfig s_instance;

	inline static auto& Get()
	{
		return s_instance;
	}

	bool isEnableRendering	= false;
	bool isEnablePhysics	= false;
	bool isEnableScript		= false;
	bool isEnableNetwork	= false;

	const char* pluginsPath			= "Plugins/";
	const char* resourcesPath		= "Resources/";
	const char* compiledShadersPath = "Shaders/";

	const char* windowTitle = "SoftEngine";
	int windowWidth = -1;
	int windowHeight = -1;
	float fixedDt = 0.016f;

	uint32_t numThreads		= -1;
	uint32_t maxThreads		= -1;
	uint32_t reservedThread = 0;

};

NAMESPACE_END