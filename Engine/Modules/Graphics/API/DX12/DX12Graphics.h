#pragma once
#include "TypeDef.h"

#include "Graphics/Graphics.h"

#include "DX12RingBufferCommandList.h"
#include "DX12GraphicsCommandList.h"
#include "DX12RenderRooms.h"

NAMESPACE_DX12_BEGIN

class DX12Graphics : public Graphics
{
public:
	constexpr static size_t NUM_GRAPHICS_COMMAND_LISTS	= 128;
	constexpr static size_t NUM_GRAPHICS_BACK_BUFFERS	= 3;

	//constexpr static size_t GPU_DESCRIPTOR_HEAP_SIZE	= 64*KB;

	constexpr static size_t NUM_RENDER_ROOMS			= 2*KB;

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

	ID3D12GraphicsCommandList*				m_commandList;
	
	// brrow from below m_graphicsCommandList
	DX12SynchObject							m_synchObject;


	D3D12_RECT								m_backBufferScissorRect	= {};
	D3D12_VIEWPORT							m_backBufferViewport	= {};
	DX12RenderRooms							m_renderRooms;

	
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
	void InitRenderRooms();

public:
	// Inherited via Graphics
	virtual void BeginFrame(GraphicsCommandList** cmdList) override;
	virtual void EndFrame(GraphicsCommandList** cmdList) override;

public:
	void BeginCommandList();
	void EndCommandList();

};

NAMESPACE_DX12_END