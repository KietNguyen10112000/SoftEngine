#pragma once

#include "TypeDef.h"

#include "Core/Memory/NewMalloc.h"
#include "Core/Structures/STD/STDContainers.h"

#include "Core/Memory/Page.h"

NAMESPACE_DX12_BEGIN

class DX12DescriptorAllocator
{
private:
	constexpr static size_t HANDLE_CONTROL_BLOCK_SIZE = 64;

	struct DescriptorHeap
	{
		ComPtr<ID3D12DescriptorHeap> dx12Heap;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuHandleStart;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuHandleStart;

		// using avl-based memory allocation to control which handles-segment is free, this will use 64 bytes to control 1 handle, not too bad!
		Page page = { true };
	};

	using DescriptorHeapList = std::vector<DescriptorHeap, STDAllocatorMalloc<DescriptorHeap>>;

	DescriptorHeapList m_descriptorHeaps;
	uint32_t m_cpuHandleSize;
	uint32_t m_gpuHandleSize = 0;

	D3D12_DESCRIPTOR_HEAP_DESC m_desc;
	ID3D12Device2* m_device;

public:
	DX12DescriptorAllocator() {};

	void Initialize(ID3D12Device2* device, const D3D12_DESCRIPTOR_HEAP_DESC& primalDesc)
	{
		assert(primalDesc.NumDescriptors >= 256);

		m_desc = primalDesc;
		m_device = device;
		AllocateNewHeap();

		m_cpuHandleSize = m_device->GetDescriptorHandleIncrementSize(m_desc.Type);
		m_gpuHandleSize = m_device->GetDescriptorHandleIncrementSize(m_desc.Type);
	}

private:
	inline void AllocateNewHeap()
	{
		auto& back = m_descriptorHeaps.emplace_back();
		ThrowIfFailed(m_device->CreateDescriptorHeap(&m_desc, IID_PPV_ARGS(&back.dx12Heap)));
		back.cpuHandleStart = back.dx12Heap->GetCPUDescriptorHandleForHeapStart();	

		if (m_desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
		{
			back.gpuHandleStart = back.dx12Heap->GetGPUDescriptorHandleForHeapStart();
		}

		new (&back.page) Page(HANDLE_CONTROL_BLOCK_SIZE * m_desc.NumDescriptors);
	}

public:
	inline uint32_t GetCPUStride()
	{
		return m_cpuHandleSize;
	}

	inline uint32_t GetGPUStride()
	{
		return m_gpuHandleSize;
	}

	inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateCPU(uint32_t count)
	{
		auto controlSize = count * HANDLE_CONTROL_BLOCK_SIZE - sizeof(AllocatedBlock);
		D3D12_CPU_DESCRIPTOR_HANDLE ret = {};

	Begin:
		for (auto& heap : m_descriptorHeaps)
		{
			ret = heap.cpuHandleStart;

			auto mem = heap.page.Allocate(controlSize);
			if (mem)
			{
				size_t offset = (byte*)mem - heap.page.GetBuffer() - sizeof(AllocatedBlock);
				assert(offset % HANDLE_CONTROL_BLOCK_SIZE == 0);

				ret.ptr += (offset / HANDLE_CONTROL_BLOCK_SIZE) * m_cpuHandleSize;
				return ret;
			}
		}

		AllocateNewHeap();
		goto Begin;
	}

	inline void DeallocateCPU(D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t _count)
	{
		for (auto& heap : m_descriptorHeaps)
		{
			auto heapOffset = handle.ptr - heap.cpuHandleStart.ptr;
			auto count = heapOffset / m_cpuHandleSize;

			if (count <= m_desc.NumDescriptors)
			{
				//assert(count == _count);
				
				auto pageOffset = count * HANDLE_CONTROL_BLOCK_SIZE;
				heap.page.Free(heap.page.GetBuffer() + pageOffset + sizeof(AllocatedBlock));
				return;
			}
		}

		// unreachable
		assert(0);
	}

	/*inline D3D12_GPU_DESCRIPTOR_HANDLE AllocateGPU(uint32_t count)
	{

	}*/

};

NAMESPACE_DX12_END