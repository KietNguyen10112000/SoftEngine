#pragma once

#include "TypeDef.h"
#include "DX12Fundamental.h"

#include "Core/Structures/String.h"

#include "Modules/Graphics/Graphics.h"

#include "DX12DescriptorAllocator.h"
#include "DX12RingBufferCommandList.h"
#include "DX12ResourceUploader.h"

#include "D3D12MemAlloc.h"

#include "DX12Config.h"

NAMESPACE_DX12_BEGIN

class DX12Graphics : public Graphics, public DX12_CONFIG
{
public:
	struct WaitForFreeDX12Resource
	{
		//DX12Resource resource;
		ComPtr<ID3D12Object> dx12Object;
		ComPtr<D3D12MA::Allocation> allocation;
		UINT64 fenceValue;
	};

	ComPtr<IDXGISwapChain3>                 m_swapChain;
	ComPtr<ID3D12Device2>                   m_device;
	ComPtr<IDXGIFactory4>                   m_dxgiFactory;
	ComPtr<IDXGIAdapter>					m_adapter;
	ComPtr<ID3D12CommandQueue>				m_commandQueue;

	// all rtv, dsv, srv will be allocated via these allocators
	DX12DescriptorAllocator m_rtvAllocator;
	DX12DescriptorAllocator m_dsvAllocator;
	DX12DescriptorAllocator m_srvAllocator;

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

	DX12ResourceUploader m_resourceUploader;

	// rasterzier root signature
	ComPtr<ID3D12RootSignature> m_rootSignature;

	ComPtr<ID3D12DescriptorHeap> m_gpuVisibleHeap;

	size_t m_frameCount = 0;

	uint32_t m_CBV_SRV_UAV_CPUdescriptorHandleStride = 0;

	DX12GraphicsPipeline* m_currentGraphicsPipeline = nullptr;
	ComPtr<ID3D12PipelineState> m_currentDX12GraphicsPipeline;

	uint32_t m_renderRoomIdx = 0;

	uint64_t m_renderRoomFenceValues[NUM_RENDER_ROOM] = {};

	D3D12_CPU_DESCRIPTOR_HANDLE m_gpuVisibleHeapCPUHandleStart;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuVisibleHeapGPUHandleStart;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferViews[16] = {};

	String m_compiledShadersPath = "";

	size_t m_renderCallCount = 0;

#ifdef _DEBUG
	DX12RenderTarget* m_currentRTs[8] = {};
	uint32_t m_numCurrentRT = 0;

	DX12DepthStencilBuffer* m_currentDS = nullptr;
#endif // _DEBUG

	uint32_t m_numBoundRTVs = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE m_currentBoundRTVs[8] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE m_currentBoundDSV = {};

	std::vector<WaitForFreeDX12Resource> m_waitForFreeResources;
	spinlock m_waitForFreeResourcesLock;

public:
	DX12Graphics(void* hwnd);
	~DX12Graphics();

	void FirstInit();

private:
	void InitD3D12();
	void InitAllocators();
	void InitSwapchain(void* _hwnd);
	void InitFence();

	void InitRootSignature();
	void InitGPUVisibleDescriptorHeap();
	void StageCurrentRenderParams();

public:
	// Inherited via Graphics
	virtual SharedPtr<GraphicsPipeline> CreateRasterizerPipeline(const GRAPHICS_PIPELINE_DESC& desc) override;

	virtual void CreateShaderResources(
		uint32_t numShaderResources,
		const GRAPHICS_SHADER_RESOURCE_DESC* descs,
		SharedPtr<GraphicsShaderResource>* output
	) override;

	virtual void CreateConstantBuffers(
		uint32_t numConstantBuffers,
		const GRAPHICS_CONSTANT_BUFFER_DESC* descs,
		SharedPtr<GraphicsConstantBuffer>* output
	) override;

	virtual void CreateRenderTargets(
		uint32_t numRenderTargets,
		const GRAPHICS_RENDER_TARGET_DESC* desc,
		SharedPtr<GraphicsRenderTarget>* output
	) override;

	virtual SharedPtr<GraphicsDepthStencilBuffer> CreateDepthStencilBuffer(const GRAPHICS_DEPTH_STENCIL_BUFFER_DESC& desc) override;

	virtual SharedPtr<GraphicsVertexBuffer> CreateVertexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) override;

	virtual SharedPtr<GraphicsIndexBuffer> CreateIndexBuffer(const GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC& desc) override;
	
	virtual void SetGraphicsPipeline(GraphicsPipeline* graphicsPipeline) override;

	virtual void SetDrawParams(GraphicsParams* params) override;

	virtual void SetRenderTargets(uint32_t numRT, GraphicsRenderTarget** rtv, GraphicsDepthStencilBuffer* dsv) override;
	virtual void UnsetRenderTargets(uint32_t numRT, GraphicsRenderTarget** rtv, GraphicsDepthStencilBuffer* dsv) override;

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

	void CreateShaderResourceTexture2D(
		D3D12_CPU_DESCRIPTOR_HANDLE srvGroupStart,
		uint32_t srvGroupCount,
		D3D12_CPU_DESCRIPTOR_HANDLE srv,
		const GRAPHICS_SHADER_RESOURCE_DESC& desc,
		SharedPtr<GraphicsShaderResource>* output
	);

public:
	void ExecuteCurrentCmdList();

	void ThreadSafeFreeDX12Resource(const DX12Resource& resource, UINT64 fenceValue);
	void ThreadSafeFreeDX12Resource(ComPtr<ID3D12Object> resource, UINT64 fenceValue);
	void ProcessFreeDX12ResourceList();

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

	inline auto* GetSRVAllocator()
	{
		return &m_srvAllocator;
	}

	inline uint64_t GetCurrentDX12FenceValue()
	{
		return m_currentFenceValue;
	}

	inline void IncreaseDX12FenceValue()
	{
		m_currentFenceValue++;
	}

	inline void SignalCurrentDX12FenceValue()
	{
		m_commandQueue->Signal(m_fence.Get(), m_currentFenceValue++);
	}

	inline void WaitForDX12FenceValue(uint64_t value)
	{
		assert(value != m_currentFenceValue);

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

	inline auto* GetResourceUploader()
	{
		return &m_resourceUploader;
	}

	inline auto GetFrameCount()
	{
		return m_frameCount;
	}

	inline auto GetCbvSrvUavCPUDescriptorHandleStride()
	{
		return m_CBV_SRV_UAV_CPUdescriptorHandleStride;
	}

	inline auto GetCurrentRenderRoomCPUDescriptorHandleStart()
	{
		auto ret = m_gpuVisibleHeapCPUHandleStart;
		ret.ptr += m_renderRoomIdx * TOTAL_DESCRIPTORS_PER_RENDER_ROOM * GetCbvSrvUavCPUDescriptorHandleStride();
		return ret;
	}

	inline auto GetCurrentRenderRoomGPUDescriptorHandleStart()
	{
		auto ret = m_gpuVisibleHeapGPUHandleStart;
		ret.ptr += m_renderRoomIdx * TOTAL_DESCRIPTORS_PER_RENDER_ROOM * GetCbvSrvUavCPUDescriptorHandleStride();
		return ret;
	}
};

NAMESPACE_DX12_END