#include "DX12RingBufferCommandList.h"

#include "DX12Graphics.h"

NAMESPACE_DX12_BEGIN

void DX12RingBufferCommandList::Resize(ID3D12Device* device, size_t size)
{
	auto dx12 = DX12Graphics::GetDX12();
	for (auto& cmdAlloc : m_cmdListAllocators)
	{
		dx12->WaitForDX12FenceValue(cmdAlloc.waitFenceValue);
	}

	m_cmdListAllocators.resize(size);

	for (auto& cmdAlloc : m_cmdListAllocators)
	{
		cmdAlloc.waitFenceValue = 0;
		if (cmdAlloc.allocator.Get())
		{
			continue;
		}

		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdAlloc.allocator)));
	}

	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_cmdListAllocators[0].allocator.Get(), NULL, IID_PPV_ARGS(&m_cmdList)));

	m_numAllocators = size;
}

NAMESPACE_DX12_END