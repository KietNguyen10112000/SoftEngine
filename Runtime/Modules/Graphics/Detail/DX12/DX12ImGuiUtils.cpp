#include "DX12ImGuiUtils.h"

#include "DX12Graphics.h"

NAMESPACE_DX12_BEGIN

D3D12_GPU_DESCRIPTOR_HANDLE DX12ImguiImageDescriptorAllocator::Allocate(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	//DX12_CONFIG::TOTAL_DESCRIPTORS_PER_RENDER_ROOM

	auto graphics = DX12Graphics::GetDX12();
	auto device = graphics->m_device.Get();
	auto stride = graphics->GetCbvSrvUavCPUDescriptorHandleStride();

	if (m_remainDescriptors == 98)
	{
		//graphics->WaitForNextRenderRoom();
		//m_remainDescriptors = DX12_CONFIG::TOTAL_DESCRIPTORS_PER_RENDER_ROOM;

		m_cpuHandle = graphics->m_ImGuiSrvDescHeap->GetCPUDescriptorHandleForHeapStart();
		m_gpuHandle = graphics->m_ImGuiSrvDescHeap->GetGPUDescriptorHandleForHeapStart();

		m_cpuHandle.ptr += stride;
		m_gpuHandle.ptr += stride;

		m_remainDescriptors = 0;
	}

	//m_remainDescriptors--;

	auto cpuHandle = m_cpuHandle;
	cpuHandle.ptr += stride * m_remainDescriptors;
	device->CopyDescriptorsSimple(1, m_cpuHandle, handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	auto ret = m_gpuHandle;
	cpuHandle.ptr += stride * m_remainDescriptors;

	m_remainDescriptors++;

	return ret;
}

NAMESPACE_DX12_END