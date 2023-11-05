#pragma once

#include "Core/TypeDef.h"

#include "Core/Structures/STD/STDContainers.h"
#include "Core/Structures/Raw/ConcurrentList.h"

#include "Runtime/Config.h"

NAMESPACE_BEGIN

class Scene;
class Script;
class ScriptingSystem;

class ScriptScheduler
{
public:
	constexpr static size_t NUM_DEFER_BUFFER = Config::NUM_DEFER_BUFFER;

	std::vector<Script*> m_onGUIs;
	std::vector<Script*> m_onUpdates;

	raw::ConcurrentArrayList<Script*> m_asyncCalls[NUM_DEFER_BUFFER];

	std::atomic<size_t> m_flushAsyncCallsCount = 0;

public:
	ScriptScheduler();

	void CallOnUpdate(float dt);
	void CallOnGUI();

	bool RecordAsyncCall(ScriptingSystem* system, Script* script);
	void FlushAsyncCalls(Scene* scene);
};

NAMESPACE_END