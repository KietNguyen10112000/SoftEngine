#pragma once

#include "Core/TypeDef.h"
#include "Core/Memory/SmartPointers.h"
#include "Math/Math.h"

NAMESPACE_BEGIN

struct GRAPHICS_VIEWPORT
{
	Vec2 topLeft;

	// width, height
	Vec2 size;
};

struct TEXTURE2D_REGION
{
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t mipLevel = 0;
	uint32_t pixelStride = 0;
};

struct GRAPHICS_DATA_FORMAT
{
	enum FORMAT
	{
		FORMAT_R32_FLOAT,
		FORMAT_R32G32_FLOAT,
		FORMAT_R32G32B32_FLOAT,
		FORMAT_R32G32B32A32_FLOAT,

		FORMAT_R8_UNORM,
		FORMAT_R8G8B8_UNORM,
		FORMAT_R8G8B8A8_UNORM,

		COUNT,
		NONE
	};
};

struct GRAPHICS_CONSTANT_BUFFER_DESC
{
	size_t perferNumRoom = -1;
	size_t bufferSize;
};

struct GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC
{
	uint32_t stride;
	uint32_t count;
	//GRAPHICS_DATA_FORMAT::FORMAT format;
};

struct GRAPHICS_SHADER_RESOURCE_TYPE_TEXTURE2D_DESC
{
	GRAPHICS_DATA_FORMAT::FORMAT format = GRAPHICS_DATA_FORMAT::NONE;

	uint32_t width;
	uint32_t height;
	uint32_t mipLevels;
};

struct GRAPHICS_SHADER_RESOURCE_DESC
{
	enum TYPE
	{
		SHADER_RESOURCE_TYPE_NONE,
		SHADER_RESOURCE_TYPE_BUFFER,
		SHADER_RESOURCE_TYPE_TEXTURE2D
	};

	TYPE type = SHADER_RESOURCE_TYPE_NONE;

	union
	{
		GRAPHICS_SHADER_RESOURCE_TYPE_BUFFER_DESC buffer = {};
		GRAPHICS_SHADER_RESOURCE_TYPE_TEXTURE2D_DESC texture2D;
	};
};

using GRAPHICS_RENDER_TARGET_DESC = GRAPHICS_SHADER_RESOURCE_TYPE_TEXTURE2D_DESC;

using GRAPHICS_DEPTH_STENCIL_BUFFER_DESC = GRAPHICS_SHADER_RESOURCE_TYPE_TEXTURE2D_DESC;

struct GRAPHICS_PARAMS_DESC
{
	constexpr static size_t NUM_CONSTANT_BUFFER = 16;
	constexpr static size_t NUM_SHADER_RESOURCE = 16;

	GRAPHICS_CONSTANT_BUFFER_DESC constantBufferDesc[NUM_CONSTANT_BUFFER] = {};
	GRAPHICS_SHADER_RESOURCE_DESC shaderResourceDesc[NUM_SHADER_RESOURCE] = {};
};

struct GRAPHICS_PIPELINE_INPUT_CLASSIFICATION
{
	enum TYPE
	{
		INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		INPUT_CLASSIFICATION_PER_INSTANCE_DATA
	};
};

// just copied from d3d12 =)))
struct GRAPHICS_PIPELINE_INPUT_ELEMENT_DESC
{
	const char* semanticName;
	uint32_t												semanticIndex;
	GRAPHICS_DATA_FORMAT::FORMAT							format;
	uint32_t												inputSlot;
	uint32_t												alignedByteOffset;
	GRAPHICS_PIPELINE_INPUT_CLASSIFICATION::TYPE			inputSlotClass;
	uint32_t												instanceDataStepRate;
};

struct GRAPHICS_PIPELINE_INPUT_DESC
{
	uint32_t numElements = 0;
	GRAPHICS_PIPELINE_INPUT_ELEMENT_DESC elements[16];
};

//struct GRAPHICS_PIPELINE_DEPTH_STENCIL_DESC
//{
//	bool                       depthEnable;
//	D3D12_DEPTH_WRITE_MASK     depthWriteMask;
//	D3D12_COMPARISON_FUNC      depthFunc;
//	BOOL                       stencilEnable;
//	UINT8                      stencilReadMask;
//	UINT8                      stencilWriteMask;
//	D3D12_DEPTH_STENCILOP_DESC frontFace;
//	D3D12_DEPTH_STENCILOP_DESC backFace;
//};

struct GRAPHICS_PIPELINE_OUTPUT_DESC
{
	uint32_t numRenderTarget;
	GRAPHICS_DATA_FORMAT::FORMAT RTVFormat[8];

	//GRAPHICS_DATA_FORMAT::FORMAT DSVFormat;
};

