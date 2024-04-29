#include "SerializableDB.h"

#include "MainSystem/MainSystemInfo.h"

#include "Modules/Plugins/PluginLoader.h"
#include "Modules/Plugins/Plugin.h"

#include "MainSystem/Scripting/Components/Script.h"
#include "MainSystem/Scripting/ScriptMeta.h"

NAMESPACE_BEGIN

void SerializableDB::AddRecord(const SerializableRecord& record)
{
	assert(m_records.find(record) == m_records.end());

	m_records.insert(record);

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

void SerializableDB::RemoveRecord(const char* name)
{
	m_records.erase({ 0, name, nullptr });
}

NAMESPACE_END