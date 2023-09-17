#pragma once

#include "TypeDef.h"
#include "Core/Memory/NewMalloc.h"

NAMESPACE_DX12_BEGIN

class DX12RingBufferCommandList
{
public:
	struct CommandList
	{
		ComPtr<ID3D12CommandAllocator> allocator = nullptr;
		UINT64 waitFenceValue = 0;
	};

	std::vector<CommandList, STDAllocatorMalloc<CommandList>> m_cmdListAllocators;
	ComPtr<ID3D12GraphicsCommandList> m_cmdList = nullptr;

	size_t m_currentAllocatorId = 0;
	size_t m_numAllocators = 0;

public:
	void Resize(ID3D12Device* device, size_t size);

	inline auto* CmdList()
	{
		return m_cmdList.Get();
	}

	inline void NextCmdListAlloc(uint64_t prevWaitValue, ID3D12Fence* fence, HANDLE fenceEvent)
	{
		m_cmdListAllocators[m_currentAllocatorId].waitFenceValue = prevWaitValue;

		m_currentAllocatorId = (m_currentAllocatorId + 1) % m_numAllocators;
		auto& cmdAlloc = m_cmdListAllocators[m_currentAllocatorId];

		if (fence->GetCompletedValue() < cmdAlloc.waitFenceValue)
		{
			ThrowIfFailed(fence->SetEventOnCompletion(cmdAlloc.waitFenceValue, fenceEvent));
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		ThrowIfFailed(cmdAlloc.allocator->Reset());
		ThrowIfFailed(m_cmdList->Reset(cmdAlloc.allocator.Get(), NULL));
	}

};

NAMESPACE_DX12_END