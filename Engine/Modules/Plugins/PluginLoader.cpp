#include "PluginLoader.h"

#include "Core/Structures/String.h"

#include "FileSystem/FileUtils.h"

#include "Plugin.h"

#ifdef WIN32
#include <Windows.h>
#endif // WIN32


NAMESPACE_BEGIN

using PluginCtor = Plugin* (*)(Engine*);

struct PluginLoaderData
{
	std::vector<Plugin*, STDAllocatorMalloc<Plugin*>> loadedPlugins;
};

PluginLoaderData g_pluginData;

#ifdef WIN32

Plugin* PluginLoader_LoadPluginNative(Engine* engine, const wchar_t* path, void*& outputHandle)
{
	auto handle = LoadLibraryW(path);

	if (handle == NULL)
	{
		return nullptr;
	}

	auto ctor = (PluginCtor)GetProcAddress(handle, PLUGIN_CTOR_NAME);
	return ctor(engine);
}

void PluginLoader_UnloadPluginNative(void* nativeHandle)
{
	FreeLibrary((HMODULE)nativeHandle);
}

#endif // WIN32

bool PluginLoader::LoadAll(Engine* engine, const char* path, std::Vector<Plugin*>& output)
{
#ifdef WIN32
	const static auto ENDING = L".dll";
#endif // WIN32

	std::wstring_view ending = ENDING;

	bool ret = true;

	if (!FileUtils::IsExist(path))
	{
		std::cout << "[ERROR]: Plugins path '" << path << "' doesn't exist!\n";
		return false;
	}

	FileUtils::ForEachFiles(path,
		[&](const wchar_t* filePath)
		{
			std::wstring_view fullString = filePath;

			bool valid = false;
			if (fullString.length() >= ending.length()) 
			{
				valid = (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
			}
			else 
			{
				valid = false;
			}

			if (!valid) return;

			void* handle = nullptr;
			auto plugin = PluginLoader_LoadPluginNative(engine, filePath, handle);

			if (plugin)
			{
				plugin->m_id = g_pluginData.loadedPlugins.size();
				plugin->m_nativeHandle = handle;
				g_pluginData.loadedPlugins.push_back(plugin);

				plugin->Initialize(engine);
			}
			else
			{
				ret = false;
			}
			
			output.push_back(plugin);
		}
	);
	return ret;
}

void PluginLoader::Unload(Engine* engine, Plugin* input)
{
	assert(input != nullptr);

	auto& plugin = g_pluginData.loadedPlugins[input->m_id];
	auto back = g_pluginData.loadedPlugins.back();
	back->m_id = input->m_id;
	plugin = back;
	g_pluginData.loadedPlugins.pop_back();

	auto handle = input->m_nativeHandle;
	input->Finalize(engine);
	DELETE_PLUGIN(input);
	PluginLoader_UnloadPluginNative(handle);
}

void PluginLoader::UnloadAll(Engine* engine, std::Vector<Plugin*>& input)
{
	for (auto& plugin : input)
	{
		Unload(engine, plugin);
	}
}

NAMESPACE_END