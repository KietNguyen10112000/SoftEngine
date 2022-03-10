#include "Buffer.h"

#include "DX11Global.h"
#include "Renderer.h"

#include <Common.h>

VertexBuffer::VertexBuffer(const void* data, int numDataElm, int dataElmSize, size_t flag)
{
	auto size = numDataElm * dataElmSize;
	m_vertexCount = numDataElm;
	m_stride = dataElmSize;

	CD3D11_BUFFER_DESC vDesc(
		size,
		D3D11_BIND_VERTEX_BUFFER
	);

	if (flag == DYNAMIC)
	{
		vDesc.Usage = D3D11_USAGE_DYNAMIC;
		vDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}

	D3D11_SUBRESOURCE_DATA vData = {};
	vData.pSysMem = data;

	auto device = DX11Global::renderer->m_d3dDevice;

	if (FAILED(device->CreateBuffer(&vDesc, &vData, &m_buffer)))
	{
		Throw(L"CreateBuffer() failed.");
	}
}

VertexBuffer::~VertexBuffer()
{
	m_buffer->Release();
}

void VertexBuffer::Update(const void* data, size_t size)
{
	auto ctx = DX11Global::renderer->m_d3dDeviceContext;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ctx->Map(m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

	::memcpy(mappedResource.pData, data, size);

	ctx->Unmap(m_buffer, 0);
}

IndexBuffer::IndexBuffer(const void* data, int numDataElm, int dataElmSize)
{
	m_indexCount = numDataElm;
	auto size = numDataElm * dataElmSize;

	m_format = DXGI_FORMAT_R16_UINT;

	if (dataElmSize == 4)
	{
		m_format = DXGI_FORMAT_R32_UINT;
	}

	CD3D11_BUFFER_DESC vDesc(
		size,
		D3D11_BIND_INDEX_BUFFER
	);

	D3D11_SUBRESOURCE_DATA vData = {};
	vData.pSysMem = data;

	auto device = DX11Global::renderer->m_d3dDevice;

	if (FAILED(device->CreateBuffer(&vDesc, &vData, &m_buffer)))
	{
		Throw(L"CreateBuffer() failed.");
	}
}

IndexBuffer::~IndexBuffer()
{
	m_buffer->Release();
}

void IndexBuffer::Update(const void* data, size_t size)
{
	DX11Global::renderer->m_d3dDeviceContext->UpdateSubresource(m_buffer, 0, 0, data, 0, 0);
}
