#include "DX12Graphics.h"

NAMESPACE_DX12_BEGIN

DX12Graphics::DX12Graphics(void* hwnd)
{
}


DX12Graphics::~DX12Graphics()
{
}

SharedPtr<GraphicsPipeline> DX12Graphics::CreateRasterizerPipeline(const GRAPHICS_PIPELINE_DESC& desc)
{
	return SharedPtr<GraphicsPipeline>();
}

SharedPtr<GraphicsShaderResource> DX12Graphics::CreateShaderResource(const GRAPHICS_SHADER_RESOURCE_DESC& desc)
{
	return SharedPtr<GraphicsShaderResource>();
}

SharedPtr<GraphicsRenderTarget> DX12Graphics::CreateRenderTarget(const GRAPHICS_RENDER_TARGET_DESC& desc)
{
	return SharedPtr<GraphicsRenderTarget>();
}

SharedPtr<GraphicsPipelineInput> DX12Graphics::CreatePipelineInput(const GRAPHICS_PIPELINE_INPUT_DESC& desc)
{
	return SharedPtr<GraphicsPipelineInput>();
}

SharedPtr<GraphicsVertexBuffer> DX12Graphics::CreateVertexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc)
{
	return SharedPtr<GraphicsVertexBuffer>();
}

SharedPtr<GraphicsIndexBuffer> DX12Graphics::CreateIndexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc)
{
	return SharedPtr<GraphicsIndexBuffer>();
}

NAMESPACE_DX12_END