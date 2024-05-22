#pragma once

#include "TypeDef.h"

#include "DX12Config.h"

NAMESPACE_DX12_BEGIN

class DX12Graphics;

class DX12ImguiImageDescriptorAllocator
{
public:
	size_t m_remainDescriptors = 98;

	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle;

	D3D12_GPU_DESCRIPTOR_HANDLE Allocate(D3D12_CPU_DESCRIPTOR_HANDLE handle);

};

NAMESPACE_DX12_END