#include "DX12RenderParams.h"
#include "DX12Graphics.h"

NAMESPACE_DX12_BEGIN

inline UINT64 DX12RenderParams::EndRenderingBatch(DX12Graphics* graphics, ID3D12CommandQueue* commandQueue, ID3D12GraphicsCommandList* cmdList)
{
    m_curBatchIdx = (m_curBatchIdx + 1) % m_numBatches;

    graphics->EndCommandList();
    m_cbBatchFenceValues[m_curBatchIdx] = m_synchObject->MakeBarrier(commandQueue);
    graphics->BeginCommandList();

    auto waitBatch = (m_curBatchIdx + 1) % m_numBatches;

    assert(m_curCBInBatchIdx < 2 * m_numElmPerBatch);
    if (m_curCBInBatchIdx >= m_numElmPerBatch)
    {
        m_curCBInBatchIdx -= m_numElmPerBatch;
    }
    else
    {
        m_curCBInBatchIdx = 0;
    }

    m_synchObject->WaitFor(m_cbBatchFenceValues[waitBatch]);
    return m_cbBatchFenceValues[m_curBatchIdx];
}

NAMESPACE_DX12_END