#pragma once

#include "TypeDef.h"
#include "Modules/Graphics/Graphics.h"

NAMESPACE_DX12_BEGIN

class DX12Graphics : public Graphics
{
public:
	DX12Graphics(void* hwnd);
	~DX12Graphics();

public:
	// Inherited via Graphics
	virtual SharedPtr<GraphicsPipeline> CreateRasterizerPipeline(const GRAPHICS_PIPELINE_DESC& desc) override;
	virtual SharedPtr<GraphicsShaderResource> CreateShaderResource(const GRAPHICS_SHADER_RESOURCE_DESC& desc) override;
	virtual SharedPtr<GraphicsRenderTarget> CreateRenderTarget(const GRAPHICS_RENDER_TARGET_DESC& desc) override;
	virtual SharedPtr<GraphicsPipelineInput> CreatePipelineInput(const GRAPHICS_PIPELINE_INPUT_DESC& desc) override;
	virtual SharedPtr<GraphicsVertexBuffer> CreateVertexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) override;
	virtual SharedPtr<GraphicsIndexBuffer> CreateIndexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) override;

};

NAMESPACE_DX12_END