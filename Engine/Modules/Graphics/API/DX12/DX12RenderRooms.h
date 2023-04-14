#pragma once

#include "TypeDef.h"

#include "Core/Memory/Memory.h"

NAMESPACE_DX12_BEGIN

class DX12RenderRooms
{
public:
	constexpr static size_t NUM_SRV_PER_PSO		= 16;
	constexpr static size_t NUM_CBV_PER_PSO		= 16;
	constexpr static size_t NUM_PARAMS_PER_PSO	= NUM_SRV_PER_PSO + NUM_CBV_PER_PSO;

	constexpr static size_t CBV_RANGE_INDEX		= 0;
	constexpr static size_t SRV_RANGE_INDEX		= 1;

	ID3D12Device1*	m_device;
	size_t			m_roomCount;
	UINT64*			m_roomFenceValue;
	size_t			m_curRoomIdx;

	DX12SynchObject*						m_synchObject = nullptr;
	ComPtr<ID3D12DescriptorHeap>			m_gpuDescriptorHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE				m_gpuDescriptorHeapCPUAddr;
	size_t									m_gpuDescriptorHeapCPUAddrSize;

	D3D12_GPU_DESCRIPTOR_HANDLE				m_gpuDescriptorHeapGPUAddr;
	size_t									m_gpuDescriptorHeapGPUAddrSize;

	ComPtr<ID3D12RootSignature>				m_rootSignature;

	inline void Init(size_t numRoom, ID3D12Device1* device, DX12SynchObject* synch)
	{
		m_synchObject = synch;
		m_device = device;
		m_roomCount = numRoom;
		m_curRoomIdx = 0;
		m_roomFenceValue = rheap::NewArray<size_t>(numRoom, 0);

		InitRootSignature();

		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = numRoom * NUM_PARAMS_PER_PSO;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_gpuDescriptorHeap)));

		m_gpuDescriptorHeapCPUAddr = m_gpuDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_gpuDescriptorHeapCPUAddrSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_gpuDescriptorHeapGPUAddr = m_gpuDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_gpuDescriptorHeapGPUAddrSize = m_gpuDescriptorHeapCPUAddrSize;
	}

	inline void InitRootSignature()
	{
		D3D12_DESCRIPTOR_RANGE ranges[2] = {};

		D3D12_DESCRIPTOR_RANGE& cbvRange = ranges[CBV_RANGE_INDEX];
		cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRange.BaseShaderRegister = 0;
		cbvRange.RegisterSpace = 0;
		cbvRange.NumDescriptors = NUM_CBV_PER_PSO;
		cbvRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_DESCRIPTOR_RANGE& srvRange = ranges[SRV_RANGE_INDEX];
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.BaseShaderRegister = 0;
		srvRange.RegisterSpace = 0;
		srvRange.NumDescriptors = NUM_SRV_PER_PSO;
		srvRange.OffsetInDescriptorsFromTableStart = NUM_CBV_PER_PSO;

		D3D12_ROOT_PARAMETER rootParameter = {};
		rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameter.DescriptorTable.pDescriptorRanges = ranges;
		rootParameter.DescriptorTable.NumDescriptorRanges = ARRAYSIZE(ranges);
		rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_STATIC_SAMPLER_DESC rootSampler = {};
		rootSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		rootSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		rootSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		rootSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		rootSampler.MipLODBias = 0;
		rootSampler.MaxAnisotropy = 0;
		rootSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		rootSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		rootSampler.MinLOD = 0.0f;
		rootSampler.MaxLOD = D3D12_FLOAT32_MAX;
		rootSampler.ShaderRegister = 0;
		rootSampler.RegisterSpace = 0;
		rootSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.pParameters = &rootParameter;
		rootSignatureDesc.NumParameters = 1;
		rootSignatureDesc.pStaticSamplers = &rootSampler;
		rootSignatureDesc.NumStaticSamplers = 1;

		ComPtr<ID3DBlob> signature;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr));

		ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSignature)));
	}

	inline void Destroy()
	{
		for (size_t i = 0; i < m_roomCount; i++)
		{
			WaitForFence(m_synchObject->fence, m_roomFenceValue[i], m_synchObject->fenceEvent);
		}
		rheap::Delete(m_roomFenceValue);
	}

	inline void BeginCommandList(ID3D12GraphicsCommandList* cmdList)
	{
		cmdList->SetGraphicsRootSignature(m_rootSignature.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { m_gpuDescriptorHeap.Get() };
		cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	}

	// prepare cbv, srv for rendering pipeline
	// srcParamsTable is addr of DescriptorHeap contains NUM_PARAMS_PER_PSO (cbv + srv) from cpu side
	inline void PrepareARoom(D3D12_CPU_DESCRIPTOR_HANDLE srcParamsTable, size_t numSrcDescriptor, ID3D12GraphicsCommandList* cmdList)
	{
		// wait for current room available
		m_synchObject->WaitFor(m_roomFenceValue[m_curRoomIdx]);

		m_roomFenceValue[m_curRoomIdx] = *(m_synchObject->fenceValue) + 1;

		auto offset = m_curRoomIdx * NUM_PARAMS_PER_PSO * m_gpuDescriptorHeapCPUAddrSize;
		D3D12_CPU_DESCRIPTOR_HANDLE dest = m_gpuDescriptorHeapCPUAddr;
		dest.ptr += offset;

		m_device->CopyDescriptorsSimple(numSrcDescriptor, dest, srcParamsTable, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_curRoomIdx = (m_curRoomIdx + 1) % m_roomCount;

		D3D12_GPU_DESCRIPTOR_HANDLE gpuDest = m_gpuDescriptorHeapGPUAddr;
		gpuDest.ptr += offset;
		cmdList->SetGraphicsRootDescriptorTable(CBV_RANGE_INDEX, gpuDest);

		//gpuDest.ptr += m_gpuDescriptorHeapGPUAddrSize * NUM_CBV_PER_PSO;
		//cmdList->SetGraphicsRootDescriptorTable(SRV_RANGE_INDEX, gpuDest);
	}
};


NAMESPACE_DX12_END