struct GRAPHICS_PRIMITIVE_TOPOLOGY
{
	enum TYPE 
	{
		PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED = 0,
		PRIMITIVE_TOPOLOGY_TYPE_POINT = 1,
		PRIMITIVE_TOPOLOGY_TYPE_LINE = 2,
		PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE = 3,
		PRIMITIVE_TOPOLOGY_TYPE_PATCH = 4,

		COUNT
	};
};

struct GRAPHICS_COMPUTE_PIPELINE_DESC
{

};

struct GRAPHICS_PIPELINE_DESC
{
	const char* vs = nullptr;
	const char* ps = nullptr;
	const char* hs = nullptr;
	const char* ds = nullptr;
	const char* gs = nullptr;

	GRAPHICS_PIPELINE_INPUT_DESC inputDesc;
	GRAPHICS_PIPELINE_OUTPUT_DESC outputDesc;

	GRAPHICS_PARAMS_DESC vsParamsDesc;
	GRAPHICS_PARAMS_DESC psParamsDesc;
	GRAPHICS_PARAMS_DESC hsParamsDesc;
	GRAPHICS_PARAMS_DESC dsParamsDesc;
	GRAPHICS_PARAMS_DESC gsParamsDesc;

	GRAPHICS_PRIMITIVE_TOPOLOGY::TYPE primitiveTopology;

	size_t preferRenderCallPerFrame = -1;
};

struct GRAPHICS_RAYTRACING_PIPELINE_DESC
{

};

struct GRAPHICS_SHADER_SPACE
{
	enum SPACE
	{
		SHADER_SPACE_VS,
		SHADER_SPACE_PS,
		SHADER_SPACE_GS,
		SHADER_SPACE_HS,
		SHADER_SPACE_DS,

		COUNT
	};
};

class GraphicsShaderResource
{
public:
	// only work if GRAPHICS_SHADER_RESOURCE_DESC::TYPE is SHADER_RESOURCE_TYPE_BUFFER
	virtual void UpdateBuffer(const void* buffer, size_t bufferSize) = 0;

	// only work if GRAPHICS_SHADER_RESOURCE_DESC::TYPE is SHADER_RESOURCE_TYPE_TEXTURE2D
	virtual void UpdateTexture2D(const void* buffer, size_t bufferSize, const TEXTURE2D_REGION& region, bool endUpdateChain = true) = 0;

	virtual void GetDesc(GRAPHICS_SHADER_RESOURCE_DESC* output) = 0;

};

class GraphicsConstantBuffer
{
public:
	virtual void UpdateBuffer(const void* buffer, size_t bufferSize) = 0;

};


class GraphicsRenderTarget
{
protected:
	SharedPtr<GraphicsShaderResource> m_shaderResource = nullptr;

public:
	inline const SharedPtr<GraphicsShaderResource>& GetShaderResource()
	{
		return m_shaderResource;
	}

};

class GraphicsDepthStencilBuffer
{
protected:
	SharedPtr<GraphicsShaderResource> m_shaderResource = nullptr;

public:
	inline const SharedPtr<GraphicsShaderResource>& GetShaderResource()
	{
		return m_shaderResource;
	}

};

class GraphicsParams
{
public:
	// set ConstantBuffer with async update content
	virtual void SetConstantBuffers(GRAPHICS_SHADER_SPACE::SPACE shaderSpace, ID baseSlotIndex,
		uint32_t numBuffers, SharedPtr<GraphicsConstantBuffer>* constantBuffers) = 0;

	// set Shader Resource
	virtual void SetShaderResources(GRAPHICS_SHADER_SPACE::SPACE shaderSpace, ID baseSlotIndex,
		uint32_t numResources, SharedPtr<GraphicsShaderResource>* shaderResources) = 0;

};

class GraphicsVertexBuffer
{
public:
	virtual void UpdateBuffer(const void* buffer, size_t bufferSize) = 0;

};

class GraphicsIndexBuffer
{

};

//class GraphicsPipelineInput
//{
//public:
//	virtual void SetVertexBuffers(uint32_t numBuffers, GraphicsVertexBuffer** vertexBuffer) = 0;
//	virtual void SetIndexBuffers(uint32_t numBuffers, GraphicsIndexBuffer** indexBuffer) = 0;
//
//};
//
//class GraphicsPipelineOutput
//{
//public:
//	virtual void SetRenderTarget(uint32_t numRT, GraphicsRenderTarget** rtv) = 0;
//	virtual void SetDepthStencilBuffer(GraphicsDepthStencilBuffer* dsv) = 0;
//
//};
//
//struct GraphicsRenderCall
//{
//	GraphicsParams* params;
//	GraphicsPipelineInput* input;
//	GraphicsPipelineOutput* output;
//};

class GraphicsPipeline
{
public:
	virtual GraphicsParams* PrepareRenderParams() = 0;

};

NAMESPACE_END