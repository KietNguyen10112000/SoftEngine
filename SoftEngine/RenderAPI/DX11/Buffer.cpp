#include "Buffer.h"

#include "DX11Global.h"
#include "Renderer.h"

#include <Common.h>

VertexBuffer::VertexBuffer(const void* data, int numDataElm, int dataElmSize)
{
	auto size = numDataElm * dataElmSize;
	m_vertexCount = numDataElm;
	m_stride = dataElmSize;

	CD3D11_BUFFER_DESC vDesc(
		size,
		D3D11_BIND_VERTEX_BUFFER
	);

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

void VertexBuffer::Update(const void* data)
{
	DX11Global::renderer->m_d3dDeviceContext->UpdateSubresource(m_buffer, 0, 0, data, 0, 0);
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

void IndexBuffer::Update(const void* data)
{
	DX11Global::renderer->m_d3dDeviceContext->UpdateSubresource(m_buffer, 0, 0, data, 0, 0);
}
