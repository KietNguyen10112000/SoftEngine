#include "DX12ImGuiUtils.h"

#include "DX12Graphics.h"

NAMESPACE_DX12_BEGIN

D3D12_GPU_DESCRIPTOR_HANDLE DX12ImguiImageDescriptorAllocator::Allocate(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	//DX12_CONFIG::TOTAL_DESCRIPTORS_PER_RENDER_ROOM

	auto it = m_map.find(ID(handle.ptr));
	if (it != m_map.end())
	{
		return (D3D12_GPU_DESCRIPTOR_HANDLE&)(it->second);
	}

	auto graphics = DX12Graphics::GetDX12();
	auto device = graphics->m_device.Get();
	auto stride = graphics->GetCbvSrvUavCPUDescriptorHandleStride();

	if (m_countDescriptors == MAX_PER_FRAMES - 1)
	{
		//graphics->WaitForNextRenderRoom();
		//m_remainDescriptors = DX12_CONFIG::TOTAL_DESCRIPTORS_PER_RENDER_ROOM;

		m_cpuHandle = graphics->m_ImGuiSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
		m_gpuHandle = graphics->m_ImGuiSrvDescHeap->GetGPUDescriptorHandleForHeapStart();

		m_cpuHandle.ptr += stride;
		m_gpuHandle.ptr += stride;

		m_countDescriptors = 0;
	}

	auto cpuHandle = m_cpuHandle;
	cpuHandle.ptr += stride * m_countDescriptors;
	device->CopyDescriptorsSimple(1, m_cpuHandle, handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	auto ret = m_gpuHandle;
	cpuHandle.ptr += stride * m_countDescriptors;

	m_countDescriptors++;

	m_map.insert({ handle.ptr,ret.ptr });

	return ret;
}

NAMESPACE_DX12_END