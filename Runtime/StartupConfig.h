#pragma once
#include "Core/TypeDef.h"
#include "Core/Structures/String.h"
#include "Core/Pattern/Singleton.h"

NAMESPACE_BEGIN

struct StartupConfig : public Singleton<StartupConfig>
{
	struct BuildConfig
	{
		String name;
		std::vector<String> outputDirectories;
	};

	bool isEnableRendering	= true;
	bool isEnablePhysics	= true;
	bool isEnableScript		= true;
	bool isEnableNetwork	= true;

	bool isEnableGUIEditing	= true;
	bool padd[3];

	String configFilePath = nullptr;
	std::vector<BuildConfig> buildConfigs;
	String currentConfigName = nullptr;
	const char* executablePath = nullptr;

	const char* windowTitle = "SoftEngine";
	int windowWidth = 1920;
	int windowHeight = 1080;
	float fixedDt = 0;

	uint32_t numThreads		= -1;
	uint32_t maxThreads		= -1;
	uint32_t reservedThread = 4;

	void LoadConfigFile(const String& path);

};

NAMESPACE_END