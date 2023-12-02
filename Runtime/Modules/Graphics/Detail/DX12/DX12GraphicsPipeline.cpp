#include "DX12GraphicsPipeline.h"

#include "DX12ShaderResource.h"

NAMESPACE_DX12_BEGIN

void DX12GraphicsParams::SetConstantBuffers(soft::GRAPHICS_SHADER_SPACE::SPACE shaderSpace, ID baseSlotIndex, 
	uint32_t numBuffers, soft::SharedPtr<soft::GraphicsConstantBuffer>* constantBuffers)
{
	auto fenceValue = DX12Graphics::GetDX12()->GetCurrentDX12FenceValue();
	auto& paramsSpace = m_params[shaderSpace];

	assert(numBuffers != 0 && numBuffers <= GRAPHICS_PARAMS_DESC::NUM_CONSTANT_BUFFER - paramsSpace.m_constantBufferDescriptorRangesIdx);

	auto dx12 = DX12Graphics::GetDX12();

	uint32_t startIdx = 1;
	
Begin:
	auto dx12buffer = (DX12ConstantBuffer*)constantBuffers[0].get();
	dx12buffer->m_fenceValues[dx12buffer->m_viewIdx] = fenceValue;
	dx12buffer->m_lastFenceValue = fenceValue;
	auto baseCPUHandle = dx12buffer->GetCurrentCBV();

	auto cpuHandleStride = dx12buffer->m_CPUHandleStride;

	auto range = &paramsSpace.m_constantBufferDescriptorRanges[paramsSpace.m_constantBufferDescriptorRangesIdx++];
	range->baseCPUDescriptor = baseCPUHandle;
	range->count = 1;
	range->baseRegisterIndex = (uint32_t)baseSlotIndex;

	bool loop = false;
	for (size_t i = startIdx; i < numBuffers; i++)
	{
		auto offset = i - startIdx;
		dx12buffer = (DX12ConstantBuffer*)constantBuffers[i].get();
		dx12buffer->m_fenceValues[dx12buffer->m_viewIdx] = fenceValue;

		dx12buffer->m_lastFenceValue = fenceValue;

		auto cpuHandle = dx12buffer->GetCurrentCBV();

		if ((cpuHandle.ptr - baseCPUHandle.ptr) / cpuHandleStride != offset)
		{
			// just for test
			assert(0);

			startIdx = i;
			//paramsSpace.m_constantBufferDescriptorRangesIdx++;
			loop = true;
			
			break;
		}

		baseSlotIndex++;
		range->count++;
	}

	if (loop)
	{
		goto Begin;
	}
}

void DX12GraphicsParams::SetShaderResources(soft::GRAPHICS_SHADER_SPACE::SPACE shaderSpace, ID baseSlotIndex,
	uint32_t numResources, soft::SharedPtr<soft::GraphicsShaderResource>* shaderResources)
{
	auto fenceValue = DX12Graphics::GetDX12()->GetCurrentDX12FenceValue();
	auto& paramsSpace = m_params[shaderSpace];

	assert(numResources != 0 && numResources <= GRAPHICS_PARAMS_DESC::NUM_SHADER_RESOURCE - paramsSpace.m_shaderResourceDescriptorRangesIdx);

	auto dx12 = DX12Graphics::GetDX12();

	uint32_t startIdx = 0;

Begin:
	auto dx12SRV = (DX12ShaderResource*)shaderResources[startIdx++].get();
	dx12SRV->m_lastFenceValue = fenceValue;
	auto baseCPUHandle = dx12SRV->m_srv;

	auto range = &paramsSpace.m_shaderResourceDescriptorRanges[paramsSpace.m_shaderResourceDescriptorRangesIdx++];
	range->baseCPUDescriptor = baseCPUHandle;
	range->count = 1;
	range->baseRegisterIndex = (uint32_t)baseSlotIndex;

	bool loop = false;
	for (size_t i = startIdx; i < numResources; i++)
	{
		auto prevDx12SRV = dx12SRV;
		dx12SRV = (DX12ShaderResource*)shaderResources[i].get();
		dx12SRV->m_lastFenceValue = fenceValue;

		if (dx12SRV->m_srvGroupStart.ptr != prevDx12SRV->m_srvGroupStart.ptr)
		{
			// just for test
			assert(0);

			startIdx = i;
			loop = true;

			break;
		}

		baseSlotIndex++;
		range->count++;
	}

	if (loop)
	{
		goto Begin;
	}
}

DX12GraphicsPipeline::DX12GraphicsPipeline(size_t preferRenderCallPerFrame, const GRAPHICS_PIPELINE_DESC& desc)
{
	m_renderRoomParamsCount = preferRenderCallPerFrame;
	m_renderRoomParams = (DX12GraphicsParams*)std::malloc(m_renderRoomParamsCount * sizeof(DX12GraphicsParams));
	for (size_t i = 0; i < m_renderRoomParamsCount; i++)
	{
		auto& room = m_renderRoomParams[i];
		new (&room) DX12GraphicsParams();
		room.m_pipeline = this;
	}
}

DX12GraphicsPipeline::~DX12GraphicsPipeline()
{
	DX12Graphics::GetDX12()->ThreadSafeFreeDX12Resource(m_pipelineState, m_lastFenceValue);

	for (size_t i = 0; i < m_renderRoomParamsCount; i++)
	{
		auto& room = m_renderRoomParams[i];
		room.~DX12GraphicsParams();
	}
	std::free(m_renderRoomParams);
}

soft::GraphicsParams* DX12GraphicsPipeline::PrepareRenderParams()
{
	m_currentRoomIdx = (m_currentRoomIdx + 1) % m_renderRoomParamsCount;
	auto roomParams = &m_renderRoomParams[m_currentRoomIdx];

	for (auto& space : roomParams->m_params)
	{
		space.m_constantBufferDescriptorRangesIdx = 0;
		space.m_shaderResourceDescriptorRangesIdx = 0;
	}
	
	auto dx12 = DX12Graphics::GetDX12();
	dx12->WaitForDX12FenceValue(roomParams->m_fenceValue);
	return roomParams;
}

NAMESPACE_DX12_END