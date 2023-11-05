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

	bool isEnableGUIEditing	= true;
	bool padd[3];

	const char* pluginsPath			= "Plugins/";
	const char* resourcesPath		= "Resources/";
	const char* compiledShadersPath = "Shaders/";
	const char* executablePath = nullptr;

	const char* windowTitle = "SoftEngine";
	int windowWidth = 1920;
	int windowHeight = 1080;
	float fixedDt = 0;

	uint32_t numThreads		= -1;
	uint32_t maxThreads		= -1;
	uint32_t reservedThread = 4;

};

NAMESPACE_END