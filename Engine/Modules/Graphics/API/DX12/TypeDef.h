#pragma once
#include "Core/TypeDef.h"

#define NOMINMAX
#include <wrl/client.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <Windows.h>

using Microsoft::WRL::ComPtr;

#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d12")

#ifdef _DEBUG
#define ThrowIfFailed(x) if (FAILED(x)) throw __LINE__;
#else
#define ThrowIfFailed(x) x
#endif // _DEBUG

#define WaitForFence(fence, fenceValue, fenceEvent)                                         \
if (fence->GetCompletedValue() < fenceValue)                                               \
{                                                                                           \
    ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));                     \
    WaitForSingleObject(fenceEvent, INFINITE);                                              \
}


#define NAMESPACE_DX12_BEGIN NAMESPACE_BEGIN namespace dx12 {

#define NAMESPACE_DX12_END } NAMESPACE_END


NAMESPACE_DX12_BEGIN

struct DX12SynchObject
{
    HANDLE			fenceEvent;
    UINT64*         fenceValue;
    ID3D12Fence*    fence;

    inline UINT64 MakeBarrier(ID3D12CommandQueue* commandQueue)
    {
        auto& v = *fenceValue;
        ThrowIfFailed(
            commandQueue->Signal(
                fence,
                ++v
            )
        );
        return v;
    }

    inline void WaitFor(UINT64 value)
    {
        WaitForFence(fence, value, fenceEvent);
    }
};

NAMESPACE_DX12_END