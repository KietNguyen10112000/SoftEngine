#include "PluginLoader.h"

#include "Core/Structures/String.h"

#include "TaskSystem/TaskSystem.h"

#include "FileSystem/FileUtils.h"
#include "FileSystem/FileSystem.h"

#include "Plugin.h"

#ifdef WIN32
#include <Windows.h>
#endif // WIN32
#undef Yield

NAMESPACE_BEGIN

using PluginCtor = Plugin* (*)(Runtime*);

#ifdef WIN32

Plugin* PluginLoader_LoadPluginNative(Runtime* engine, const wchar_t* path, void*& outputHandle)
{
	auto handle = LoadLibraryW(path);

	if (handle == NULL)
	{
		auto err = GetLastError();
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

Plugin* PluginLoader::LoadPluginImpl(Runtime* engine, const wchar_t* filePath, ID idx)
{
	static TaskWaitingHandle taskHandle = { 0,0 };

#ifdef WIN32
	const static wchar_t* ENDING = L".dll";
#endif // WIN32

	std::wstring_view ending = ENDING;

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

	assert(valid);

	void* handle = nullptr;
	auto plugin = PluginLoader_LoadPluginNative(engine, filePath, handle);

	if (plugin)
	{
		plugin->m_filePath = filePath;
		TaskSystem::PrepareHandle(&taskHandle);

		m_currentLoadingPlugin = plugin;

		if (idx != INVALID_ID)
		{
			plugin->m_id = idx;
			m_loadedPlugins[idx] = plugin;
		}
		else
		{
			plugin->m_id = m_loadedPlugins.size();
			m_loadedPlugins.push_back(plugin);
		}
		
		plugin->m_nativeHandle = handle;

		auto currentThreadId = Thread::GetID();

		auto count = TaskSystem::GetWorkerCount();
		for (size_t i = 0; i < TaskSystem::GetWorkerCount(); i++)
		{
			if (i == currentThreadId)
			{
				continue;
			}

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

		m_currentLoadingPlugin = nullptr;
	}

	return plugin;
}

bool PluginLoader::LoadAll(Runtime* engine, const char* path, std::Vector<Plugin*>& output)
{
	if (m_pluginPath.empty())
	{
		m_pluginPath = path;
	}

	bool ret = true;

	if (!FileUtils::IsExist(path))
	{
		std::cout << "[ERROR]: Plugins path '" << path << "' doesn't exist!\n";
		return false;
	}

	auto LoadPlugin = [&](const wchar_t* filePath)
	{
#ifdef WIN32
		const static auto EXTENSION = "dll";
#endif // WIN32
		auto ext = FileUtils::GetExtension(filePath);
		if (ext != EXTENSION)
		{
			return;
		}

		auto plugin = LoadPluginImpl(engine, filePath);
		if (!plugin)
		{
			ret = false;
		}
	};

	FileUtils::ForEachFiles(path, LoadPlugin);

#ifdef PLUGIN_ALLOW_HOT_RELOAD
	LoadAllHotReloadPlugin(engine);
#endif // PLUGIN_ALLOW_HOT_RELOAD

	return ret;
}

void PluginLoader::Unload(Runtime* engine, Plugin* input, bool freeLib)
{
	static TaskWaitingHandle taskHandle = { 0,0 };

	TaskSystem::PrepareHandle(&taskHandle);

	assert(input != nullptr);

	auto& plugin = m_loadedPlugins[input->m_id];
	auto back = m_loadedPlugins.back();
	back->m_id = input->m_id;
	plugin = back;
	m_loadedPlugins.pop_back();

	auto currentThreadId = Thread::GetID();

	auto handle = input->m_nativeHandle;
	input->Finalize(engine);

	/*if (Thread::GetID() != currentThreadId)
	{
		TaskSystem::SubmitForThread(
			{
				[](void* arg)
				{
					Thread::SwitchToFiber(FiberPool::Get(Thread::GetID()), true);
				},
				(void*)0
			},
			currentThreadId
		);
		Thread::SwitchToFiber(FiberPool::Take(), true);
	}*/

	for (size_t i = 0; i < TaskSystem::GetWorkerCount(); i++)
	{
		if (i == currentThreadId)
		{
			continue;
		}

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
	for (auto& plugin : m_loadedPlugins)
	{
		Unload(engine, plugin, freeLib);
	}
}

#ifdef PLUGIN_ALLOW_HOT_RELOAD

void PluginLoader::LoadAllHotReloadPlugin(Runtime* engine)
{
	auto startIdx = m_loadedPlugins.size();
	auto LoadPlugin = [&](const wchar_t* filePath)
	{
#ifdef WIN32
		const static auto EXTENSION = "dll";
#endif // WIN32
		auto ext = FileUtils::GetExtension(filePath);
		if (ext != EXTENSION)
		{
			return;
		}

		LoadPluginImpl(engine, filePath);
	};

	auto hotReloadPath = m_pluginPath + "HotReload/";
	auto hotReloadPathReal = FileSystem::Get()->GetCacheDirectory() + "Plugins/HotReload/";

	if (!FileUtils::IsExist(hotReloadPath.c_str()))
	{
		std::filesystem::create_directories(hotReloadPath.c_str());
	}

	if (!FileUtils::IsExist(hotReloadPathReal.c_str()))
	{
		std::filesystem::create_directories(hotReloadPathReal.c_str());
	}

	FileUtils::ForEachFiles(hotReloadPath.c_str(),
		[&](const wchar_t* filePath)
		{
			auto fullpath = String(filePath);
			if (FileSystem::Get()->IsFileChanged(fullpath.c_str()))
			{
				auto fileNameWithExtension = FileUtils::GetLastName(fullpath.c_str());
				if (FileUtils::GetExtension(fileNameWithExtension) != "dll")
				{
					return;
				}
				std::filesystem::copy_file(filePath, (hotReloadPathReal + fileNameWithExtension).c_str(), std::filesystem::copy_options::overwrite_existing);
			}
		}
	);

	FileUtils::ForEachFiles(hotReloadPath.c_str(),
		[&](const wchar_t* filePath)
		{
			auto fullpath = String(filePath);
			{
				auto fileNameWithExtension = FileUtils::GetLastName(fullpath.c_str());
				if (FileUtils::GetExtension(fileNameWithExtension) != "dll")
				{
					return;
				}
				auto realPath = (hotReloadPathReal + fileNameWithExtension);
				std::string realPathStr = (realPath.c_str());
				std::wstring path2 = StringUtils::StringToWString(realPathStr);
				LoadPlugin(path2.c_str());
			}
		}
	);

	//FileUtils::ForEachFiles(hotReloadPathReal, LoadPlugin);

	for (size_t i = startIdx; i < m_loadedPlugins.size(); i++)
	{
		auto plugin = m_loadedPlugins[i];
		plugin->m_isHotReloadable = true;
	}
}

void PluginLoader::ReloadAll(Runtime* engine)
{
	size_t startIdx = INVALID_ID;
	for (auto& plugin : m_loadedPlugins)
	{
		if (plugin->m_isHotReloadable)
		{
			Unload(engine, plugin, true);
			plugin = nullptr;
			if (startIdx == INVALID_ID)
			{
				startIdx = &plugin - m_loadedPlugins.data();
			}
		}
	}

	if (startIdx == INVALID_ID)
	{
		return;
	}

	m_loadedPlugins.resize(startIdx);

	LoadAllHotReloadPlugin(engine);
}

std::vector<Plugin*> PluginLoader::GetHotReloadablePlugins()
{
	std::vector<Plugin*> ret;
	for (auto& plugin : m_loadedPlugins)
	{
		if (plugin->m_isHotReloadable)
		{
			ret.push_back(plugin);
		}
	}
	return ret;
}

#endif // PLUGIN_ALLOW_HOT_RELOAD

NAMESPACE_END