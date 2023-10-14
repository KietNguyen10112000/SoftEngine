#include "DX12Fundamental.h"

#include "DX12Graphics.h"

NAMESPACE_DX12_BEGIN

DX12RenderTarget::~DX12RenderTarget()
{
	if (m_fenceValue)
		DX12Graphics::GetDX12()->ThreadSafeFreeDX12Resource(m_dx12Resource, m_fenceValue);

	if (m_rtv.ptr)
		DX12Graphics::GetDX12()->GetRTVAllocator()->DeallocateCPU(m_rtv, 1);
}

DX12DepthStencilBuffer::~DX12DepthStencilBuffer()
{
	if (m_fenceValue)
		DX12Graphics::GetDX12()->ThreadSafeFreeDX12Resource(m_dx12Resource, m_fenceValue);

	if (m_dsv.ptr)
		DX12Graphics::GetDX12()->GetDSVAllocator()->DeallocateCPU(m_dsv, 1);
}

NAMESPACE_DX12_END