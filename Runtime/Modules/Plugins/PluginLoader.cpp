#include "PluginLoader.h"

#include "Core/Structures/String.h"

#include "TaskSystem/TaskSystem.h"

#include "FileSystem/FileUtils.h"

#include "Plugin.h"

#ifdef WIN32
#include <Windows.h>
#endif // WIN32
#undef Yield

NAMESPACE_BEGIN

using PluginCtor = Plugin* (*)(Runtime*);

struct PluginLoaderData
{
	std::vector<Plugin*, STDAllocatorMalloc<Plugin*>> loadedPlugins;
};

PluginLoaderData g_pluginData;

#ifdef WIN32

Plugin* PluginLoader_LoadPluginNative(Runtime* engine, const wchar_t* path, void*& outputHandle)
{
	auto handle = LoadLibraryW(path);

	if (handle == NULL)
	{
		return nullptr;
	}

	auto ctor = (PluginCtor)GetProcAddress(handle, PLUGIN_CTOR_NAME);
	
	auto ret = ctor(engine);

	ret->m_initFunc = (decltype(ret->m_initFunc))GetProcAddress(handle, PLUGIN_INITIALIZE_PER_THREAD_NAME);
	ret->m_finalFunc = (decltype(ret->m_finalFunc))GetProcAddress(handle, PLUGIN_FINALIZE_PER_THREAD_NAME);

	outputHandle = handle;

	return ret;
}

void PluginLoader_UnloadPluginNative(void* nativeHandle)
{
	FreeLibrary((HMODULE)nativeHandle);
}

#endif // WIN32

bool PluginLoader::LoadAll(Runtime* engine, const char* path, std::Vector<Plugin*>& output)
{
#ifdef WIN32
	const static auto ENDING = L".dll";
#endif // WIN32

	static TaskWaitingHandle taskHandle = { 0,0 };

	TaskSystem::PrepareHandle(&taskHandle);

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

				auto count = TaskSystem::GetWorkerCount();
				for (size_t i = 1; i < TaskSystem::GetWorkerCount(); i++)
				{
					Task task;
					task.Params() = plugin;
					task.Entry() = [](void* p)
					{
						auto plugin = (Plugin*)p;
						plugin->m_initFunc(0);
					};

					TaskSystem::SubmitForThread(&taskHandle, i, task);
				}

				plugin->m_initFunc(0);
				while (taskHandle.counter.load(std::memory_order_relaxed) != 1)
				{
					Thread::Sleep(5);
				}
				//TaskSystem::WaitForHandle(&taskHandle);
				taskHandle.counter--;

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

void PluginLoader::Unload(Runtime* engine, Plugin* input, bool freeLib)
{
	static TaskWaitingHandle taskHandle = { 0,0 };

	TaskSystem::PrepareHandle(&taskHandle);

	assert(input != nullptr);

	auto& plugin = g_pluginData.loadedPlugins[input->m_id];
	auto back = g_pluginData.loadedPlugins.back();
	back->m_id = input->m_id;
	plugin = back;
	g_pluginData.loadedPlugins.pop_back();

	auto handle = input->m_nativeHandle;
	input->Finalize(engine);

	for (size_t i = 1; i < TaskSystem::GetWorkerCount(); i++)
	{
		Task task;
		task.Params() = input;
		task.Entry() = [](void* p)
		{
			auto plugin = (Plugin*)p;
			plugin->m_finalFunc(0);
		};

		TaskSystem::SubmitForThread(&taskHandle, i, task);
	}

	input->m_finalFunc(0);
	while (taskHandle.counter.load(std::memory_order_relaxed) != 1)
	{
		Thread::Sleep(5);
	}
	taskHandle.counter--;
	//TaskSystem::WaitForHandle(&taskHandle);

	DELETE_PLUGIN(input);

	if (freeLib)
	{
		PluginLoader_UnloadPluginNative(handle);
	}
}

void PluginLoader::UnloadAll(Runtime* engine, std::Vector<Plugin*>& input, bool freeLib)
{
	for (auto& plugin : input)
	{
		Unload(engine, plugin, freeLib);
	}
}

NAMESPACE_END