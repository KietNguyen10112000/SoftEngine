#pragma once
#include "Core/TypeDef.h"

#include "Core/Pattern/Singleton.h"
#include "Core/Memory/SmartPointers.h"

#include "GraphicsFundamental.h"

NAMESPACE_BEGIN

enum class GRAPHICS_BACKEND_API
{
	DX12
};

class DebugGraphics;
class RenderingSystem;
class Camera;

class API Graphics : public Singleton<Graphics>
{
public:
	DebugGraphics* m_debugGraphics = nullptr;

	RenderingSystem* m_bindedRdSys = nullptr;

public:
	virtual ~Graphics() {};

public:
	static int Initilize(void* windowNativeHandle, GRAPHICS_BACKEND_API backendAPI);
	static void Finalize();

public:
	virtual SharedPtr<GraphicsPipeline> CreateRasterizerPipeline(const GRAPHICS_PIPELINE_DESC& desc) = 0;
	virtual SharedPtr<GraphicsShaderResource> CreateShaderResource(const GRAPHICS_SHADER_RESOURCE_DESC& desc) = 0;
	virtual SharedPtr<GraphicsRenderTarget> CreateRenderTarget(const GRAPHICS_RENDER_TARGET_DESC& desc) = 0;

	virtual SharedPtr<GraphicsPipelineInput> CreatePipelineInput(const GRAPHICS_PIPELINE_INPUT_DESC& desc) = 0;
	virtual SharedPtr<GraphicsVertexBuffer> CreateVertexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) = 0;
	virtual SharedPtr<GraphicsIndexBuffer> CreateIndexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) = 0;

public:
	inline auto GetDebugGraphics()
	{
		return m_debugGraphics;
	}

	inline void Bind(RenderingSystem* sys)
	{
		m_bindedRdSys = sys;
	}

	inline auto GetRenderingSystem()
	{
		return m_bindedRdSys;
	}

};

NAMESPACE_END