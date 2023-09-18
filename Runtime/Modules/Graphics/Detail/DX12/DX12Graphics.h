#pragma once

#include "TypeDef.h"
#include "DX12Fundamental.h"

#include "Modules/Graphics/Graphics.h"

#include "DX12DescriptorAllocator.h"
#include "DX12RingBufferCommandList.h"

#include "D3D12MemAlloc.h"

NAMESPACE_DX12_BEGIN

class DX12Graphics : public Graphics
{
protected:
	constexpr static size_t			NUM_GRAPHICS_BACK_BUFFERS					= 3;
	constexpr static size_t			NUM_GRAPHICS_COMMAND_LIST_ALLOCATORS		= 128;
	constexpr static DXGI_FORMAT	BACK_BUFFER_FORMAT							= DXGI_FORMAT_R8G8B8A8_UNORM;

	constexpr static size_t			RTV_ALLOCATOR_NUM_RTV_PER_HEAP				= 256;
	constexpr static size_t			DSV_ALLOCATOR_NUM_DSV_PER_HEAP				= 256;

	ComPtr<IDXGISwapChain3>                 m_swapChain;
	ComPtr<ID3D12Device2>                   m_device;
	ComPtr<IDXGIFactory4>                   m_dxgiFactory;
	ComPtr<IDXGIAdapter>					m_adapter;
	ComPtr<ID3D12CommandQueue>				m_commandQueue;

	// all rtv and dsv will be allocated via these allocators
	DX12DescriptorAllocator m_rtvAllocator;
	DX12DescriptorAllocator m_dsvAllocator;

	// all gpu resource will be allocated via this allocator
	ComPtr<D3D12MA::Allocator> m_dx12ResourceAllocator;

	// back buffer render target and depth buffer
	ComPtr<ID3D12Resource>                  m_renderTargets[NUM_GRAPHICS_BACK_BUFFERS];
	//ComPtr<ID3D12Resource>                  m_depthBuffers[NUM_GRAPHICS_BACK_BUFFERS];
	DX12Resource							m_depthBuffers[NUM_GRAPHICS_BACK_BUFFERS];
	uint64_t								m_frameFenceValues[NUM_GRAPHICS_BACK_BUFFERS] = {};
	D3D12_RECT								m_backBufferScissorRect = {};
	D3D12_VIEWPORT							m_backBufferViewport = {};
	size_t									m_currentBackBufferId = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE				m_rtvStart = {};
	D3D12_CPU_DESCRIPTOR_HANDLE				m_dsvStart = {};

	DX12RenderTarget m_currentRenderTarget;
	DX12DepthStencilBuffer m_currentDepthStencilBuffer;

	// just 1 fence for all
	ComPtr<ID3D12Fence> m_fence;
	HANDLE m_fenceEvent;
	uint64_t m_currentFenceValue;

	DX12RingBufferCommandList m_ringBufferCmdList;

public:
	DX12Graphics(void* hwnd);
	~DX12Graphics();

private:
	void InitD3D12();
	void InitAllocators();
	void InitSwapchain(void* _hwnd);
	void InitFence();

	void ExecuteCurrentCmdList();

public:
	// Inherited via Graphics
	virtual SharedPtr<GraphicsPipeline> CreateRasterizerPipeline(const GRAPHICS_PIPELINE_DESC& desc) override;

	virtual void CreateShaderResources(
		uint32_t numShaderResources,
		const GRAPHICS_SHADER_RESOURCE_DESC* descs,
		SharedPtr<GraphicsShaderResource>* output
	) override;

	virtual SharedPtr<GraphicsRenderTarget> CreateRenderTarget(const GRAPHICS_RENDER_TARGET_DESC& desc) override;

	virtual SharedPtr<GraphicsVertexBuffer> CreateVertexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) override;

	virtual SharedPtr<GraphicsIndexBuffer> CreateIndexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) override;
	
	virtual void SetGraphicsPipeline(GraphicsPipeline* graphicsPipeline) override;

	virtual void SetDrawParams(GraphicsParams* params) override;

	virtual void SetRenderTarget(uint32_t numRT, GraphicsRenderTarget** rtv, GraphicsDepthStencilBuffer* dsv) override;

	virtual void DrawInstanced(uint32_t numVertexBuffers, GraphicsVertexBuffer** vertexBuffers, uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) override;

	virtual void DrawIndexedInstanced(GraphicsIndexBuffer* indexBuffer, uint32_t indexCountPerInstance, uint32_t numVertexBuffers, GraphicsVertexBuffer** vertexBuffers, uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation) override;

	virtual void ClearRenderTarget(GraphicsRenderTarget* rtv, Vec4 clearColor, uint32_t numRect, const AARect* rects) override;

	virtual void ClearDepthStencil(GraphicsDepthStencilBuffer* dsv, uint32_t numRect, const AARect* rects) override;

	virtual GraphicsRenderTarget* GetScreenRenderTarget() override;

	virtual GraphicsDepthStencilBuffer* GetScreenDepthStencilBuffer() override;

	virtual uint64_t GetCurrentFenceValue() override;

	virtual void WaitForFenceValue(uint64_t value) override;

	virtual void BeginFrame() override;

	virtual void EndFrame(bool vsync) override;

public:
	void AllocateDX12Resource(
		const D3D12_RESOURCE_DESC* desc,
		D3D12_HEAP_TYPE heapType,
		D3D12_RESOURCE_STATES resourceState,
		const D3D12_CLEAR_VALUE* clearValue,
		DX12Resource* output
	);

public:
	inline static DX12Graphics* GetDX12()
	{
		return (DX12Graphics*)Get();
	}

	inline auto* GetRTVAllocator()
	{
		return &m_rtvAllocator;
	}

	inline auto* GetDSVAllocator()
	{
		return &m_dsvAllocator;
	}

	inline uint64_t GetCurrentDX12FenceValue()
	{
		return m_currentFenceValue;
	}

	inline void IncreaseDX12FenceValue()
	{
		m_currentFenceValue++;
	}

	inline void WaitForDX12FenceValue(uint64_t value)
	{
		if (m_fence->GetCompletedValue() < value)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(value, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
	}

	inline auto* GetCmdList()
	{
		return m_ringBufferCmdList.CmdList();
	}

};

NAMESPACE_DX12_END