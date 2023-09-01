#pragma once

#include "Plugins/Plugin.h"

#include "Core/Thread/Thread.h"

#include "Plugins/Bridge/ImGuiBridge.h"

using namespace soft;

void Initialize(Runtime* engine);
void Finalize(Runtime* engine);

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

	virtual void Initialize(Runtime* engine)
	{
		ImGuiBridge::InitializeImGui();
		::Initialize(engine);
	}

	virtual void Finalize(Runtime* engine)
	{
		::Finalize(engine);
		ImGuiBridge::FinalizeImGui();
	}

};

DECLARE_PLUGIN(ScriptBridge);
IMPL_PLUGIN(ScriptBridge);