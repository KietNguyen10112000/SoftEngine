#pragma once

#include "TypeDef.h"

#include "Modules/Graphics/GraphicsFundamental.h"

#include "DX12GraphicsPipeline.h"

NAMESPACE_DX12_BEGIN

class DX12ShaderResource;

class DX12RenderTarget : public GraphicsRenderTarget
{
public:
	friend class DX12Graphics;

	// resource
	DX12Resource m_dx12Resource;

	// view
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtv;

	D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

	UINT64 m_fenceValue = 0;

	~DX12RenderTarget();

	inline auto GetDX12ShaderResource()
	{
		return (DX12ShaderResource*)m_shaderResource.get();
	}

};

class DX12DepthStencilBuffer : public GraphicsDepthStencilBuffer
{
public:
	friend class DX12Graphics;

	// resource
	DX12Resource m_dx12Resource;

	// view
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsv;

	D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;

	UINT64 m_fenceValue = 0;

	~DX12DepthStencilBuffer();

	inline auto GetDX12ShaderResource()
	{
		return (DX12ShaderResource*)m_shaderResource.get();
	}

};

NAMESPACE_DX12_END