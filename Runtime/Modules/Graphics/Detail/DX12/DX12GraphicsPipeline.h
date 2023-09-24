#pragma once

#include "TypeDef.h"

#include "Modules/Graphics/GraphicsFundamental.h"

NAMESPACE_DX12_BEGIN

class DX12GraphicsPipeline;

class DX12GraphicsParams : public GraphicsParams
{
public:
	struct DescriptorRange
	{
		D3D12_CPU_DESCRIPTOR_HANDLE baseCPUDescriptor;
		uint32_t count;
		uint32_t baseRegisterIndex;
	};

	struct ParamsSpace
	{
		DescriptorRange m_constantBufferDescriptorRanges[GRAPHICS_PARAMS_DESC::NUM_CONSTANT_BUFFER];
		uint32_t m_constantBufferDescriptorRangesIdx = 0;

		DescriptorRange m_shaderResourceDescriptorRanges[GRAPHICS_PARAMS_DESC::NUM_SHADER_RESOURCE];
		uint32_t m_shaderResourceDescriptorRangesIdx = 0;
	};

	ParamsSpace m_params[GRAPHICS_SHADER_SPACE::COUNT] = {};

	uint64_t m_fenceValue = 0;

	DX12GraphicsPipeline* m_pipeline = nullptr;

	// Inherited via GraphicsParams
	virtual void SetConstantBuffers(soft::GRAPHICS_SHADER_SPACE::SPACE shaderSpace, ID baseSlotIndex, uint32_t numBuffers, soft::SharedPtr<soft::GraphicsConstantBuffer>* constantBuffers);
	virtual void SetShaderResourcesBuffer(soft::GRAPHICS_SHADER_SPACE::SPACE shaderSpace, ID baseSlotIndex, uint32_t numResources, soft::SharedPtr<soft::GraphicsShaderResource>* shaderResources);
	virtual void SetShaderResourcesTexture2D(soft::GRAPHICS_SHADER_SPACE::SPACE shaderSpace, ID baseSlotIndex, uint32_t numResources, soft::SharedPtr<soft::GraphicsShaderResource>* shaderResources);

	inline void OnRenderCall(uint64_t fenceValue)
	{
		m_fenceValue = fenceValue;
	}
};

class DX12GraphicsPipeline : public GraphicsPipeline
{
public:
	ComPtr<ID3D12PipelineState> m_pipelineState;

	DX12GraphicsParams* m_renderRoomParams;
	size_t	m_renderRoomParamsCount = 0;

	size_t m_currentRoomIdx = 0;

	uint32_t m_constantBufferRangesCount = 0;
	uint32_t m_shaderResourceRangesCount = 0;

	D3D12_PRIMITIVE_TOPOLOGY m_primitiveTopology;

	DX12GraphicsPipeline(size_t preferRenderCallPerFrame, const GRAPHICS_PIPELINE_DESC& desc);
	~DX12GraphicsPipeline();

	// Inherited via GraphicsPipeline
	virtual soft::GraphicsParams* PrepareRenderParams();

	inline auto GetCurrentRenderParamsRoom()
	{
		return &m_renderRoomParams[m_currentRoomIdx];
	}

};

NAMESPACE_DX12_END