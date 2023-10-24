#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/Memory.h"
#include "Core/Structures/String.h"
#include "Core/Structures/STD/STDContainers.h"

#include "MainSystem/MainSystemInfo.h"

NAMESPACE_BEGIN

class Scene;
class Runtime;

#define DECLARE_PLUGIN(name)							\
EXTERN_C EXPORT Plugin* ___plugin(Runtime*);			\
EXTERN_C EXPORT void ___pluginInit(Runtime*);			\
EXTERN_C EXPORT void ___pluginFinal(Runtime*);

#define IMPL_PLUGIN(name)																\
Plugin* ___plugin(Runtime*) { static auto instance = new name(); return instance; };	\
void ___pluginInit(Runtime*) { Thread::InitializeForThisThreadInThisModule(); };		\
void ___pluginFinal(Runtime*) { Thread::FinalizeForThisThreadInThisModule(); };

#define PLUGIN_CTOR_NAME "___plugin"

#define PLUGIN_INITIALIZE_PER_THREAD_NAME "___pluginInit"
#define PLUGIN_FINALIZE_PER_THREAD_NAME "___pluginFinal"

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
	friend class MainComponentDB;

	size_t m_id = INVALID_ID;
	void* m_nativeHandle = nullptr;

	void* m_ctorFunc;
	void (*m_initFunc)(Runtime*);
	void (*m_finalFunc)(Runtime*);

	friend Plugin* PluginLoader_LoadPluginNative(Runtime* engine, const wchar_t* path, void*& outputHandle);

	// custom main components from plugin of each MainComponent type
	std::vector<String> m_customComps[MainSystemInfo::COUNT];

public:
	virtual void GetDesc(PLUGIN_DESC* output) = 0;
	virtual void Update() = 0;

	virtual void Initialize(Runtime* engine) = 0;
	virtual void Finalize(Runtime* engine) = 0;

public:
	inline const auto& GetComponentNameList(ID COMPONENT_ID) const
	{
		return m_customComps[COMPONENT_ID];
	}
};

NAMESPACE_END