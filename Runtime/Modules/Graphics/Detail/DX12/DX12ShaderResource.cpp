#include "DX12ShaderResource.h"

NAMESPACE_DX12_BEGIN

DX12ShaderResource::~DX12ShaderResource()
{
	if (m_srvGroupStart.ptr == m_srv.ptr)
	{
		DX12Graphics::GetDX12()->GetSRVAllocator()->DeallocateCPU(m_srvGroupStart, m_srvGroupCount);
	}
}

void DX12ShaderResource::UpdateBuffer(void* buffer, size_t bufferSize)
{
}

void DX12ShaderResource::UpdateTexture2D(void* buffer, size_t bufferSize, const TEXTURE2D_REGION& region, bool endChain)
{
	auto uploader = DX12Graphics::GetDX12()->GetResourceUploader();
	uploader->UploadTexture2D(m_resource.resource.Get(), region.x, region.y, buffer, region.width, region.height, region.pixelStride, region.mipLevel, endChain);
}

NAMESPACE_DX12_END