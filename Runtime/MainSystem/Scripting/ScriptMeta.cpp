#include "ScriptMeta.h"

#include "Components/Script.h"

NAMESPACE_BEGIN

ScriptMeta::ScriptMeta()
{
	m_freeSchedulerID.reserve(8 * KB);

	if (onGUIVtbIdx == INVALID_ID)
	{
		onGUIVtbIdx = VTableIndex<Script>(&Script::OnGUI);
		//onCollideVtbIdx			= VTableIndex<Script>(&Script::OnCollide);
		//onCollisionEnterVtbIdx	= VTableIndex<Script>(&Script::OnCollisionEnter);
		//onCollisionExitVtbIdx		= VTableIndex<Script>(&Script::OnCollisionExit);
		onUpdateVtbIdx = VTableIndex<Script>(&Script::OnUpdate);
	}
}

ScriptMeta::~ScriptMeta()
{
	for (auto& v : m_metaDatas)
	{
		delete v;
	}
	m_metaDatas.clear();
}

//void ScriptMeta::RegisterScriptMetaData(ScriptMetaData* metaData)
//{
//	assert(m_metaDatas.find(metaData) == m_metaDatas.end());
//	m_metaDatas.insert(metaData);
//
//	metaData->schedulerId = m_schedulerCounter++;
//}
//
//void ScriptMeta::UnregisterScriptMetaData(ScriptMetaData* metaData)
//{
//	m_metaDatas.erase(metaData);
//}

ScriptMetaData* ScriptMeta::AllocateMetaData(const char* className)
{
	ScriptMetaData metaData;
	metaData.className = className;

	auto it = m_metaDatas.find(&metaData);
	if (it == m_metaDatas.end())
	{
		auto ret = new ScriptMetaData();
		ret->className = metaData.className;

		if (m_freeSchedulerID.empty()) 
		{
			ret->schedulerId = m_schedulerCounter++;
		}
		else
		{
			ret->schedulerId = m_freeSchedulerID.back();
			m_freeSchedulerID.pop_back();
		}

		m_metaDatas.insert(ret);
		return ret;
	}

	(*it)->refCount++;
	return *it;
}

void ScriptMeta::ForceDeallocateMetaData(const char* className)
{
	ScriptMetaData metaData;
	metaData.className = className;

	auto it = m_metaDatas.find(&metaData);
	if (it == m_metaDatas.end())
	{
		return;
	}

	auto meta = *it;
	assert(meta->refCount == 1); // reached assertion mean u are trying to deallocate built-in Script component likes FPPCameraScript,...
	m_freeSchedulerID.push_back(meta->schedulerId);
	m_metaDatas.erase(it);
	delete meta;
}

NAMESPACE_END
