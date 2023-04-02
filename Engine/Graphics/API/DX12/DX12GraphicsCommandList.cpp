#include "DX12GraphicsCommandList.h"

NAMESPACE_DX12_BEGIN

DX12GraphicsCommandList::DX12GraphicsCommandList()
{
}

DX12GraphicsCommandList::~DX12GraphicsCommandList()
{
}

void DX12GraphicsCommandList::Init(DX12Graphics* graphics)
{
	m_graphics = graphics;
	m_commandList = graphics->m_graphicsCommandList.m_commandList.Get();
}

void DX12GraphicsCommandList::ClearScreen(const Vec4& color)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Transition.pResource = m_graphics->m_renderTargets[m_graphics->m_currentBackBufferId].Get();
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	m_commandList->ResourceBarrier(1, &barrier);

	auto currentRTV = m_graphics->m_cpuRTVDescriptorHandle;
	currentRTV.ptr += m_graphics->m_currentBackBufferId * m_graphics->m_cpuRTVDescriptorSize;
	auto currentDSV = m_graphics->m_cpuDSVDescriptorHandle;
	currentDSV.ptr += m_graphics->m_currentBackBufferId * m_graphics->m_cpuDSVDescriptorSize;

	m_commandList->ClearRenderTargetView(currentRTV, &color[0], 0, nullptr);
	m_commandList->ClearDepthStencilView(currentDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, 0);
}

NAMESPACE_DX12_END