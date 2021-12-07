#pragma once

class Engine;
class Input;
class Resource;
class IRenderer;
class RenderPipelineManager;

class Global
{
public:
	inline static Engine* engine = nullptr;
	inline static IRenderer* renderer = nullptr;
	inline static Input* input = nullptr;
	inline static Resource* resourceManager = nullptr;
	inline static RenderPipelineManager* rplManager = nullptr;

};