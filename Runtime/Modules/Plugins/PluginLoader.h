#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/STD/STDContainers.h"

NAMESPACE_BEGIN

class Plugin;
class Runtime;

class PluginLoader
{
public:
	// return false if any plugin failed to load
	static bool LoadAll(Runtime* engine, const char* path, std::Vector<Plugin*>& output);
	static void UnloadAll(Runtime* engine, std::Vector<Plugin*>& input, bool freeLib = false);

	static void Unload(Runtime* engine, Plugin* input, bool freeLib = false);

};

NAMESPACE_END