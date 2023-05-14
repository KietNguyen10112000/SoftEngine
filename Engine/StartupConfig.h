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

	bool isEnableRendering	= true;
	bool isEnablePhysics	= true;
	bool isEnableScript		= true;
	bool isEnableNetwork	= true;

	const char* pluginsPath			= "Plugins/";
	const char* resourcesPath		= "Resources/";
	const char* compiledShadersPath = "Shaders/";

	const char* windowTitle = "SoftEngine";
	int windowWidth = 960;
	int windowHeight = 720;
	float fixedDt = 0.016f;

	uint32_t maxThreads		= 2;
	uint32_t reservedThread = 0;

};

NAMESPACE_END