#pragma once
#include "TypeDef.h"

//#include <atomic>

NAMESPACE_DX12_BEGIN

template <typename CommandList, size_t N>
class DX12RingBufferCommandList
{
public:
	ComPtr<ID3D12CommandAllocator>          m_commandAllocators		[N] = {};
	UINT64                                  m_fenceValues			[N] = {};
	HANDLE                                  m_fenceEvent			    = {};
    ComPtr<ID3D12Fence>						m_fence 				    = {};
    UINT64                                  m_fenceValue                = 0;
    ComPtr<CommandList>						m_commandList               = {};
	size_t						            m_currentCommandListId		= { 0 };

public:
	inline void Init(ID3D12Device2* device)
	{
        // create command list
        for (size_t i = 0; i < N; i++)
        {
            ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])));
        }
        ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
            m_commandAllocators[0].Get(), NULL, IID_PPV_ARGS(&m_commandList)));
        m_commandList->Close();

        // create fence
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

    inline void Destroy()
    {
        CloseHandle(m_fenceEvent);
    }

    inline void Reset()
    {
        auto id = (m_currentCommandListId++);
        id = id % N;

        if (m_fence->GetCompletedValue() < m_fenceValues[id])
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[id], m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        ThrowIfFailed(m_commandAllocators[id]->Reset());
        ThrowIfFailed(m_commandList->Reset(m_commandAllocators[id].Get(), NULL));
    }

    inline void EndCommandList(ID3D12CommandQueue* commandQueue)
    {
        auto id = m_currentCommandListId;
        id = id % N;

        ThrowIfFailed(
            commandQueue->Signal(
                m_fence.Get(), 
                ++m_fenceValue
            )
        );

        m_fenceValues[id] = m_fenceValue;
    }
};

NAMESPACE_DX12_END