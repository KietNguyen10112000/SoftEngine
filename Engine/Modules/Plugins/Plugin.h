#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"

NAMESPACE_BEGIN

class Scene;
class Engine;

#define DECLARE_PLUGIN(name) EXTERN_C EXPORT Plugin* ___plugin(Engine*);
#define IMPL_PLUGIN(name) Plugin* ___plugin(Engine*) { return new name(); };
#define PLUGIN_CTOR_NAME "___plugin"
#define DELETE_PLUGIN(plugin) delete plugin;

enum class PLUGIN_TYPE
{
	CALL_ONCE,
	INTERVAL,
};

struct PLUGIN_DESC
{
	PLUGIN_TYPE type;
};

class Plugin
{
private:
	friend class PluginLoader;
	size_t m_id = INVALID_ID;
	void* m_nativeHandle = nullptr;

public:
	virtual void GetDesc(PLUGIN_DESC* output) = 0;
	virtual void Update() = 0;

	virtual void Initialize(Engine* engine) = 0;
	virtual void Finalize(Engine* engine) = 0;
};

NAMESPACE_END