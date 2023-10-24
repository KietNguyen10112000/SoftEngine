#pragma once

#include "Core/Structures/String.h"
#include "Core/Pattern/Singleton.h"
#include "Core/TemplateUtils/TemplateUtils.h"

#include <bitset>

NAMESPACE_BEGIN

struct ScriptMetaData
{
	std::bitset<64> overriddenVtbIdx = {};
	String className;
	ID schedulerId = INVALID_ID;
	size_t refCount = 1;
};

class API ScriptMeta : public Singleton<ScriptMeta>
{
	struct CMP
	{
		bool operator() (const ScriptMetaData* a, const ScriptMetaData* b) const {
			return a->className < b->className;
		}
	};

public:
	size_t onGUIVtbIdx					= INVALID_ID;
	size_t onCollideVtbIdx				= INVALID_ID;
	size_t onCollisionEnterVtbIdx		= INVALID_ID;
	size_t onCollisionExitVtbIdx		= INVALID_ID;
	size_t onUpdateVtbIdx				= INVALID_ID;

	std::set<ScriptMetaData*, CMP, STLNodeGlobalAllocator<ScriptMetaData*>> m_metaDatas;
	std::vector<ID> m_freeSchedulerID;

	ID m_schedulerCounter = 0;

	ScriptMeta();
	~ScriptMeta();

	//void RegisterScriptMetaData(ScriptMetaData* metaData);
	//void UnregisterScriptMetaData(ScriptMetaData* metaData);

	ScriptMetaData* AllocateMetaData(const char* className);

	void ForceDeallocateMetaData(const char* className);

};

NAMESPACE_END