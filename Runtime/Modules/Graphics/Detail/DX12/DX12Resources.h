#pragma once

#include "TypeDef.h"

#include "Modules/Graphics/GraphicsFundamental.h"

#include "DX12DescriptorAllocator.h"

NAMESPACE_DX12_BEGIN

class DX12ConstantBuffer : public GraphicsConstantBuffer
{
public:
	ComPtr<ID3D12Resource>					m_resource;
	D3D12_GPU_VIRTUAL_ADDRESS				m_baseGPUAddress;

	ComPtr<ID3D12DescriptorHeap>			m_descriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE				m_baseCPUHandle = { 0 };
	uint32_t								m_CPUHandleStride;

	byte*									m_vaddress;
	uint32_t								m_vaddressBytesStride;

	uint32_t								m_numViews;
	uint32_t								m_viewIdx = 0;

	uint64_t*								m_fenceValues = nullptr;

	~DX12ConstantBuffer();

	// Inherited via GraphicsConstantBuffer
	virtual void UpdateBuffer(void* buffer, size_t bufferSize);

	inline byte* GetCurrentUploadAddress()
	{
		return m_vaddress + m_vaddressBytesStride * m_viewIdx;
	}

	inline D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentCBV()
	{
		auto ret = m_baseCPUHandle;
		ret.ptr += m_CPUHandleStride * m_viewIdx;
		return ret;
	}
};

class DX12VertexBuffer : public GraphicsVertexBuffer
{
public:
	D3D12_VERTEX_BUFFER_VIEW m_view;

	DX12Resource m_resource;

	~DX12VertexBuffer();

	virtual void UpdateBuffer(void* buffer, size_t bufferSize) override;

};

NAMESPACE_DX12_END