#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

class Plugin;
class Engine;

class PluginLoader
{
public:
	// return false if any plugin failed to load
	static bool LoadAll(Engine* engine, const char* path, std::Vector<Plugin*>& output);
	static void UnloadAll(Engine* engine, std::Vector<Plugin*>& input, bool freeLib = false);

	static void Unload(Engine* engine, Plugin* input, bool freeLib = false);

};

NAMESPACE_END