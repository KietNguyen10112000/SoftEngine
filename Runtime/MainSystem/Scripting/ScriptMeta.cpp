#include "ScriptMeta.h"

NAMESPACE_BEGIN

ScriptMeta::~ScriptMeta()
{
	for (auto& v : m_metaDatas)
	{
		delete v;
	}
	m_metaDatas.clear();
}

void ScriptMeta::RegisterScriptMetaData(ScriptMetaData* metaData)
{
	assert(m_metaDatas.find(metaData) == m_metaDatas.end());
	m_metaDatas.insert(metaData);

	metaData->schedulerId = m_schedulerCounter++;
}

void ScriptMeta::UnregisterScriptMetaData(ScriptMetaData* metaData)
{
	m_metaDatas.erase(metaData);
}

NAMESPACE_END
