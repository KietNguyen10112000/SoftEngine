#pragma once

#include "Plugins/Plugin.h"

#include "Core/Thread/Thread.h"

using namespace soft;

void Initialize(Engine* engine);
void Finalize(Engine* engine);

class ScriptBridge : public Plugin
{
public:
	virtual void GetDesc(PLUGIN_DESC* output) override
	{
		output->type = PLUGIN_TYPE::CALL_ONCE;
	}

	virtual void Update()
	{

	}

	virtual void Initialize(Engine* engine)
	{
		Thread::InitializeForThisThreadInThisModule();
		::Initialize(engine);
	}

	virtual void Finalize(Engine* engine)
	{
		::Finalize(engine);
		Thread::FinalizeForThisThreadInThisModule();
	}

};

DECLARE_PLUGIN(ScriptBridge);
IMPL_PLUGIN(ScriptBridge);