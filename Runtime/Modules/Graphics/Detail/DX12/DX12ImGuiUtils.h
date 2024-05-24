#pragma once

#include "TypeDef.h"

#include "DX12Config.h"

NAMESPACE_DX12_BEGIN

class DX12Graphics;

class DX12ImguiImageDescriptorAllocator
{
public:
	constexpr static size_t MAX_PER_FRAMES = 512;

	std::map<ID, ID> m_map;

	size_t m_countDescriptors = MAX_PER_FRAMES - 1;

	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle;

	D3D12_GPU_DESCRIPTOR_HANDLE Allocate(D3D12_CPU_DESCRIPTOR_HANDLE handle);

	inline void NewFrame()
	{
		m_map.clear();
	}

};

NAMESPACE_DX12_END