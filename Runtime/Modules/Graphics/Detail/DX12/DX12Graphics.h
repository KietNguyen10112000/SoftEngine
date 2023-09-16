#pragma once

#include "TypeDef.h"
#include "Modules/Graphics/Graphics.h"

NAMESPACE_DX12_BEGIN

class DX12Graphics : public Graphics
{
protected:
	constexpr static size_t NUM_GRAPHICS_BACK_BUFFERS		= 3;
	constexpr static size_t NUM_GRAPHICS_COMMAND_LISTS		= 128;

	ComPtr<IDXGISwapChain3>                 m_swapChain;
	ComPtr<ID3D12Device2>                   m_device;
	ComPtr<IDXGIFactory4>                   m_dxgiFactory;
	ComPtr<ID3D12CommandQueue>				m_commandQueue;

	ComPtr<ID3D12DescriptorHeap>            m_rtvDescriptorHeap;
	ComPtr<ID3D12Resource>                  m_renderTargets[NUM_GRAPHICS_BACK_BUFFERS];
	ComPtr<ID3D12DescriptorHeap>            m_dsvDescriptorHeap;
	ComPtr<ID3D12Resource>                  m_depthBuffers[NUM_GRAPHICS_BACK_BUFFERS];

public:
	DX12Graphics(void* hwnd);
	~DX12Graphics();

private:


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