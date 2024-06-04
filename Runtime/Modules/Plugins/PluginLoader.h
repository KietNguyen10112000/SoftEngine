#pragma once

#include "Core/TypeDef.h"
#include "Core/Structures/STD/STDContainers.h"
#include "Core/Pattern/Singleton.h"
#include "Core/Structures/String.h"

NAMESPACE_BEGIN

class Plugin;
class Runtime;

class PluginLoader : public Singleton<PluginLoader>
{
private:
	std::vector<Plugin*> m_loadedPlugins;
	Plugin* m_currentLoadingPlugin = nullptr;

	String m_pluginPath;

private:
	Plugin* LoadPluginImpl(Runtime* engine, const wchar_t* path, ID idx = INVALID_ID);

public:
	// return false if any plugin failed to load
	bool LoadAll(Runtime* engine, const char* path, std::Vector<Plugin*>& output);
	void UnloadAll(Runtime* engine, std::Vector<Plugin*>& input, bool freeLib = false);

	void Unload(Runtime* engine, Plugin* input, bool freeLib = false);

	inline Plugin* GetCurrentLoadingPlugin()
	{
		return m_currentLoadingPlugin;
	}

#ifdef PLUGIN_ALLOW_HOT_RELOAD
private:
	void LoadAllHotReloadPlugin(Runtime* engine);

public:
	void ReloadAll(Runtime* engine);

	std::vector<Plugin*> GetHotReloadablePlugins();
#endif // PLUGIN_ALLOW_HOT_RELOAD


};

NAMESPACE_END