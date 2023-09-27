#include "DX12Resources.h"

NAMESPACE_DX12_BEGIN

DX12ConstantBuffer::~DX12ConstantBuffer()
{
	std::cout << "~DX12ConstantBuffer()\n";
	std::free(m_fenceValues);
}

void DX12ConstantBuffer::UpdateBuffer(void* buffer, size_t bufferSize)
{
	assert(m_vaddressBytesStride >= bufferSize);

	m_viewIdx = (m_viewIdx + 1) % m_numViews;

	auto dx12 = DX12Graphics::GetDX12();
	auto dest = GetCurrentUploadAddress();

	dx12->WaitForDX12FenceValue(m_fenceValues[m_viewIdx]);

	std::memcpy(dest, buffer, bufferSize);
}

DX12VertexBuffer::~DX12VertexBuffer()
{
	std::cout << "~DX12VertexBuffer()\n";
}

void DX12VertexBuffer::UpdateBuffer(void* buffer, size_t bufferSize)
{
	auto dx12 = DX12Graphics::GetDX12();
	auto uploader = dx12->GetResourceUploader();
	uploader->UploadBuffer(m_resource.resource.Get(), 0, buffer, bufferSize, true);
}

NAMESPACE_DX12_END