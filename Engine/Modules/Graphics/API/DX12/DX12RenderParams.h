#pragma once

#include "TypeDef.h"
#include "DX12RenderRooms.h"

#include "Core/Memory/MemoryUtils.h"
#include "Core/Memory/Memory.h"

NAMESPACE_DX12_BEGIN

class DX12Graphics;

class DX12RenderParams
{
public:
	DX12SynchObject*                m_synchObject	= nullptr;

	// write to m_uploadHeap
	ComPtr<ID3D12Resource>			m_uploadConstBuf;
	// and when render, flush m_uploadHeap to m_gpuHeap
	ComPtr<ID3D12Resource>			m_gpuConstBuf;

    ComPtr<ID3D12DescriptorHeap>    m_descriptorHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE     m_cpuHandle;
    size_t                          m_descriptorHandleSize;

    byte*                           m_constantBuffer        = 0;
    size_t                          m_curCBInBatchIdx       = 0;
    size_t                          m_curBatchIdx           = 0;
    size_t                          m_cbStride              = 0;
    size_t                          m_numElmPerBatch        = 0;
    size_t                          m_numBatches            = 0;
    UINT64*                         m_cbBatchFenceValues    = nullptr;
    size_t                          m_cbAccessIdx[DX12RenderRooms::NUM_CBV_PER_PSO] = {};
    size_t                          m_numDescriptorsPerRoom = 0;
    size_t                          m_numCBVs               = 0;
    size_t                          m_numSRVs               = 0;


    // size of constant buffer must 256 bytes alignment
	inline void Init(
		ID3D12Device1* device,
		size_t numElmPerBatch, size_t numBatches,
        size_t numBuiltInCBV,
		size_t numCBV, size_t* constBufSizes,
        size_t numBuiltInSRV,
		size_t numSRV, void** shaderResourceGPUAddresses,
		DX12SynchObject* synch
	) {
		m_synchObject = synch;
        m_cbBatchFenceValues = rheap::NewArray<UINT64>(numBatches);
        ::memset(m_cbBatchFenceValues, 0, sizeof(UINT64) * numBatches);

        m_numCBVs = numCBV;
        m_numSRVs = numSRV;

        assert(numCBV <= DX12RenderRooms::NUM_CBV_PER_PSO);
        m_numDescriptorsPerRoom = numCBV + numSRV;

        size_t cbSize = 0;

        for (size_t i = 0; i < numBuiltInCBV; i++)
        {
            m_cbAccessIdx[i] = -1;
        }

        for (size_t i = 0; i < numCBV; i++)
        {
            m_cbAccessIdx[i + numBuiltInCBV] = cbSize;

            // must be 256 bytes alignment
            assert(MemoryUtils::Align<256>(constBufSizes[i]) == constBufSizes[i]);
            cbSize += constBufSizes[i];
        }

        m_cbStride          = cbSize;
        m_numBatches        = numBatches;
        m_numElmPerBatch    = numElmPerBatch;

        InitResource(device, cbSize * numElmPerBatch * numBatches, 
            numElmPerBatch * numBatches * m_numDescriptorsPerRoom);
        InitCBVDescriptorHeap(device, numElmPerBatch, numBatches, numBuiltInCBV, 
            numCBV, constBufSizes);

        if (numSRV || numBuiltInSRV)
        {
            // not support yet
            assert(0);
        }
	}

    inline void Destroy()
    {
        for (size_t i = 0; i < m_numBatches; i++)
        {
            WaitForFence(m_synchObject->fence, m_cbBatchFenceValues[i], m_synchObject->fenceEvent);
        }
        rheap::Delete(m_cbBatchFenceValues);
    }

