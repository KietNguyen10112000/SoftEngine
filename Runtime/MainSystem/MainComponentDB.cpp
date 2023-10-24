#include "MainComponentDB.h"

#include "MainSystemInfo.h"

#include "Modules/Plugins/PluginLoader.h"
#include "Modules/Plugins/Plugin.h"

#include "Scripting/Components/Script.h"
#include "Scripting/ScriptMeta.h"

NAMESPACE_BEGIN

void MainComponentDB::AddRecord(const ComponentRecord& record)
{
	assert(m_compRecords.find(record) == m_compRecords.end());

	m_compRecords.insert(record);

	if (record.COMPONENT_ID == MainSystemInfo::SCRIPTING_ID)
	{
		auto scriptHandle = record.ctor();
		auto script = (Script*)scriptHandle.Get();
		script->GetScriptMetaData();
	}

	auto plugin = PluginLoader::GetCurrentLoadingPlugin();
	if (plugin)
	{
		plugin->m_customComps[record.COMPONENT_ID].push_back(record.name);
	}
}

void MainComponentDB::RemoveRecord(const char* name)
{
	m_compRecords.erase({ 0, name, nullptr });
}

NAMESPACE_END