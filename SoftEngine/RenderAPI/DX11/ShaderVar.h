#pragma once
#include "Renderer.h"
#include "DX11Global.h"

#include <Common.h>

class ShaderVar
{
protected:
	friend class DX11Renderer;
	friend class RenderPipeline;

	ID3D11Buffer* m_buffer = nullptr;

public:
	ShaderVar(const void* data, size_t size);
	~ShaderVar();

public:
	inline void Update(const void* data, size_t size);

public:
	inline auto& GetNativeHandle() { return m_buffer; };

};

inline ShaderVar::ShaderVar(const void* data, size_t size)
{

	DX11Renderer* rd = DX11Global::renderer;

	CD3D11_BUFFER_DESC vDesc(
		size,
		D3D11_BIND_CONSTANT_BUFFER
	);

	D3D11_SUBRESOURCE_DATA vData = {};
	vData.pSysMem = data;

	if (FAILED(rd->m_d3dDevice->CreateBuffer(&vDesc, &vData, &m_buffer)))
	{
		Throw(L"CreateBuffer() failed.");
	}
}

inline ShaderVar::~ShaderVar()
{
	m_buffer->Release();
}

inline void ShaderVar::Update(const void* data, size_t size)
{
	DX11Global::renderer->m_d3dDeviceContext->UpdateSubresource(m_buffer, 0, 0, data, 0, 0);
}

class DynamicShaderVar
{
protected:
	friend class DX11Renderer;
	friend class RenderPipeline;

	ID3D11Buffer* m_buffer = nullptr;

public:
	DynamicShaderVar(const void* data, size_t size);
	~DynamicShaderVar();

public:
	inline void Update(const void* data, size_t size);

public:
	inline auto& GetNativeHandle() { return m_buffer; };

};

inline DynamicShaderVar::DynamicShaderVar(const void* data, size_t size)
{

	DX11Renderer* rd = DX11Global::renderer;

	CD3D11_BUFFER_DESC vDesc(
		size,
		D3D11_BIND_CONSTANT_BUFFER
	);

	vDesc.Usage = D3D11_USAGE_DYNAMIC;
	vDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA vData = {};
	vData.pSysMem = data;

	if (FAILED(rd->m_d3dDevice->CreateBuffer(&vDesc, &vData, &m_buffer)))
	{
		Throw(L"CreateBuffer() failed.");
	}
}

inline DynamicShaderVar::~DynamicShaderVar()
{
	m_buffer->Release();
}

inline void DynamicShaderVar::Update(const void* data, size_t size)
{
	auto& context = DX11Global::renderer->m_d3dDeviceContext;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	context->Map(m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	::memcpy(mappedResource.pData, data, size);
	context->Unmap(m_buffer, 0);
}