	inline void InitResource(ID3D12Device1* device, const UINT constantBufferSize, size_t numDescriptor)
	{
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC cbufDesc = {};
        cbufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        cbufDesc.Width = constantBufferSize;
        cbufDesc.Height = 1;
        cbufDesc.DepthOrArraySize = 1;
        cbufDesc.MipLevels = 1;
        cbufDesc.SampleDesc.Count = 1;
        cbufDesc.SampleDesc.Quality = 0;
        cbufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &cbufDesc,
                D3D12_RESOURCE_STATE_COPY_SOURCE,
                nullptr,
                IID_PPV_ARGS(&m_uploadConstBuf)
            )
        );

        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &cbufDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&m_gpuConstBuf)
            )
        );

        D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
        descriptorHeapDesc.NumDescriptors = numDescriptor;
        descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_descriptorHeap)));

        m_cpuHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();

        ThrowIfFailed(m_uploadConstBuf->Map(0, 0, (void**)&m_constantBuffer));
	}

    inline void InitCBVDescriptorHeap(ID3D12Device1* device, 
        size_t numElmPerBatch, size_t numBatches,
        size_t numBuiltIn,
        size_t numCBV, size_t* constBufSizes)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};

        auto cpuAddr = m_cpuHandle;
        auto gpuStartAddr = m_gpuConstBuf->GetGPUVirtualAddress();
        auto gpuAddr = gpuStartAddr;

        m_descriptorHandleSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        auto cpuDescriptorSize = m_descriptorHandleSize;
        auto total = numElmPerBatch * numBatches;

        D3D12_CPU_DESCRIPTOR_HANDLE builtInHandle = {};

        for (size_t j = 0; j < total; j++)
        {
            // set n descriptors first as built in constant buffer
            /*for (size_t i = 0; i < numBuiltIn; i++)
            {
                builtInHandle = builtInDescriptors[i];
                device->CopyDescriptorsSimple(1, cpuAddr, builtInHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                cpuAddr.ptr += cpuDescriptorSize;
            }*/

            for (size_t i = 0; i < numCBV; i++)
            {
                cbvDesc.BufferLocation = gpuAddr;
                cbvDesc.SizeInBytes = constBufSizes[i];
                device->CreateConstantBufferView(&cbvDesc, cpuAddr);

                gpuAddr += constBufSizes[i];
                cpuAddr.ptr += cpuDescriptorSize;
            }

            cpuAddr.ptr += cpuDescriptorSize * m_numSRVs;
        }

    }

    inline byte* AllocateConstantBufferWriteHead()
    {
        auto ret = m_curCBInBatchIdx;
        m_curCBInBatchIdx++;

        assert(m_curCBInBatchIdx < 2 * m_numElmPerBatch);

        return &m_constantBuffer[(m_curBatchIdx * m_numElmPerBatch + ret) * m_cbStride];
    }


    // get constant buffer shader register at index
    // auto head = AllocateConstantBufferWriteHead()
    // auto buf = GetConstantBuffer(head, 0)
    // built-in constant buffer cannot be modified,
    inline byte* GetConstantBuffer(byte* writeHead, size_t index)
    {
        if (m_cbAccessIdx[index] == -1) return nullptr;
        return &writeHead[m_cbAccessIdx[index]];
    }

    inline bool IsNeedFlush()
    {
        return m_curCBInBatchIdx >= m_numElmPerBatch;
    }

    inline bool IsRemainBatch()
    {
        return m_curCBInBatchIdx != 0;
    }

    // flush data to render
    template <typename FUNC>
    inline UINT64 RenderBatch(
        DX12Graphics* graphics,
        ID3D12Device1* device, 
        ID3D12CommandQueue* commandQueue, 
        ID3D12GraphicsCommandList* cmdList, 
        DX12RenderRooms* rooms,
        size_t numBuiltInCBV, D3D12_CPU_DESCRIPTOR_HANDLE* srcBuiltInCBVs,
        size_t numBuiltInSRV, D3D12_CPU_DESCRIPTOR_HANDLE* srcBuiltInSRVs,
        FUNC drawcall)
    {
        auto offset = m_curBatchIdx * m_numElmPerBatch * m_cbStride;
        cmdList->CopyBufferRegion(m_gpuConstBuf.Get(), offset, m_uploadConstBuf.Get(),
            offset, m_numElmPerBatch * m_cbStride);

        rooms->BeginCommandList(cmdList);

        auto cpuTable = m_cpuHandle;
        auto cpuDescriptorSize = m_descriptorHandleSize;

        cpuTable.ptr += cpuDescriptorSize * m_curBatchIdx * m_numElmPerBatch * m_numDescriptorsPerRoom;

        D3D12_CPU_DESCRIPTOR_HANDLE cpuSRVTable = cpuTable;
        cpuSRVTable.ptr += cpuDescriptorSize * m_numCBVs;

        auto num = std::min(m_curCBInBatchIdx, m_numElmPerBatch);
        for (size_t i = 0; i < num; i++)
        {
            rooms->PrepareARoom(numBuiltInCBV, srcBuiltInCBVs, m_numCBVs, cpuTable, 
                numBuiltInSRV, srcBuiltInSRVs, m_numSRVs, cpuSRVTable, cmdList);
            drawcall();
            cpuTable.ptr += cpuDescriptorSize * DX12RenderRooms::NUM_PARAMS_PER_PSO;
        }

        return EndRenderingBatch(graphics, commandQueue, cmdList);
    }

    UINT64 EndRenderingBatch(DX12Graphics* graphics, 
        ID3D12CommandQueue* commandQueue, ID3D12GraphicsCommandList* cmdList);


    // for using params as builtin params
    inline void BeginRenderingAsBuiltInParam(ID3D12GraphicsCommandList* cmdList, 
        D3D12_CPU_DESCRIPTOR_HANDLE* outputCBVs, D3D12_CPU_DESCRIPTOR_HANDLE* outputSRVs)
    {
        auto offset = m_curBatchIdx * m_numElmPerBatch * m_cbStride;
        cmdList->CopyBufferRegion(m_gpuConstBuf.Get(), offset, m_uploadConstBuf.Get(),
            offset, m_numElmPerBatch * m_cbStride);

        auto cpuTable = m_cpuHandle;
        auto cpuDescriptorSize = m_descriptorHandleSize;

        cpuTable.ptr += cpuDescriptorSize * m_curBatchIdx * m_numElmPerBatch * m_numDescriptorsPerRoom;

        for (size_t i = 0; i < m_numCBVs; i++)
        {
            outputCBVs[i] = cpuTable;
            cpuTable.ptr += cpuDescriptorSize;
        }

        for (size_t i = 0; i < m_numSRVs; i++)
        {
            outputSRVs[i] = cpuTable;
            cpuTable.ptr += cpuDescriptorSize;
        }
    }

    inline void EndRenderingAsBuiltInParam(ID3D12CommandQueue* commandQueue)
    {
        m_curBatchIdx = (m_curBatchIdx + 1) % m_numBatches;

        m_cbBatchFenceValues[m_curBatchIdx] = m_synchObject->MakeBarrier(commandQueue);

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
    }

};

NAMESPACE_DX12_END