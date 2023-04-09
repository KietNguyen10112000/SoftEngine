#pragma once
#include "TypeDef.h"

#include "Graphics/Graphics.h"

#include "DX12RingBufferCommandList.h"
#include "DX12GraphicsCommandList.h"

NAMESPACE_DX12_BEGIN

class DX12Graphics : public Graphics
{
public:
	constexpr static size_t NUM_GRAPHICS_COMMAND_LISTS	= 128;
	constexpr static size_t NUM_GRAPHICS_BACK_BUFFERS	= 3;

	constexpr static size_t NUM_SRV_PER_PSO				= 16;
	constexpr static size_t NUM_CBV_PER_PSO				= 16;

	constexpr static size_t GPU_DESCRIPTOR_HEAP_SIZE	= 64*KB;

	ComPtr<ID3D12DescriptorHeap>            m_rtvDescriptorHeap;
	ComPtr<ID3D12Resource>                  m_renderTargets		[NUM_GRAPHICS_BACK_BUFFERS];
	ComPtr<ID3D12DescriptorHeap>            m_dsvDescriptorHeap;
	ComPtr<ID3D12Resource>                  m_depthBuffers		[NUM_GRAPHICS_BACK_BUFFERS];

	size_t									m_currentBackBufferId		= 0;
	size_t                                  m_cpuRTVDescriptorSize		= 0;
	D3D12_CPU_DESCRIPTOR_HANDLE             m_cpuRTVDescriptorHandle	= {};
	size_t                                  m_cpuDSVDescriptorSize		= 0;
	D3D12_CPU_DESCRIPTOR_HANDLE             m_cpuDSVDescriptorHandle	= {};


	ComPtr<ID3D12CommandQueue>				m_commandQueue;

	// ring buffer command list
	DX12RingBufferCommandList<ID3D12GraphicsCommandList, NUM_GRAPHICS_COMMAND_LISTS> m_graphicsCommandList;
	
	// brrow from below m_graphicsCommandList
	HANDLE									m_fenceEvent;
	UINT64*									m_fenceValue;
	ID3D12Fence*							m_fence;

	ComPtr<ID3D12RootSignature>				m_rootSignature;
	ComPtr<ID3D12DescriptorHeap>			m_gpuDescriptorHeap;

	ComPtr<IDXGISwapChain3>                 m_swapChain;
	ComPtr<ID3D12Device2>                   m_device;
	ComPtr<IDXGIFactory4>                   m_dxgiFactory;


	DX12GraphicsCommandList					m_userCmdList;

public:
	DX12Graphics(void* hwnd);
	~DX12Graphics();

private:
	void InitD3D12();
	void InitSwapchain(void* hwnd);
	void InitCommandLists();
	void InitRootSignature();
	void InitDescriptorHeap();
	void SetInitState();

public:
	// Inherited via Graphics
	virtual void BeginCommandList(GraphicsCommandList** cmdList) override;
	virtual void EndCommandList(GraphicsCommandList** cmdList) override;

	virtual void BeginFrame() override;
	virtual void EndFrame() override;

};

NAMESPACE_DX12_END