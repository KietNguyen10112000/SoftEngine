#pragma once

#include "TypeDef.h"

#include "Modules/Graphics/GraphicsFundamental.h"

#include "DX12GraphicsPipeline.h"

NAMESPACE_DX12_BEGIN

class DX12RenderTarget : public GraphicsRenderTarget
{
public:
	// resource
	DX12Resource m_dx12Resource;

	// view
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtv;

};

class DX12DepthStencilBuffer : public GraphicsDepthStencilBuffer
{
public:
	// resource
	DX12Resource m_dx12Resource;

	// view
	D3D12_CPU_DESCRIPTOR_HANDLE m_dsv;

};

NAMESPACE_DX12_END