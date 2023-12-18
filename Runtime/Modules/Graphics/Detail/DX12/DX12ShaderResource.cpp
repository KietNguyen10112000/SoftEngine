#include "DX12ShaderResource.h"

NAMESPACE_DX12_BEGIN

DX12ShaderResource::~DX12ShaderResource()
{
	if (m_lastFenceValue)
		DX12Graphics::GetDX12()->ThreadSafeFreeDX12Resource(m_resource, m_lastFenceValue);

	if (m_srvGroupStart.ptr == m_srv.ptr)
	{
		DX12Graphics::GetDX12()->GetSRVAllocator()->DeallocateCPU(m_srvGroupStart, m_srvGroupCount);
	}
}

void DX12ShaderResource::UpdateBuffer(const void* buffer, size_t bufferSize)
{
}

void DX12ShaderResource::UpdateTexture2D(const void* buffer, size_t bufferSize, const TEXTURE2D_REGION& region, bool endChain)
{
	auto uploader = DX12Graphics::GetDX12()->GetResourceUploader();
	m_lastFenceValue = DX12Graphics::GetDX12()->GetCurrentDX12FenceValue();
	uploader->UploadTexture2D(m_resource.resource.Get(), region.x, region.y, buffer, region.width, region.height, region.pixelStride, region.mipLevel, endChain);
}

void DX12ShaderResource::GetDesc(GRAPHICS_SHADER_RESOURCE_DESC* output)
{
	*output = m_desc;
}

NAMESPACE_DX12_END