#pragma once

#include "TypeDef.h"

#include "Modules/Graphics/GraphicsFundamental.h"

#include "DX12Fundamental.h"

NAMESPACE_DX12_BEGIN

class DX12ShaderResource : public GraphicsShaderResource
{
public:
	DX12Resource					m_resource;
	GRAPHICS_SHADER_RESOURCE_DESC	m_desc;

	D3D12_CPU_DESCRIPTOR_HANDLE		m_srvGroupStart;
	uint32_t						m_srvGroupCount;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_srv;

	UINT64							m_lastFenceValue = 0;

	~DX12ShaderResource();

	// Inherited via GraphicsShaderResource
	virtual void UpdateBuffer(const void* buffer, size_t bufferSize, const GRAPHICS_BUFFER_REGION& region, bool endUpdateChain) override;

	virtual void UpdateTexture2D(const void* buffer, size_t bufferSize, const TEXTURE2D_REGION& region, bool endChain) override;

	virtual void GetDesc(GRAPHICS_SHADER_RESOURCE_DESC* output) override;
};

NAMESPACE_DX12_END