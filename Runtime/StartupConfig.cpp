#include "StartupConfig.h"
#include "DebugVar.h"

#include "FileSystem/FileUtils.h"
#include "FileSystem/FileSystem.h"

#include "JSON/JSON.h"

#include "Runtime.h"

NAMESPACE_BEGIN

DebugVar DebugVar::s_instance = {};

void StartupConfig::LoadConfigFile(const String& path)
{
	if (!FileSystem::Get()->IsFileExisted(path))
	{
		return;
	}

	assert(std::filesystem::path(path.c_str()).is_absolute());

	configFilePath = path;

	byte* buffer = nullptr; size_t fileSize = 0;
	FileUtils::ReadFile(path, buffer, fileSize);
	auto config = json::parse((char*)buffer);
	FileUtils::FreeBuffer(buffer);

	if (config.contains("ForwardProject"))
	{
		String base = FileUtils::PopPath(path).c_str();
		String relative = config["ForwardProject"];
		LoadConfigFile(FileUtils::JoinPaths(base, relative));
		return;
	}

	Runtime::Get()->SetWorkingDirectory(FileUtils::PopPath(path));

	if (config.contains("BuildConfigs"))
	{
		auto& arr = config["BuildConfigs"];
		for (size_t i = 0; i < arr.size(); i++)
		{
			auto& buildConfigJson = arr[i];
			auto& buildConfig = buildConfigs.emplace_back();
			buildConfig.name = buildConfigJson["Name"];
			buildConfig.outputDirectories = buildConfigJson["OutputDirectories"];
		}
	}
}

NAMESPACE_END