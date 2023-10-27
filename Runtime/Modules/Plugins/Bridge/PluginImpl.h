#pragma once

#include "Plugins/Plugin.h"

#include "Core/Thread/Thread.h"

#include "Plugins/Bridge/ImGuiBridge.h"

using namespace soft;

void RegisterCustomMainComponents();
void Initialize(Runtime* runtime);
void Finalize(Runtime* runtime);

class PluginImpl : public Plugin
{
public:
	virtual void GetDesc(PLUGIN_DESC* output) override
	{
		output->type = PLUGIN_TYPE::CALL_ONCE;
	}

	virtual void Update()
	{

	}

	virtual void Initialize(Runtime* runtime)
	{
		ImGuiBridge::InitializeImGui();
		::Initialize(runtime);
		::RegisterCustomMainComponents();
	}

	virtual void Finalize(Runtime* runtime)
	{
		::Finalize(runtime);
		ImGuiBridge::FinalizeImGui();
	}

};

DECLARE_PLUGIN(PluginImpl);
IMPL_PLUGIN(PluginImpl);