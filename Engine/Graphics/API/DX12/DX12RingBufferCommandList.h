#pragma once
#include "TypeDef.h"

//#include <atomic>

NAMESPACE_DX12_BEGIN

template <typename CommandList, size_t N>
class DX12RingBufferCommandList
{
public:
	ComPtr<ID3D12CommandAllocator>          m_commandAllocators		[N] = {};
	ComPtr<CommandList>						m_commandList			    = {};
	ComPtr<ID3D12Fence>						m_fences				[N] = {};
	UINT64                                  m_fenceValues			[N] = {};
	HANDLE                                  m_fenceEvents			[N] = {};
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
        for (size_t i = 0; i < N; i++)
        {
            ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fences[i])));
            m_fenceValues[i] = 0; // set the initial fence value to 0
            m_fenceEvents[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        }
	}

    inline void Destroy()
    {
        for (size_t i = 0; i < N; i++)
        {
            CloseHandle(m_fenceEvents[i]);
        }
    }

    inline void Reset()
    {
        auto id = (m_currentCommandListId++);
        id = id % N;

        /*if (m_fences[id]->GetCompletedValue() != m_fenceValues[i])
        {
            ThrowIfFailed(m_fences[i]->SetEventOnCompletion(m_fenceValues[id], m_fenceEvents[id]));
            WaitForSingleObject(m_fenceEvents[id], INFINITE);
        }*/

        WaitForFence(m_fences[id], m_fenceValues[id], m_fenceEvents[id]);

        ThrowIfFailed(m_commandAllocators[id]->Reset());
        ThrowIfFailed(m_commandList->Reset(m_commandAllocators[id].Get(), NULL));
    }

    inline void EndCommandList(ID3D12CommandQueue* commandQueue)
    {
        auto id = m_currentCommandListId;
        id = id % N;

        ThrowIfFailed(
            commandQueue->Signal(
                m_fences[id].Get(), 
                m_fenceValues[id]
            )
        );
    }
};

NAMESPACE_DX12_END